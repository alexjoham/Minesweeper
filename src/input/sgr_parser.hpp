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
    // Ground, Escape and Ss3 so far: ESC [ and the CSI states in docs/sgr-parser.md are not implemented yet.
    [[nodiscard]] constexpr std::optional<InputEvent> feed(std::uint8_t byte) noexcept {
        // Rules that apply in every state.
        switch (byte) {
            case 0x7F: // DEL
                return std::nullopt;
            case 0x18: // CAN
            case 0x1A: // SUB
                state_ = SgrState::Ground;
                return std::nullopt;
            case 0x1B: // ESC
                state_ = SgrState::Escape;
                return std::nullopt;
            default:
                break;
        }
        if (byte < 0x20) { // any other C0 control, state unchanged
            return KeyEvent{byte};
        }

        switch (state_) {
            case SgrState::Ground:
                return KeyEvent{byte};
            case SgrState::Escape:
                if (byte == 'O') {
                    state_ = SgrState::Ss3;
                    return std::nullopt;
                }
                state_ = SgrState::Ground;
                return KeyEvent{byte};
            case SgrState::Ss3:
                state_ = SgrState::Ground;
                return std::nullopt;
            case SgrState::CsiEntry:
            case SgrState::MouseParam:
            case SgrState::Discard:
                break; // nothing enters these states yet
        }
        return std::nullopt;
    }

private:
    SgrState state_ = SgrState::Ground;
    [[maybe_unused]] std::array<std::uint16_t, 3> params_{};
    [[maybe_unused]] std::uint8_t index_ = 0;  // 0-2
    [[maybe_unused]] std::uint8_t digits_ = 0; // 0-4
};
