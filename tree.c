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
