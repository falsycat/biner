#include "./tree.h"

#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "./zone.h"

#include "generated/biner.y.h"

const char* const biner_tree_struct_member_type_name_string_map
    [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_MAX_] = {
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_UINT8]   = "uint8_t",
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_UINT16]  = "uint16_t",
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_UINT32]  = "uint32_t",
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_UINT64]  = "uint64_t",
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_INT8]    = "int8_t",
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_INT16]   = "int16_t",
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_INT32]   = "int32_t",
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_INT64]   = "int64_t",
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_FLOAT32] = "float",
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_FLOAT64] = "double",
};

biner_tree_parse_context_t biner_tree_parse_context_ = {0};

const uint8_t* biner_tree_parse(FILE* fp) {
  if (atomic_flag_test_and_set(&biner_tree_parse_context_.dirty)) {
    fprintf(stderr, "parsing context is dirty now\n");
    abort();
  }

  extern FILE* yyin;
  yyin = fp;

  biner_tree_parse_context_.root = biner_zone_alloc(
    &biner_tree_parse_context_.zone, sizeof(biner_tree_root_t));
  assert(biner_tree_parse_context_.root == 0);
  return yyparse()? NULL: biner_tree_parse_context_.zone.ptr;
}
