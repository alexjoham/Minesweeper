# Minesweeper

![CI](https://github.com/alexjoham/Minesweeper/actions/workflows/ci.yml/badge.svg)

A mouse-driven Minesweeper for the terminal, written in C++20.

The game board is 9x9 and there are 10 mines. It's not the game itself that's interesting. The purpose of this repository is the framework around it: target-based CMake, a game core that contains no I/O, GoogleTest with 41 test cases, AddressSanitizer and UndefinedBehaviorSanitizer presets, `clang-tidy` and `clang-format` enforced in CI, and every warning promoted to an error on two compilers.

Tagged [`v2.0`](https://github.com/alexjoham/Minesweeper/releases/tag/v2.0).

## What it looks like

The grids below are produced by the real rendering code (`Tui::glyph`) against a fixed mine layout,
with the ANSI colour codes stripped.

A fresh board. Every cell is hidden.

```
┌───────────────────────────┐
│ ▢  ▢  ▢  ▢  ▢  ▢  ▢  ▢  ▢ │
│ ▢  ▢  ▢  ▢  ▢  ▢  ▢  ▢  ▢ │
│ ▢  ▢  ▢  ▢  ▢  ▢  ▢  ▢  ▢ │
│ ▢  ▢  ▢  ▢  ▢  ▢  ▢  ▢  ▢ │
│ ▢  ▢  ▢  ▢  ▢  ▢  ▢  ▢  ▢ │
│ ▢  ▢  ▢  ▢  ▢  ▢  ▢  ▢  ▢ │
│ ▢  ▢  ▢  ▢  ▢  ▢  ▢  ▢  ▢ │
│ ▢  ▢  ▢  ▢  ▢  ▢  ▢  ▢  ▢ │
│ ▢  ▢  ▢  ▢  ▢  ▢  ▢  ▢  ▢ │
└───────────────────────────┘
```

Opening the empty cell at row 0, column 0. A cell with no adjacent mines opens its whole connected
region in one breadth-first sweep, and stops on the first ring of numbered cells.

```
┌───────────────────────────┐
│                           │
│                           │
│                           │
│                           │
│ 1  1  1              1  1 │
│ ▢  ▢  1  1  1  1  1  2  ▢ │
│ ▢  ▢  ▢  ▢  ▢  ▢  ▢  ▢  ▢ │
│ ▢  ▢  ▢  ▢  ▢  ▢  ▢  ▢  ▢ │
│ ▢  ▢  ▢  ▢  ▢  ▢  ▢  ▢  ▢ │
└───────────────────────────┘
```

A flag at row 7, column 0, and a numbered cell opened at row 6, column 1. Flagged cells cannot be
opened by a click and are never consumed by a flood fill.

```
┌───────────────────────────┐
│                           │
│                           │
│                           │
│                           │
│ 1  1  1              1  1 │
│ ▢  ▢  1  1  1  1  1  2  ▢ │
│ ▢  2  ▢  ▢  ▢  ▢  ▢  ▢  ▢ │
│ ⚑  ▢  ▢  ▢  ▢  ▢  ▢  ▢  ▢ │
│ ▢  ▢  ▢  ▢  ▢  ▢  ▢  ▢  ▢ │
└───────────────────────────┘
```

A mine at row 7, column 5 was clicked. The board is revealed and the game is over.

```
┌───────────────────────────┐
│                           │
│                           │
│                           │
│                           │
│ 1  1  1              1  1 │
│ 1  ★  1  1  1  1  1  2  ★ │
│ 2  2  1  1  ★  2  2  ★  3 │
│ ★  2  2  3  3  ★  3  3  ★ │
│ 1  2  ★  ★  2  2  ★  2  1 │
└───────────────────────────┘
```

### Glyphs

| Glyph | Meaning |
|---|---|
| `▢` | Hidden cell |
| `⚑` | Flagged cell |
| `★` | Mine |
| (blank) | Opened cell with no adjacent mines |
| `1` to `8` | Opened cell, count of adjacent mines including diagonals |

## Controls

| Input | Effect |
|---|---|
| Left click on `Start` | Leave the menu and begin a game |
| Left click on `Quit` | Exit |
| Left click on a cell | Open it |
| Left click on the `⚑` button | Toggle flag mode; the button inverts while flag mode is on |
| Left click on a cell in flag mode | Place or remove a flag |
| `r` | Restart with a fresh random layout, from any state |
| `q` or `Ctrl+C` | Exit and restore the terminal |

A mouse is required. The program puts the terminal into raw mode and enables SGR mouse reporting
(`\x1b[?1006h`), so it needs a real TTY that supports mouse events. Without one it prints
`not a terminal` and exits with status 1.

## Requirements

- A C++20 compiler. Verified on GCC 13.3 and Clang 18.1.
- CMake 3.20 or newer.
- A POSIX terminal. The program uses `termios` and `unistd.h` directly, so Linux and macOS work and
  Windows does not, except under WSL.
- Network access on the first configure. GoogleTest is fetched with `FetchContent`, pinned to
  `v1.18.0`. Nothing else is downloaded.

## Build, run, test

Presets are the shortest path. They write into `build/debug`, `build/release` and `build/asan`.

```bash
cmake --preset debug
cmake --build --preset debug --parallel
./build/debug/minesweeper
```

Run the tests:

```bash
ctest --preset debug
```

Expected output ends with `100% tests passed out of 41`.

Release build:

```bash
cmake --preset release
cmake --build --preset release --parallel
ctest --preset release
```

Sanitizers. The `asan` preset adds `-fsanitize=address,undefined -fno-omit-frame-pointer -g`, and
the matching test preset sets `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1` so that undefined
behaviour aborts the run instead of printing and continuing.

```bash
cmake --preset asan
cmake --build --preset asan --parallel
ctest --preset asan
```

Pick a specific compiler without a preset:

```bash
cmake -S . -B build/clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug
cmake --build build/clang --parallel
ctest --test-dir build/clang --output-on-failure
```

Run a single test or a subset by name:

```bash
ctest --preset debug -R GameTest
ctest --preset debug -R 'TuiGlyph.*' --output-on-failure
ctest --preset debug -N
./build/debug/ms_tests --gtest_filter='GameTest.*'
```

### Static analysis and formatting

Both tools are pinned to version 18 in CI, because the set of findings changes between releases.

```bash
# include-what-you-use enforcement, same invocation as CI
clang-tidy-18 -p build/debug \
  --checks='-*,misc-include-cleaner' \
  --warnings-as-errors='*' \
  src/game/game.cpp src/tui/tui.cpp src/button/button.cpp src/main.cpp \
  tests/game_test.cpp tests/tui_test.cpp

# formatting check, non-destructive
clang-format-18 --dry-run --Werror $(git ls-files '*.cpp' '*.hpp')

# apply formatting
clang-format-18 -i $(git ls-files '*.cpp' '*.hpp')
```

`clang-tidy` needs `compile_commands.json`, which the build generates because
`CMAKE_EXPORT_COMPILE_COMMANDS` is `ON`. Point `-p` at a configured build directory.

## Continuous integration

`.github/workflows/ci.yml` runs five jobs on every push to `main` and every pull request. All five
must be green.

| Job | What it proves |
|---|---|
| `g++` | Configures, builds and tests under GCC with warnings as errors |
| `clang++` | The same under Clang, which diagnoses a different set of defects |
| `ASan + UBSan` | The test suite is clean under both sanitizers |
| `clang-tidy (misc-include-cleaner)` | Every translation unit includes the headers declaring the symbols it uses |
| `clang-format` | The tree matches `.clang-format` |

## Layout

```
src/
  main.cpp              raw-mode terminal setup, SGR mouse parsing, the event loop
  helpers.hpp           write() wrapper, cursor positioning, screen clear
  game/
    game.hpp/.cpp       Game: board state, flood fill, flagging, win check. No I/O.
    random_layout.hpp   randomLayout<N>(mine_count, rng), a pure function
  tui/
    tui.hpp/.cpp        Tui: renders a Game::Board, maps screen coordinates to cells
  button/
    button.hpp/.cpp     Button base, LabelButton, UnicodeButton
  enums/                FieldType, GameState
  structs/              BoardCoord
tests/
  game_test.cpp         25 cases against the core
  tui_test.cpp          glyph and hit-testing cases
```

CMake targets:

| Target | Kind | Contents |
|---|---|---|
| `ms_warnings` | `INTERFACE` | The warning flag set, applied per target and never to dependencies |
| `ms_core` | library | `game.cpp`. Contains no reference to `write` or to any ANSI escape |
| `ms_tui` | library | `tui.cpp`, `button.cpp`. Links `ms_core` publicly |
| `minesweeper` | executable | `main.cpp` |
| `ms_tests` | executable | Links `ms_tui` and `GTest::gtest_main`, registered with `gtest_discover_tests` |

The split is the most important design constraint. `Game` never prints. Therefore, every rule in the game can be tested without a terminal, screen buffer or captured stream. This is why the core carries 25 tests and the flood fill can be asserted cell by cell.

## Engineering standards

- `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror`, plus `-Wsign-conversion` and
  `-Woverloaded-virtual`, carried by the `ms_warnings` interface target. Never in
  `CMAKE_CXX_FLAGS`, so GoogleTest is not compiled with them.
- C++20 through `target_compile_features`, with `CMAKE_CXX_EXTENSIONS OFF`.
- Include what you use, enforced. Transitive includes are not relied upon.
- `#pragma once` in every header. Functions defined in headers are `inline`, not `static`.
- `const` by default, `noexcept` where it is true and load-bearing, `override` on every override.
- One logical change per commit.