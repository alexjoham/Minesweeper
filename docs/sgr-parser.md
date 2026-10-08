# SGR mouse parser

This is a plan for replacing the input loop in `src/main.cpp` (roughly lines 113–255) with a small parser that can be tested.

## What's wrong with the current parsing

**mx/my are swapped.** SGR reports are `ESC [ < button ; column ; row M`, but the `sscanf` call writes the column into `my` and the row into `mx`. It only works because `Button::press` and `boardCellAt` also take the row first. The new parser uses the names `row` and `column`.

**Esc eats the next key.** Pressing Esc and then `q` doesn't quit. After an `ESC`, anything other than `[` clears the buffer, and that byte gets dropped too. The same happens with Alt+q in terminals that send Alt as Esc. If you press Esc and then click, the press report is lost: the second `ESC` gets dropped and the rest (`[<0;10;6M`) is handled as ordinary keys. The release that follows still gets through. That's enough for the board, which only reacts to the release. Buttons need both the press and the release, so clicking Start, Quit or the flag button after Esc does nothing. It should work like this: if the byte after an `ESC` doesn't continue a sequence, it's handled as a normal key.

**Numbers that don't fit in an int.** cppreference says that for `%d`, if the number doesn't fit in the target type, the behavior is undefined. I tried it with a small test program:

```
ESC[<0;4294967301;3M   ->  column = 5
```

On my Mac it wraps silently, so the bytes above would count as a click on row 3, column 5. UBSan doesn't report it because the overflow happens inside libc. The new parser rejects it.

**`%d` is too lenient.** It skips whitespace and accepts a `+` or `-` sign, so `ESC[<0;-5;7M` gets through. `sscanf` also only checks that three numbers were read, so `ESC[<0;5;5;9M` (four numbers) is accepted as well.

**The mouse's back button counts as a left click.** According to the xterm docs (ctlseqs), buttons 8–11 are reported as 128 and up. For 128, the checks `btn & 0x20`, `btn & 0x40` and `btn & 3` are all 0, so the back button on a mouse counts as a left click.

**Can't quit in the middle of a sequence.** Once the buffer has 4 or more bytes, only `M` and `m` are checked. If you type Esc `[` `<`, then `q` and Ctrl+C do nothing until you type an `m`/`M` or reach 33 bytes. When the 32-byte limit hits, the rest of the sequence is handled as normal keys.

## Push or pull

Push:

```cpp
constexpr std::optional<InputEvent> feed(std::uint8_t byte);
```

Input from the terminal never ends, and it arrives a byte at a time whenever the user does something. No caller ever has a complete buffer it could hand to the parser. To pull, the parser would need a source it can ask for the next byte, and that source blocks. So a blocking read, or a callback standing in for one, would end up inside the parser.

With push, the parser is a plain function from (state, byte) to (new state, maybe an event). It doesn't know where the bytes come from. The caller owns the loop, so if we later want a timeout for a lone Esc, or `poll()` on more than one input, that change stays in `main()` and the parser doesn't change.

## Reference: Williams' VT parser

The byte rules follow Paul Williams' state machine, "A parser for DEC's ANSI-compatible video terminals" (vt100.net/emu/dec_ansi_parser). From it:

- Inside a control sequence, `0x30–0x3F` are parameter bytes, `0x20–0x2F` are intermediates, and **any byte `0x40–0x7E` ends the sequence**. Many of those aren't letters: F5 is `ESC[15~` and bracketed paste is `ESC[200~`, both ending in `~`.
- C0 controls (for example Ctrl+C) in the middle of a sequence are executed, and the sequence keeps going.
- `ESC` from any state abandons the current sequence and starts a new one.
- CAN (`0x18`) and SUB (`0x1A`) from any state abandon the sequence and go back to ground.
- DEL (`0x7F`) is ignored.

Where we deviate from it, and why:

- **ESC followed by a normal byte** (Alt+q is `ESC q`) is a complete escape sequence for Williams and gets dispatched. We hand the byte back as a key instead, so pressing Esc never swallows the next key and Alt+q still quits.
- **SS3** (`ESC O` plus one byte) is how xterm sends F1–F4 and the arrow keys in application mode. Williams dispatches at `O`. We consume one more byte, so `P` from F1 doesn't leak out as a key. This has a cost (see "ESC O" under edge cases).
- **We don't handle DCS, OSC, PM or APC strings.** The terminal only sends those as replies to queries, and we never send queries.
- **Bytes `0x80` and above** are ordinary data, not C1 controls, because terminal input is UTF-8.

## State

```cpp
enum class SgrState : std::uint8_t {
    Ground,      // normal input
    Escape,      // got ESC
    Ss3,         // got ESC O, one more byte to consume
    CsiEntry,    // got ESC [
    MouseParam,  // got ESC [ <, reading numbers
    Discard,     // CSI we don't want or that is broken, skip until its final byte
};
```

Besides the state, the parser keeps `params` (three `uint16_t`), `index` (which number it's on, 0–2) and `digits` (digits in the current number, 0–4). It never decides anything from a buffer's length. `CsiEntry → MouseParam` resets all three to 0.

The table is the specification: every edge case below is a row in it, or a guard on a row. The guards keep `index ≤ 2` and `digits ≤ 4` at all times, so `params[index]` can never be out of bounds and a 4-digit number always fits in a `uint16_t`.

These transitions apply in every state:
- `ESC` goes to `Escape`.
- CAN and SUB go to `Ground`.
- DEL is ignored.
- Any other C0 control is reported as a key, and the state doesn't change.

The rest:

| State | Byte | Next state | Output |
|---|---|---|---|
| Ground | anything else | Ground | key |
| Escape | `[` | CsiEntry | – |
| Escape | `O` | Ss3 | – |
| Escape | anything else | Ground | key (that byte) |
| Ss3 | anything | Ground | – |
| CsiEntry | `<` | MouseParam | – |
| CsiEntry | `0x40–0x7E` | Ground | – |
| CsiEntry | anything else | Discard | – |
| MouseParam | digit, `digits < 4` | MouseParam | – (append the digit, `digits++`) |
| MouseParam | digit, `digits == 4` | Discard | – |
| MouseParam | `;`, `digits > 0` and `index < 2` | MouseParam | – (`index++`, `digits = 0`) |
| MouseParam | `;`, `digits == 0` or `index == 2` | Discard | – |
| MouseParam | `M` / `m`, `report_ok` | Ground | mouse event |
| MouseParam | `M` / `m`, not `report_ok` | Ground | – |
| MouseParam | other `0x40–0x7E` | Ground | – |
| MouseParam | anything else | Discard | – |
| Discard | `0x40–0x7E` | Ground | – |
| Discard | anything else | Discard | – |

`report_ok` is the only check left for the final byte:
- `index == 2` and `digits > 0`, meaning all three numbers are there.
- Row and column are ≥ 1.
- The button code decodes to an allowed combination (see "Button codes" below).

Too many values and empty values never get this far, because the `;` rows already sent them to `Discard`.

Checking it against the Esc-then-click case, `ESC ESC [ < 0 ; 1 0 ; 6 M`:

```
ESC  -> Escape
ESC  -> Escape          (first ESC abandoned, nothing reported)
[    -> CsiEntry
<    -> MouseParam
0;10;6 -> MouseParam    (params 0, 10, 6)
M    -> Ground          MouseEvent{Left, Press, row 6, column 10}
```

The press arrives, so the buttons work after Esc.

## Output

```cpp
enum class MouseButton : std::uint8_t {
    Left, Middle, Right, None,
    WheelUp, WheelDown, WheelLeft, WheelRight,
    Button8, Button9, Button10, Button11,
};
enum class MouseAction : std::uint8_t { Press, Release, Motion };

struct MouseEvent {
    MouseButton button;
    MouseAction action;
    bool shift, meta, ctrl;
    std::uint16_t row;     // 1-based
    std::uint16_t column;  // 1-based
    constexpr bool operator==(const MouseEvent&) const = default;
};
struct KeyEvent {
    std::uint8_t byte;
    constexpr bool operator==(const KeyEvent&) const = default;
};
using InputEvent = std::variant<KeyEvent, MouseEvent>;
```

## Edge cases

- **Too many values or an empty value** (`ESC[<0;5;5;9M`, `ESC[<;5;5M`): the `;` that causes it switches to `Discard`. No event.
- **Too few values, or row/column 0** (`ESC[<0;5M`, `ESC[<0;0;5M`): `report_ok` fails at the final byte. No event, back to `Ground`.
- **Overlong number:** each number can have at most 4 digits. A 5th digit switches to `Discard`, and nothing is reported.
- **Negative number:** `-` is an intermediate byte (`0x2D`), so the parser goes to `Discard` and reports nothing. The same goes for `+` and spaces.
- **Printable byte in the middle of a sequence:** a byte in `0x40–0x7E` ends the sequence, nothing is reported, and that byte isn't treated as a key. Any other byte that isn't a digit or `;` switches to `Discard`. Ctrl+C is still passed through as a key, so quitting always works.
- **Button codes:** the button code is made of the low bits plus flags 4, 8 and 16 (modifiers), 32 (motion), 64 (wheel) and 128 (buttons 8–11). `report_ok` rejects the following, and each gives no event and goes back to `Ground`:
  - Codes from 192 to 255, which set both 64 and 128 and aren't defined by xterm.
  - Anything above 255, which no terminal sends.
  - `None` (low bits 3) without the motion bit. In SGR mode a release reports the real button and ends in `m`, so `{None, Press}` and `{None, Release}` never come from a terminal. `{None, Motion}` is valid: that's mouse movement with no button held.
  - A wheel code ending in `m`, or a wheel code with the motion bit. The wheel only sends presses, so `{WheelUp, Release}` is never produced.
  - The motion bit with a final `m`. Motion always ends in `M`.
- **Lone ESC:** nothing happens until the next byte arrives, and that byte decides (see the table). There's no timeout, because `read()` blocks and nothing in the game uses Esc. A timeout could be added in `main()` later.
- **ESC O (known trade-off):** `Ss3` consumes whatever byte comes next. Without a timeout, F1 (`ESC O P`) and Alt+Shift+O (`ESC O`) followed by a key look the same. So pressing Alt+Shift+O and then `q` doesn't quit, which is the second defect again in a narrower form. I accept it for three reasons. Alt+Shift+O means nothing in this game. Only one key is lost, and pressing it again works. Without `Ss3`, every press of F1–F4 would leak `P`, `Q`, `R` or `S` as a key, and the game would start reacting to those the moment anything is bound to them. The same timeout that would fix the lone Esc would fix this too.
- **Longer than any legal sequence:** the longest valid report is exactly 18 bytes. That's `ESC [ <`, three 4-digit numbers, two `;` and the final byte. Anything longer must contain a 5-digit number or a fourth value, so the parser is already in `Discard` and skips ahead to the final byte. Nothing is buffered.

## Boundary

The parser reports every valid mouse event (all buttons, the wheel, drag and modifier keys) and every other byte as a key. It knows nothing about the game.

`main()` decides what to use: it ignores drag (`Motion`), the wheel, and every button except `Left`. It also handles `q`, `r` and Ctrl+C.

The parser doesn't need I/O or allocation, so all of it is `constexpr` in a single header, `src/input/sgr_parser.hpp`. That lets tests run byte sequences at compile time.

The test helper `parse_all` feeds a string and returns **every** event it produced, not just the last one. That way a stray `KeyEvent` before the right `MouseEvent` fails the test. It returns a fixed-capacity array plus a count, so it stays `constexpr`. A byte never produces more than one event, so the capacity only has to be as large as the longest test input:

```cpp
template <std::size_t N> struct Events { std::array<InputEvent, N> items{}; std::size_t count = 0; };

constexpr auto ev = parse_all("\x1b\x1b[<0;10;6M");
static_assert(ev.count == 1);
static_assert(ev.items[0] == InputEvent{MouseEvent{MouseButton::Left, MouseAction::Press,
                                                   false, false, false, /*row*/ 6, /*column*/ 10}});
```

It lives in its own CMake target, `ms_input`. That's an `INTERFACE` library, because there's no `.cpp`, and `minesweeper` and `ms_tests` link it. It doesn't depend on the game, so it shouldn't go in `ms_tui`, which pulls in `ms_core`.

A header that's only compiled through the files that include it gets incomplete clang-tidy coverage, and none at all from `misc-include-cleaner`. So `src/input/sgr_parser.hpp` gets added to `MS_HEADERS`, the list the `ms_header_tus` object library compiles directly as C++ (`LANGUAGE CXX`), which puts the header in `compile_commands.json` as its own translation unit (with Clang or GCC 15+; older GCC can't silence `#pragma once in main file`, so the target is skipped there). Nothing in CI needs to change: the lint job already runs `run-clang-tidy-18 -p build` over every entry under `src/` and `tests/`, and the format job already checks every C++ file from `git ls-files`, so both pick up the new header automatically.
