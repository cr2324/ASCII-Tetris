cmake_minimum_required(VERSION 3.10)

project(ASCII_Tetris C)

add_executable(ascii_tetris main.c tetris.c) 

target_sources(ascii_tetris PRIVATE main.c)

target_include_directories(ascii_tetris PRIVATE include)