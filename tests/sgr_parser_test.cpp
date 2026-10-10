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
