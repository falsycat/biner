cmake_minimum_required(VERSION 3.18)

project(biner C)

include(CTest)

set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD          "11")
add_compile_options(
  -Wall -Wextra -pedantic -Werror -Wno-missing-field-initializers
)
