cmake_minimum_required(VERSION 3.20)
project(minesweeper LANGUAGES CXX)

add_executable(minesweeper
    src/main.cpp
    src/game/game.cpp
    src/button/button.cpp
    src/tui/tui.cpp
)

add_executable(minesweeper_test
    src/tui/tui.cpp
    tests/tui_test.cpp
)

set_target_properties(minesweeper PROPERTIES
    EXPORT_COMPILE_COMMANDS ON
    CXX_EXTENSIONS OFF
)

target_include_directories(minesweeper PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(minesweeper_test PRIVATE cxx_std_20)
target_compile_features(minesweeper PRIVATE cxx_std_20)
target_compile_options(minesweeper PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror -Wsign-conversion -Woverloaded-virtual
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wshadow-all>
)