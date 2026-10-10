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
