cmake_minimum_required(VERSION 3.20)
project(minesweeper LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(ms_warnings INTERFACE)
target_compile_options(ms_warnings INTERFACE
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror
    -Wsign-conversion -Woverloaded-virtual
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wshadow-all>
)

add_library(ms_core
    src/game/game.cpp
)
target_include_directories(ms_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(ms_core PUBLIC cxx_std_20)
target_link_libraries(ms_core PRIVATE ms_warnings)

add_library(ms_tui
    src/tui/tui.cpp
    src/button/button.cpp
)
target_include_directories(ms_tui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(ms_tui PUBLIC cxx_std_20)
target_link_libraries(ms_tui PUBLIC ms_core PRIVATE ms_warnings)

add_executable(minesweeper src/main.cpp)
target_link_libraries(minesweeper PRIVATE ms_tui ms_warnings)


include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.18.0
)
set(BUILD_GMOCK OFF)
set(INSTALL_GTEST OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS OFF)
FetchContent_MakeAvailable(googletest)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()
include(GoogleTest)
add_executable(ms_tests
    tests/game_test.cpp
    tests/tui_test.cpp
)
target_link_libraries(ms_tests PRIVATE ms_tui ms_warnings GTest::gtest_main)
gtest_discover_tests(ms_tests)
