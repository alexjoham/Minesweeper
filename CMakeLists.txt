cmake_minimum_required(VERSION 3.20)
project(minesweeper LANGUAGES CXX)

add_executable(minesweeper
    main.cpp
    game/game.cpp
    button/button.cpp
)

set_target_properties(minesweeper PROPERTIES
    EXPORT_COMPILE_COMMANDS ON
    CXX_EXTENSIONS OFF
)

target_include_directories(minesweeper PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(minesweeper PRIVATE cxx_std_20)
target_compile_options(minesweeper PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror -Wsign-conversion -Woverloaded-virtual
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wshadow-all>
)