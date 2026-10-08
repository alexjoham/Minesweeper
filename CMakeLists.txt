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

set(MS_HEADERS
    src/button/button.hpp
    src/enums/field_type.hpp
    src/enums/game_state.hpp
    src/game/game.hpp
    src/game/random_layout.hpp
    src/helpers.hpp
    src/structs/board_coord.hpp
    src/tui/tui.hpp
)
# Compile every header as its own translation unit, so it shows up in
# compile_commands.json for clang-tidy and has to be self-contained.
# GCC before 15 can't silence "#pragma once in main file", so skip it there.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 15)
    add_library(ms_header_tus OBJECT ${MS_HEADERS})
    set_source_files_properties(${MS_HEADERS} PROPERTIES LANGUAGE CXX)
    target_compile_features(ms_header_tus PRIVATE cxx_std_20)
    target_compile_options(ms_header_tus PRIVATE -Wno-pragma-once-outside-header)
    target_link_libraries(ms_header_tus PRIVATE ms_warnings)
endif()


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
