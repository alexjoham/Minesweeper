#include "src/input/sgr_parser.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <string_view>

namespace {

template <std::size_t N>
struct Events {
    std::array<InputEvent, N> items{};
    std::size_t count = 0;
};

// N counts the terminating '\0', so the input is N - 1 bytes.
template <std::size_t N>
constexpr Events<N - 1> parse_all(const char (&input)[N]) {
    SgrParser parser;
    Events<N - 1> events;
    // feed() yields at most one event per byte, so count <= N - 1
    // and items[count++] can never go out of bounds.
    for (const char c : std::string_view{input, N - 1}) {
        if (const auto ev = parser.feed(static_cast<std::uint8_t>(c))) {
            events.items[events.count++] = *ev;
        }
    }
    return events;
}

// Every group 4 report is at row 6, column 10; the tests only differ in
// the button code and the final byte.
constexpr MouseEvent mouse(MouseButton button, MouseAction action, bool shift = false, bool meta = false,
                           bool ctrl = false) {
    return MouseEvent{button, action, shift, meta, ctrl, 6, 10};
}

} // namespace

TEST(SgrParserTest, PrintableByteGivesOneKey) {
    const auto ev = parse_all("a");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}

TEST(SgrParserTest, SeveralBytesKeepTheirOrder) {
    const auto ev = parse_all("ab");
    ASSERT_EQ(ev.count, 2U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
    EXPECT_EQ(ev.items[1], InputEvent{KeyEvent{'b'}});
}

TEST(SgrParserTest, C0ControlIsReportedAsKey) {
    const auto ev = parse_all("\x03"); // Ctrl+C
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{0x03}});
}

TEST(SgrParserTest, ByteAbove0x7FIsOrdinaryData) {
    const auto ev = parse_all("\xc3");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{0xC3}});
}

// The no-event rules below follow with 'a': asserting only "no event"
// would pass against a parser that does nothing.
TEST(SgrParserTest, DelIsIgnored) {
    const auto ev = parse_all("\x7f"
                              "a");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}

TEST(SgrParserTest, CanInGroundGivesNoEvent) {
    const auto ev = parse_all("\x18"
                              "a");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}

TEST(SgrParserTest, SubInGroundGivesNoEvent) {
    const auto ev = parse_all("\x1a"
                              "a");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}

TEST(SgrParserTest, EscThenKeyGivesThatKey) {
    const auto ev = parse_all("\x1b"
                              "q"); // Alt+q, or Esc then q
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'q'}});
}

TEST(SgrParserTest, EscThenHighByteGivesThatByte) {
    const auto ev = parse_all("\x1b"
                              "\xc3");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{0xC3}});
}

// Several tests below continue with "OPa": O enters Ss3 only if the
// parser is still in Escape, so the output shows which state it was in.
TEST(SgrParserTest, SecondEscRestartsTheSequence) {
    const auto ev = parse_all("\x1b"
                              "\x1b"
                              "OPa");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}

TEST(SgrParserTest, CanInEscapeReturnsToGround) {
    const auto ev = parse_all("\x1b"
                              "\x18"
                              "O");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'O'}});
}

TEST(SgrParserTest, SubInEscapeReturnsToGround) {
    const auto ev = parse_all("\x1b"
                              "\x1a"
                              "O");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'O'}});
}

TEST(SgrParserTest, DelInEscapeIsIgnored) {
    const auto ev = parse_all("\x1b"
                              "\x7f"
                              "OPa");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}

TEST(SgrParserTest, C0InEscapeIsReportedAndKeepsState) {
    const auto ev = parse_all("\x1b"
                              "\x03" // Ctrl+C
                              "OPa");
    ASSERT_EQ(ev.count, 2U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{0x03}});
    EXPECT_EQ(ev.items[1], InputEvent{KeyEvent{'a'}});
}

TEST(SgrParserTest, Ss3SwallowsOneByte) {
    const auto ev = parse_all("\x1b"  //
                              "OPa"); // F1, then a
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}

TEST(SgrParserTest, EscInSs3RestartsTheSequence) {
    const auto ev = parse_all("\x1b"
                              "O"
                              "\x1b"
                              "OPa");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}

TEST(SgrParserTest, CanInSs3ReturnsToGround) {
    const auto ev = parse_all("\x1b"
                              "O"
                              "\x18"
                              "a");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}

TEST(SgrParserTest, DelInSs3IsIgnored) {
    const auto ev = parse_all("\x1b"
                              "O"
                              "\x7f"
                              "Pa");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}

TEST(SgrParserTest, C0InSs3IsReportedAndKeepsState) {
    const auto ev = parse_all("\x1b"
                              "O"
                              "\x03" // Ctrl+C
                              "Pa");
    ASSERT_EQ(ev.count, 2U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{0x03}});
    EXPECT_EQ(ev.items[1], InputEvent{KeyEvent{'a'}});
}

// Known trade-off (docs/sgr-parser.md, "ESC O"): without a timeout,
// Alt+Shift+O followed by a key loses that key.
TEST(SgrParserTest, Ss3SwallowsOrdinaryKey) {
    const auto ev = parse_all("\x1b"
                              "Oqa");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}

// SGR reports are ESC [ < button ; column ; row, then M or m. Column and
// row differ in every test, so swapping them cannot pass.
TEST(SgrParserTest, LeftPressGivesMouseEvent) {
    const auto ev = parse_all("\x1b"
                              "[<0;10;6M");
    constexpr MouseEvent expected{
        .button = MouseButton::Left,
        .action = MouseAction::Press,
        .shift = false,
        .meta = false,
        .ctrl = false,
        .row = 6,
        .column = 10,
    };
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{expected});
}

TEST(SgrParserTest, LeftReleaseGivesMouseEvent) {
    const auto ev = parse_all("\x1b"
                              "[<0;10;6m");
    constexpr MouseEvent expected{
        .button = MouseButton::Left,
        .action = MouseAction::Release,
        .shift = false,
        .meta = false,
        .ctrl = false,
        .row = 6,
        .column = 10,
    };
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{expected});
}

// The Esc-then-click case from docs/sgr-parser.md: the first ESC is
// abandoned silently and the press still arrives.
TEST(SgrParserTest, EscThenClickGivesExactlyOneEvent) {
    const auto ev = parse_all("\x1b"
                              "\x1b"
                              "[<0;10;6M");
    constexpr MouseEvent expected{
        .button = MouseButton::Left,
        .action = MouseAction::Press,
        .shift = false,
        .meta = false,
        .ctrl = false,
        .row = 6,
        .column = 10,
    };
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{expected});
}

// The tests below continue with 'a': asserting only "no event" would
// pass against a parser that does nothing, and 'a' shows that the parser
// is back in Ground afterwards.

// F5 is ESC [ 15 ~: not a mouse report, so nothing comes out.
TEST(SgrParserTest, NonMouseCsiGivesNothing) {
    const auto ev = parse_all("\x1b"
                              "[15~a");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}

// The final byte comes right after ESC [ (an arrow key): CsiEntry row.
TEST(SgrParserTest, CsiFinalByteRightAfterEntryGivesNothing) {
    const auto ev = parse_all("\x1b"
                              "[Aa");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}

TEST(SgrParserTest, TooManyValuesGiveNothing) {
    const auto ev = parse_all("\x1b"
                              "[<0;5;5;9Ma");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}

TEST(SgrParserTest, EmptyFirstValueGivesNothing) {
    const auto ev = parse_all("\x1b"
                              "[<;5;5Ma");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}

TEST(SgrParserTest, EmptyMiddleValueGivesNothing) {
    const auto ev = parse_all("\x1b"
                              "[<0;;5Ma");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}

TEST(SgrParserTest, TooFewValuesGiveNothing) {
    const auto ev = parse_all("\x1b"
                              "[<0;5Ma");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}

TEST(SgrParserTest, RowZeroGivesNothing) {
    const auto ev = parse_all("\x1b"
                              "[<0;5;0Ma");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}

TEST(SgrParserTest, ColumnZeroGivesNothing) {
    const auto ev = parse_all("\x1b"
                              "[<0;0;5Ma");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}

TEST(SgrParserTest, FiveDigitNumberGivesNothing) {
    const auto ev = parse_all("\x1b"
                              "[<0;10000;5Ma");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}

TEST(SgrParserTest, MinusSignGivesNothing) {
    const auto ev = parse_all("\x1b"
                              "[<0;-5;7Ma");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}

// A final byte other than M or m ends the report without an event.
TEST(SgrParserTest, OtherFinalByteInMouseParamGivesNothing) {
    const auto ev = parse_all("\x1b"
                              "[<0;5;5~a");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}

// The other side of the 5-digit rejection: 9999 still fits.
TEST(SgrParserTest, FourDigitMaximumIsAccepted) {
    const auto ev = parse_all("\x1b"
                              "[<0;9999;9998M");
    constexpr MouseEvent expected{
        .button = MouseButton::Left,
        .action = MouseAction::Press,
        .shift = false,
        .meta = false,
        .ctrl = false,
        .row = 9998,
        .column = 9999,
    };
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{expected});
}

// Ctrl+C inside a sequence is reported and the sequence goes on.
TEST(SgrParserTest, C0InMouseParamIsReportedAndKeepsState) {
    const auto ev = parse_all("\x1b"
                              "[<0;1"
                              "\x03" // Ctrl+C
                              "0;6M");
    constexpr MouseEvent expected{
        .button = MouseButton::Left,
        .action = MouseAction::Press,
        .shift = false,
        .meta = false,
        .ctrl = false,
        .row = 6,
        .column = 10,
    };
    ASSERT_EQ(ev.count, 2U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{0x03}});
    EXPECT_EQ(ev.items[1], InputEvent{expected});
}

// Ctrl+C inside a rejected sequence still quits.
TEST(SgrParserTest, C0InDiscardIsReportedAndKeepsState) {
    const auto ev = parse_all("\x1b"
                              "[<;"
                              "\x03" // Ctrl+C
                              "5;5Ma");
    ASSERT_EQ(ev.count, 2U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{0x03}});
    EXPECT_EQ(ev.items[1], InputEvent{KeyEvent{'a'}});
}

// Group 4: the button code. Low bits 0-3 select the button, 4/8/16 are
// shift/meta/ctrl, 32 is motion, 64 the wheel and 128 buttons 8-11.
TEST(SgrParserTest, MiddlePressGivesMiddle) {
    const auto ev = parse_all("\x1b"
                              "[<1;10;6M");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{mouse(MouseButton::Middle, MouseAction::Press)});
}
TEST(SgrParserTest, RightPressGivesRight) {
    const auto ev = parse_all("\x1b"
                              "[<2;10;6M");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{mouse(MouseButton::Right, MouseAction::Press)});
}
// Mouse movement with no button held: low bits 3 plus the motion bit.
TEST(SgrParserTest, NoneWithMotionGivesNoneMotion) {
    const auto ev = parse_all("\x1b"
                              "[<35;10;6M");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{mouse(MouseButton::None, MouseAction::Motion)});
}

// Each modifier flag has its own test with the other two false, so a
// swapped flag cannot pass.
TEST(SgrParserTest, Flag4SetsShift) {
    const auto ev = parse_all("\x1b"
                              "[<4;10;6M");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{mouse(MouseButton::Left, MouseAction::Press, true, false, false)});
}
TEST(SgrParserTest, Flag8SetsMeta) {
    const auto ev = parse_all("\x1b"
                              "[<8;10;6M");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{mouse(MouseButton::Left, MouseAction::Press, false, true, false)});
}
TEST(SgrParserTest, Flag16SetsCtrl) {
    const auto ev = parse_all("\x1b"
                              "[<16;10;6M");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{mouse(MouseButton::Left, MouseAction::Press, false, false, true)});
}
TEST(SgrParserTest, AllModifierFlagsTogether) {
    const auto ev = parse_all("\x1b"
                              "[<28;10;6M");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{mouse(MouseButton::Left, MouseAction::Press, true, true, true)});
}
// Drag: the button stays Left, the action becomes Motion.
TEST(SgrParserTest, Flag32GivesMotion) {
    const auto ev = parse_all("\x1b"
                              "[<32;10;6M");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{mouse(MouseButton::Left, MouseAction::Motion)});
}

TEST(SgrParserTest, Code64GivesWheelUp) {
    const auto ev = parse_all("\x1b"
                              "[<64;10;6M");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{mouse(MouseButton::WheelUp, MouseAction::Press)});
}
TEST(SgrParserTest, Code65GivesWheelDown) {
    const auto ev = parse_all("\x1b"
                              "[<65;10;6M");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{mouse(MouseButton::WheelDown, MouseAction::Press)});
}
TEST(SgrParserTest, Code66GivesWheelLeft) {
    const auto ev = parse_all("\x1b"
                              "[<66;10;6M");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{mouse(MouseButton::WheelLeft, MouseAction::Press)});
}
TEST(SgrParserTest, Code67GivesWheelRight) {
    const auto ev = parse_all("\x1b"
                              "[<67;10;6M");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{mouse(MouseButton::WheelRight, MouseAction::Press)});
}
// 64 + 16: the wheel with ctrl held.
TEST(SgrParserTest, WheelKeepsItsModifiers) {
    const auto ev = parse_all("\x1b"
                              "[<80;10;6M");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{mouse(MouseButton::WheelUp, MouseAction::Press, false, false, true)});
}

TEST(SgrParserTest, Code128GivesButton8) {
    const auto ev = parse_all("\x1b"
                              "[<128;10;6M");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{mouse(MouseButton::Button8, MouseAction::Press)});
}
TEST(SgrParserTest, Code129GivesButton9) {
    const auto ev = parse_all("\x1b"
                              "[<129;10;6M");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{mouse(MouseButton::Button9, MouseAction::Press)});
}
TEST(SgrParserTest, Code130GivesButton10) {
    const auto ev = parse_all("\x1b"
                              "[<130;10;6M");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{mouse(MouseButton::Button10, MouseAction::Press)});
}
TEST(SgrParserTest, Code131GivesButton11) {
    const auto ev = parse_all("\x1b"
                              "[<131;10;6M");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{mouse(MouseButton::Button11, MouseAction::Press)});
}
TEST(SgrParserTest, Button8ReleaseGivesRelease) {
    const auto ev = parse_all("\x1b"
                              "[<128;10;6m");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{mouse(MouseButton::Button8, MouseAction::Release)});
}

// Rejections from "Button codes" in docs/sgr-parser.md. They continue with
// 'a' like the group 3 rejections. 192 and 255 are the ends of the range
// that sets both 64 and 128; 256 and 9999 are above any legal code.
TEST(SgrParserTest, Code192GivesNothing) {
    const auto ev = parse_all("\x1b"
                              "[<192;10;6Ma");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}
TEST(SgrParserTest, Code255GivesNothing) {
    const auto ev = parse_all("\x1b"
                              "[<255;10;6Ma");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}
TEST(SgrParserTest, Code256GivesNothing) {
    const auto ev = parse_all("\x1b"
                              "[<256;10;6Ma");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}
TEST(SgrParserTest, Code9999GivesNothing) {
    const auto ev = parse_all("\x1b"
                              "[<9999;10;6Ma");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}

// None without the motion bit never comes from a terminal.
TEST(SgrParserTest, NonePressGivesNothing) {
    const auto ev = parse_all("\x1b"
                              "[<3;10;6Ma");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}
TEST(SgrParserTest, NoneReleaseGivesNothing) {
    const auto ev = parse_all("\x1b"
                              "[<3;10;6ma");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}
// The wheel only sends presses.
TEST(SgrParserTest, WheelReleaseGivesNothing) {
    const auto ev = parse_all("\x1b"
                              "[<64;10;6ma");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}
// 64 + 32.
TEST(SgrParserTest, WheelWithMotionGivesNothing) {
    const auto ev = parse_all("\x1b"
                              "[<96;10;6Ma");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}
// Motion always ends in M.
TEST(SgrParserTest, MotionWithReleaseGivesNothing) {
    const auto ev = parse_all("\x1b"
                              "[<32;10;6ma");
    ASSERT_EQ(ev.count, 1U);
    EXPECT_EQ(ev.items[0], InputEvent{KeyEvent{'a'}});
}
