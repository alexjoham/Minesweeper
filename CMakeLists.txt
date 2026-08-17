cmake_minimum_required(VERSION 3.20)
project(minesweeper LANGUAGES CXX)

add_executable(minesweeper
    main.cpp
    game/game.cpp
    button/button.cpp
)

target_include_directories(minesweeper PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(minesweeper PRIVATE cxx_std_20)