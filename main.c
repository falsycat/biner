#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "./tree.h"

static inline void print_enum_(
    const uint8_t* zone, const biner_tree_decl_t* decl) {
  assert(zone != NULL);
  assert(decl != NULL);

  printf("enum %s:\n", zone + decl->name);
  const biner_tree_enum_member_t* member =
    (const biner_tree_enum_member_t*) (zone + decl->enum_);
  while ((uintptr_t) member != (uintptr_t) zone) {
    printf("  %s\n", zone + member->name);
    member = (const biner_tree_enum_member_t*) (zone + member->prev);
  }
}

static inline void print_struct_(
    const uint8_t* zone, const biner_tree_decl_t* decl) {
  assert(zone != NULL);
  assert(decl != NULL);

  printf("struct %s:\n", zone + decl->name);
  const biner_tree_struct_member_t* member =
    (const biner_tree_struct_member_t*) (zone + decl->struct_);
  while ((uintptr_t) member != (uintptr_t) zone) {
    printf("  %2zu ", member->index);
    const biner_tree_struct_member_type_t* type =
      (const biner_tree_struct_member_type_t*) (zone + member->type);
    if (type->name == BINER_TREE_STRUCT_MEMBER_TYPE_NAME_USER_DECL) {
      const biner_tree_decl_t* decl =
        (const biner_tree_decl_t*) (zone + type->decl);
      printf("%8s ", zone + decl->name);
    } else {
      printf("%8s ",
        biner_tree_struct_member_type_name_string_map[type->name]);
    }
    if (member->condition == 0) {
      printf("%s", zone + member->name);
    } else {
      printf("(%s)", zone + member->name);
    }
    printf("\n");
    member = (const biner_tree_struct_member_t*) (zone + member->prev);
  }
}

int main(void) {
  const uint8_t* zone = biner_tree_parse(stdin);
  if (zone == NULL) return EXIT_FAILURE;

  const biner_tree_root_t* root = (const biner_tree_root_t*) zone;

  const biner_tree_decl_t* decl =
    (const biner_tree_decl_t*) (zone + root->decls);
  while ((uintptr_t) decl != (uintptr_t) zone) {
    switch (decl->type) {
    case BINER_TREE_DECL_TYPE_CONST:
      printf("const %s\n", zone + decl->name);
      break;
    case BINER_TREE_DECL_TYPE_ENUM:
      print_enum_(zone, decl);
      break;
    case BINER_TREE_DECL_TYPE_STRUCT:
      print_struct_(zone, decl);
      break;
    default:
      ;
    }
    decl = (const biner_tree_decl_t*) (zone + decl->prev);
  }
  return EXIT_SUCCESS;
}
