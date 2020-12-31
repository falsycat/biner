add_executable(biner-test)
target_sources(biner-test
  PRIVATE
    main.c
)
target_biner_sources(biner-test
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated
  SOURCES
    user.biner
)
target_include_directories(biner-test
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)
add_test(
  NAME    biner-test
  COMMAND $<TARGET_FILE:biner-test>
)
