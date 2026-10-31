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

// decode_button() computes a button as first + the low two bits of the
// code, so each group of four must stay consecutive and in this order.
static_assert(static_cast<int>(MouseButton::None) == static_cast<int>(MouseButton::Left) + 3);
static_assert(static_cast<int>(MouseButton::WheelRight) == static_cast<int>(MouseButton::WheelUp) + 3);
static_assert(static_cast<int>(MouseButton::Button11) == static_cast<int>(MouseButton::Button8) + 3);

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
            return report(byte == 'm');
        }
        state_ = is_final(byte) ? SgrState::Ground : SgrState::Discard;
        return std::nullopt;
    }

    // The final byte is already known to be M or m; `release` is true for m.
    [[nodiscard]] constexpr std::optional<InputEvent> report(bool release) const noexcept {
        const bool complete = index_ == 2 && digits_ > 0;
        const std::uint16_t column = params_[1];
        const std::uint16_t row = params_[2];
        if (!complete || column == 0 || row == 0) {
            return std::nullopt;
        }
        return decode_button(params_[0], release, row, column);
    }

    // Button code table of docs/sgr-parser.md: the low bits 0-3 select the
    // button, 4/8/16 are shift/meta/ctrl, 32 is motion, 64 the wheel and
    // 128 buttons 8-11. Returns nullopt for a combination a terminal never
    // sends.
    [[nodiscard]] static constexpr std::optional<MouseEvent>
    decode_button(std::uint16_t code, bool release, std::uint16_t row, std::uint16_t column) noexcept {
        const bool wheel = (code & 64) != 0;
        const bool extra = (code & 128) != 0;
        const bool motion = (code & 32) != 0;
        if (code > 255 || (wheel && extra)) { // 192-255 and above
            return std::nullopt;
        }
        if (motion && release) { // motion always ends in M
            return std::nullopt;
        }
        if (wheel && (motion || release)) { // the wheel only sends presses
            return std::nullopt;
        }
        const auto low = static_cast<std::uint8_t>(code & 3);
        MouseButton first = MouseButton::Left;
        if (wheel) {
            first = MouseButton::WheelUp;
        } else if (extra) {
            first = MouseButton::Button8;
        }
        const auto button = static_cast<MouseButton>(static_cast<std::uint8_t>(first) + low);
        if (button == MouseButton::None && !motion) { // never reported without motion
            return std::nullopt;
        }
        MouseAction action = MouseAction::Press;
        if (motion) {
            action = MouseAction::Motion;
        } else if (release) {
            action = MouseAction::Release;
        }
        return MouseEvent{button, action, (code & 4) != 0, (code & 8) != 0, (code & 16) != 0, row, column};
    }

    SgrState state_ = SgrState::Ground;
    std::array<std::uint16_t, 3> params_{};
    std::uint8_t index_ = 0;  // 0-2
    std::uint8_t digits_ = 0; // 0-4
};
