#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <variant>

enum class SgrState : std::uint8_t {
    Ground,     // normal input
    Escape,     // got ESC
    Ss3,        // got ESC O, one more byte to consume
    CsiEntry,   // got ESC [
    MouseParam, // got ESC [ <, reading numbers
    Discard,    // CSI we don't want or that is broken, skip until its final byte
};

enum class MouseButton : std::uint8_t {
    Left,
    Middle,
    Right,
    None,
    WheelUp,
    WheelDown,
    WheelLeft,
    WheelRight,
    Button8,
    Button9,
    Button10,
    Button11,
};

enum class MouseAction : std::uint8_t { Press, Release, Motion };

struct MouseEvent {
    MouseButton button;
    MouseAction action;
    bool shift, meta, ctrl;
    std::uint16_t row;    // 1-based
    std::uint16_t column; // 1-based
    constexpr bool operator==(const MouseEvent&) const = default;
};

struct KeyEvent {
    std::uint8_t byte;
    constexpr bool operator==(const KeyEvent&) const = default;
};
using InputEvent = std::variant<KeyEvent, MouseEvent>;

class SgrParser {
public:
    // Ground only so far: ESC and the sequence states in docs/sgr-parser.md are not implemented yet.
    [[nodiscard]] constexpr std::optional<InputEvent> feed(std::uint8_t byte) noexcept {
        switch (byte) {
            case 0x7F: // DEL
            case 0x18: // CAN
            case 0x1A: // SUB
                return std::nullopt;
            default:
                return KeyEvent{byte};
        }
    }

private:
    [[maybe_unused]] SgrState state_ = SgrState::Ground;
    [[maybe_unused]] std::array<std::uint16_t, 3> params_{};
    [[maybe_unused]] std::uint8_t index_ = 0;  // 0-2
    [[maybe_unused]] std::uint8_t digits_ = 0; // 0-4
};
