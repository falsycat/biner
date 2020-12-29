cmake_minimum_required(VERSION 3.18)

project(biner C)

include(CTest)

set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD          "11")
add_compile_options(
  -Wall -Wextra -pedantic -Werror -Wno-missing-field-initializers
)

set(BINER_GENRATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

find_package(BISON REQUIRED)
find_package(FLEX  REQUIRED)

file(MAKE_DIRECTORY ${BINER_GENRATED_DIR})
bison_target(biner-parser biner.y ${BINER_GENRATED_DIR}/biner.y.c)
flex_target(biner-scanner biner.l ${BINER_GENRATED_DIR}/biner.l.c)
add_flex_bison_dependency(biner-scanner biner-parser)

add_executable(biner)
target_sources(biner
  PRIVATE
    main.c
    transpile_c.c
    tree.c
    ${BISON_biner-parser_OUTPUTS}
    ${FLEX_biner-scanner_OUTPUTS}
  PUBLIC
    c/zone.h
)
target_include_directories(biner PRIVATE . ${CMAKE_CURRENT_BINARY_DIR})
