#pragma once

#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "./c/zone.h"

typedef struct biner_tree_expr_t                    biner_tree_expr_t;
typedef struct biner_tree_struct_member_t           biner_tree_struct_member_t;
typedef struct biner_tree_struct_member_reference_t biner_tree_struct_member_reference_t;
typedef struct biner_tree_enum_member_t             biner_tree_enum_member_t;
typedef struct biner_tree_decl_t                    biner_tree_decl_t;

typedef enum biner_tree_expr_type_t {
  BINER_TREE_EXPR_TYPE_OPERAND_INTEGER,
  BINER_TREE_EXPR_TYPE_OPERAND_REFERENCE,
  BINER_TREE_EXPR_TYPE_OPERATOR_EQUAL,
  BINER_TREE_EXPR_TYPE_OPERATOR_NEQUAL,
  BINER_TREE_EXPR_TYPE_OPERATOR_GREATER_EQUAL,
  BINER_TREE_EXPR_TYPE_OPERATOR_LESS_EQUAL,
  BINER_TREE_EXPR_TYPE_OPERATOR_GREATER,
  BINER_TREE_EXPR_TYPE_OPERATOR_LESS,
  BINER_TREE_EXPR_TYPE_OPERATOR_NOT,
  BINER_TREE_EXPR_TYPE_OPERATOR_ADD,
  BINER_TREE_EXPR_TYPE_OPERATOR_SUB,
  BINER_TREE_EXPR_TYPE_OPERATOR_MUL,
  BINER_TREE_EXPR_TYPE_OPERATOR_DIV,
} biner_tree_expr_type_t;

typedef struct biner_tree_expr_t {
  biner_tree_expr_type_t type;
  bool dynamic;
  union {
    int64_t i;
    biner_zone_ptr(biner_tree_struct_member_reference_t) r;
    struct {
      biner_zone_ptr(biner_tree_expr_t) l;
      biner_zone_ptr(biner_tree_expr_t) r;
    } operands;
  };
} biner_tree_expr_t;

typedef enum biner_tree_struct_member_type_name_t {
  BINER_TREE_STRUCT_MEMBER_TYPE_NAME_LU8,
  BINER_TREE_STRUCT_MEMBER_TYPE_NAME_LU16,
  BINER_TREE_STRUCT_MEMBER_TYPE_NAME_LU32,
  BINER_TREE_STRUCT_MEMBER_TYPE_NAME_LU64,
  BINER_TREE_STRUCT_MEMBER_TYPE_NAME_LI8,
  BINER_TREE_STRUCT_MEMBER_TYPE_NAME_LI16,
  BINER_TREE_STRUCT_MEMBER_TYPE_NAME_LI32,
  BINER_TREE_STRUCT_MEMBER_TYPE_NAME_LI64,
  BINER_TREE_STRUCT_MEMBER_TYPE_NAME_BU8,
  BINER_TREE_STRUCT_MEMBER_TYPE_NAME_BU16,
  BINER_TREE_STRUCT_MEMBER_TYPE_NAME_BU32,
  BINER_TREE_STRUCT_MEMBER_TYPE_NAME_BU64,
  BINER_TREE_STRUCT_MEMBER_TYPE_NAME_BI8,
  BINER_TREE_STRUCT_MEMBER_TYPE_NAME_BI16,
  BINER_TREE_STRUCT_MEMBER_TYPE_NAME_BI32,
  BINER_TREE_STRUCT_MEMBER_TYPE_NAME_BI64,
  BINER_TREE_STRUCT_MEMBER_TYPE_NAME_F16,
  BINER_TREE_STRUCT_MEMBER_TYPE_NAME_F32,
  BINER_TREE_STRUCT_MEMBER_TYPE_NAME_F64,
  BINER_TREE_STRUCT_MEMBER_TYPE_NAME_USER_DECL,

  BINER_TREE_STRUCT_MEMBER_TYPE_NAME_MAX_,
} biner_tree_struct_member_type_name_t;

typedef struct biner_tree_struct_member_type_name_meta_t {
  const char* name;
  size_t      size;
} biner_tree_struct_member_type_name_meta_t;

extern const biner_tree_struct_member_type_name_meta_t
biner_tree_struct_member_type_name_meta_map
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_MAX_];

typedef enum biner_tree_struct_member_type_qualifier_t {
  BINER_TREE_STRUCT_MEMBER_TYPE_QUALIFIER_NONE,
  BINER_TREE_STRUCT_MEMBER_TYPE_QUALIFIER_STATIC_ARRAY,
  BINER_TREE_STRUCT_MEMBER_TYPE_QUALIFIER_DYNAMIC_ARRAY,
} biner_tree_struct_member_type_qualifier_t;

typedef struct biner_tree_struct_member_type_t {
  biner_tree_struct_member_type_name_t name;
  union {
    biner_zone_ptr(biner_tree_decl_t) decl;
  };

  biner_tree_struct_member_type_qualifier_t qualifier;
  union {
    size_t i;
    biner_zone_ptr(biner_tree_expr_t) expr;
  };
} biner_tree_struct_member_type_t;

typedef struct biner_tree_struct_member_t {
  size_t index;
  biner_zone_ptr(char)                            name;
  biner_zone_ptr(biner_tree_struct_member_type_t) type;
  biner_zone_ptr(biner_tree_expr_t)               condition;
  biner_zone_ptr(biner_tree_struct_member_t)      prev;
} biner_tree_struct_member_t;

typedef struct biner_tree_struct_member_reference_t {
  biner_zone_ptr(biner_tree_struct_member_t)           member;
  biner_zone_ptr(biner_tree_expr_t)                    index;
  biner_zone_ptr(biner_tree_struct_member_reference_t) prev;
} biner_tree_struct_member_reference_t;

typedef struct biner_tree_enum_member_t {
  biner_zone_ptr(char)                     name;
  biner_zone_ptr(biner_tree_expr_t)        expr;
  biner_zone_ptr(biner_tree_enum_member_t) prev;
} biner_tree_enum_member_t;

typedef enum biner_tree_decl_type_t {
  BINER_TREE_DECL_TYPE_CONST,
  BINER_TREE_DECL_TYPE_ENUM,
  BINER_TREE_DECL_TYPE_STRUCT,
} biner_tree_decl_type_t;

typedef struct biner_tree_decl_t {
  biner_zone_ptr(char) name;
  biner_tree_decl_type_t type;
  union {
    biner_zone_ptr(void) body;

    biner_zone_ptr(biner_tree_expr_t)          const_;
    biner_zone_ptr(biner_tree_enum_member_t)   enum_;
    biner_zone_ptr(biner_tree_struct_member_t) struct_;
  };
  biner_zone_ptr(biner_tree_decl_t) prev;
} biner_tree_decl_t;

typedef struct biner_tree_root_t {
  biner_zone_ptr(biner_tree_decl_t) decls;
} biner_tree_root_t;

typedef struct biner_tree_parse_context_t {
  atomic_flag  dirty;
  biner_zone_t zone;

  size_t line;
  size_t column;

  biner_zone_ptr(biner_tree_root_t) root;
  biner_zone_ptr(biner_tree_decl_t) last_decl;

  biner_zone_ptr(biner_tree_struct_member_t) last_struct;
  biner_zone_ptr(biner_tree_enum_member_t)   last_enum;
} biner_tree_parse_context_t;

extern biner_tree_parse_context_t biner_tree_parse_context_;

const uint8_t*
biner_tree_parse(
  FILE* fp
);
