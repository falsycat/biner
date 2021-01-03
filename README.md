biner
====

The Simplest Interface Description Language for Binary Format

## features

- provides readable way to express binary format
- generates pack/unpack function for C from biner file

## requirements

- flex
- bison
- cmake
- gcc (generated codes uses GCC extension)

## install

1. add this repo to yours as submodule
2. write the following code on your CMakeLists.txt

```CMake
add_subdirectory(path/to/biner)
target_biner_sources(target-name
  OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/generated
  SOURCES
    file1.biner
    file2.biner
)
```

Now `file1.biner.h` and `file2.biner.h` are generated under `${CMAKE_CURRENT_BINARY_DIR}/generated` directory.

You can get an example from `test` directory.

## license

Do What The Fuck You Want To Public License

## author

falsycat

- [portfolio](https://falsy.cat)
- [twitter](https://twitter.com/falsycat)

If you find something, please contact me at email address written in my portfolio.
