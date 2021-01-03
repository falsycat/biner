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
    transpile.h
    transpile_c.c
    tree.h
    tree.c
    zone.h
    ${BISON_biner-parser_OUTPUTS}
    ${FLEX_biner-scanner_OUTPUTS}
  PUBLIC
    c/pack.h
    c/result.h
    c/unpack.h
)
target_include_directories(biner PRIVATE . ${CMAKE_CURRENT_BINARY_DIR})

function(target_biner_sources target)
  cmake_parse_arguments("FUNC" "DIVIDE" "OUTPUT" "SOURCES" ${ARGN})
  file(MAKE_DIRECTORY "${FUNC_OUTPUT}")

  foreach(path ${FUNC_SOURCES})
    get_filename_component(basename "${path}" NAME)
    set(in  "${CMAKE_CURRENT_SOURCE_DIR}/${path}")
    set(out "${FUNC_OUTPUT}/${basename}")
    if(FUNC_DIVIDE)
      add_custom_command(
          OUTPUT  "${out}.type.h"
          COMMAND $<TARGET_FILE:biner> --only-type < "${in}" > "${out}.type.h"
          DEPENDS "${in}" biner
          COMMENT "transpiling ${basename} to ${basename}.type.h"
          VERBATIM)
      add_custom_command(
          OUTPUT  "${out}.func.h"
          COMMAND $<TARGET_FILE:biner> --only-func < "${in}" > "${out}.func.h"
          DEPENDS "${in}" biner
          COMMENT "transpiling ${basename} to ${basename}.func.h"
          VERBATIM)
      target_sources(${target} PRIVATE "${out}.type.h" "${out}.func.h")
    else()
      add_custom_command(
          OUTPUT  "${out}.h"
          COMMAND $<TARGET_FILE:biner> < "${in}" > "${out}.h"
          DEPENDS "${in}" biner
          COMMENT "transpiling ${basename} to ${basename}.h"
          VERBATIM)
      target_sources(${target} PRIVATE "${out}.h" "${out}.h")
    endif()
  endforeach()
endfunction()

if (BUILD_TESTING)
  add_subdirectory(test)
endif()
