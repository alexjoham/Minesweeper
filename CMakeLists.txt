cmake_minimum_required(VERSION 3.20)
project(minesweeper LANGUAGES CXX)

add_library(ms_warnings INTERFACE)
target_compile_options(ms_warnings INTERFACE
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror
    -Wsign-conversion -Woverloaded-virtual
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wshadow-all>
)

add_library(ms_core
    src/game/game.cpp
    src/tui/tui.cpp
    src/button/button.cpp
)
target_include_directories(ms_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(ms_core PUBLIC cxx_std_20)
target_link_libraries(ms_core PRIVATE ms_warnings)

add_executable(minesweeper src/main.cpp)
target_link_libraries(minesweeper PRIVATE ms_core ms_warnings)

set_target_properties(minesweeper PROPERTIES
    EXPORT_COMPILE_COMMANDS ON
    CXX_EXTENSIONS OFF
)

enable_testing()
add_executable(tui_test tests/tui_test.cpp)
target_link_libraries(tui_test PRIVATE ms_core ms_warnings)
add_test(NAME tui_test COMMAND tui_test)