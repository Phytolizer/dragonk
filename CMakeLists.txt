cmake_minimum_required(VERSION 3.23)

project(dragonk LANGUAGES C)

add_library(dragonk-compiler src/compiler/token.c)
target_include_directories(dragonk-compiler PUBLIC include)
