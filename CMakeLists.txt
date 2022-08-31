cmake_minimum_required(VERSION 3.23)

project(dragonk LANGUAGES C)

add_library(dragonk-core src/core/str.c src/core/file.c src/core/arg.c)
target_include_directories(dragonk-core PUBLIC include)
target_compile_features(dragonk-core PUBLIC c_std_11)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
find_package(GPerf REQUIRED)
add_library(dragonk-compiler src/compiler/token.c src/compiler/lexer.c)
gperf_generate(
  gperf/keywords.gperf
  ${CMAKE_CURRENT_BINARY_DIR}/include/dragon/gperf/keywords.h dragonk-compiler
)
target_include_directories(
  dragonk-compiler PUBLIC include ${CMAKE_CURRENT_BINARY_DIR}/include
)
target_compile_features(dragonk-compiler PUBLIC c_std_11)
target_link_libraries(dragonk-compiler PUBLIC dragonk-core)

add_executable(dragonk-driver src/driver/main.c)
target_link_libraries(dragonk-driver PRIVATE dragonk-compiler dragonk-core)
