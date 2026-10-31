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
                return feed_escape(byte);
            case SgrState::Ss3:
                state_ = SgrState::Ground;
                return std::nullopt;
            case SgrState::CsiEntry:
                feed_csi_entry(byte);
                return std::nullopt;
            case SgrState::MouseParam:
                return feed_mouse_param(byte);
            case SgrState::Discard:
                if (is_final(byte)) {
                    state_ = SgrState::Ground;
                }
                return std::nullopt;
        }
        return std::nullopt;
    }

private:
    // Any byte 0x40-0x7E ends a CSI sequence (docs/sgr-parser.md).
    static constexpr bool is_final(std::uint8_t byte) noexcept { return byte >= 0x40 && byte <= 0x7E; }

    constexpr std::optional<InputEvent> feed_escape(std::uint8_t byte) noexcept {
        if (byte == '[') {
            state_ = SgrState::CsiEntry;
            return std::nullopt;
        }
        if (byte == 'O') {
            state_ = SgrState::Ss3;
            return std::nullopt;
        }
        state_ = SgrState::Ground;
        return KeyEvent{byte};
    }

    constexpr void feed_csi_entry(std::uint8_t byte) noexcept {
        if (byte == '<') {
            state_ = SgrState::MouseParam;
            params_ = {};
            index_ = 0;
            digits_ = 0;
        } else if (is_final(byte)) {
            state_ = SgrState::Ground;
        } else {
            state_ = SgrState::Discard;
        }
    }

    constexpr std::optional<InputEvent> feed_mouse_param(std::uint8_t byte) noexcept {
        if (byte >= '0' && byte <= '9') {
            if (digits_ == 4) { // a 5th digit
                state_ = SgrState::Discard;
                return std::nullopt;
            }
            // digits_ < 4 keeps the value <= 9999, so it fits in a uint16_t.
            params_[index_] = static_cast<std::uint16_t>(params_[index_] * 10 + (byte - '0'));
            ++digits_;
            return std::nullopt;
        }
        if (byte == ';') {
            if (digits_ == 0 || index_ == 2) { // empty value, or a 4th value
                state_ = SgrState::Discard;
                return std::nullopt;
            }
            ++index_;
            digits_ = 0;
            return std::nullopt;
        }
        if (byte == 'M' || byte == 'm') {
            state_ = SgrState::Ground;
            return report(byte == 'M' ? MouseAction::Press : MouseAction::Release);
        }
        state_ = is_final(byte) ? SgrState::Ground : SgrState::Discard;
        return std::nullopt;
    }

    // Group 3 only knows button code 0 (Left). Every other code is
    // rejected until the button code decoding of group 4 is in.
    [[nodiscard]] constexpr std::optional<InputEvent> report(MouseAction action) const noexcept {
        const bool complete = index_ == 2 && digits_ > 0;
        const std::uint16_t column = params_[1];
        const std::uint16_t row = params_[2];
        if (!complete || column == 0 || row == 0 || params_[0] != 0) {
            return std::nullopt;
        }
        return MouseEvent{MouseButton::Left, action, false, false, false, row, column};
    }

    SgrState state_ = SgrState::Ground;
    std::array<std::uint16_t, 3> params_{};
    std::uint8_t index_ = 0;  // 0-2
    std::uint8_t digits_ = 0; // 0-4
};
