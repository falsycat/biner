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

const biner_tree_struct_member_type_name_meta_t
biner_tree_struct_member_type_name_meta_map
    [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_MAX_] = {
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_LU8]  = {"lu8",  1},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_LU16] = {"lu16", 2},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_LU32] = {"lu32", 4},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_LU64] = {"lu64", 8},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_LI8]  = {"li8",  1},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_LI16] = {"li16", 2},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_LI32] = {"li32", 4},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_LI64] = {"li64", 8},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_BU8]  = {"bu8",  1},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_BU16] = {"bu16", 2},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_BU32] = {"bu32", 4},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_BU64] = {"bu64", 8},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_BI8]  = {"bi8",  1},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_BI16] = {"bi16", 2},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_BI32] = {"bi32", 4},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_BI64] = {"bi64", 8},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_F16]  = {"f16",  2},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_F32]  = {"f32",  4},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_F64]  = {"f64",  8},
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
