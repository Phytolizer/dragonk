cmake_minimum_required(VERSION 3.23)

project(dragonk LANGUAGES C)

add_library(
  dragonk-core src/core/str.c src/core/file.c src/core/arg.c src/core/strtox.c
               src/core/process.c
)
target_include_directories(dragonk-core PUBLIC include)
target_compile_features(dragonk-core PUBLIC c_std_11)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(EmbedFile)

add_executable(embed src/embed/embed.c)
target_compile_features(embed PRIVATE c_std_11)

find_package(GPerf REQUIRED)
add_library(
  dragonk-compiler
  src/compiler/token.c src/compiler/lexer.c src/compiler/parser.c
  src/compiler/ast.c src/compiler/codegen.c
)
gperf_generate(
  gperf/keywords.gperf
  ${CMAKE_CURRENT_BINARY_DIR}/include/dragon/gperf/keywords.h dragonk-compiler
)
gperf_generate(
  gperf/ppkeywords.gperf
  ${CMAKE_CURRENT_BINARY_DIR}/include/dragon/gperf/ppkeywords.h
  dragonk-compiler
)
embed_file(resources/snippets/header.nasm HEADER_NASM dragonk-compiler)
target_include_directories(
  dragonk-compiler PUBLIC include ${CMAKE_CURRENT_BINARY_DIR}/include
)
target_compile_features(dragonk-compiler PUBLIC c_std_11)
target_link_libraries(dragonk-compiler PUBLIC dragonk-core)

add_library(dragonk-driver src/driver/run.c)
target_link_libraries(dragonk-driver PUBLIC dragonk-compiler dragonk-core)

add_executable(dragonk src/main.c)
target_link_libraries(dragonk PRIVATE dragonk-driver)

configure_file(cmake/config.h.cmake-in include/dragon/config.h)

add_executable(
  dragonk-test tests/test.c tests/parser.c tests/list.c tests/lexer.c
               tests/execute.c
)
target_link_libraries(dragonk-test PRIVATE dragonk-driver)
target_include_directories(dragonk-test PRIVATE tests/include)

enable_testing()
add_test(NAME dragonk-test COMMAND dragonk-test)
