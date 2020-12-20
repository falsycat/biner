#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "./tree.h"

int main(void) {
  const uint8_t* zone = biner_tree_parse(stdin);
  if (zone == NULL) return EXIT_FAILURE;

  const biner_tree_root_t* root = (const biner_tree_root_t*) zone;

  const biner_tree_decl_t* decl =
    (const biner_tree_decl_t*) (zone + root->decls);
  while ((uintptr_t) decl != (uintptr_t) zone) {
    printf("%s:\n", zone + decl->name);

    const biner_tree_struct_member_t* member =
      (const biner_tree_struct_member_t*) (zone + decl->member);
    while ((uintptr_t) member != (uintptr_t) zone) {
      printf("  %s\n", zone + member->name);
      member = (const biner_tree_struct_member_t*) (zone + member->prev);
    }
    decl = (const biner_tree_decl_t*) (zone + decl->prev);
  }
  return EXIT_SUCCESS;
}
