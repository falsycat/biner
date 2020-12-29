#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "./transpile.h"
#include "./tree.h"

int main(void) {
  const uint8_t* zone = biner_tree_parse(stdin);
  if (zone == NULL) return EXIT_FAILURE;

  biner_transpile_c(&(biner_transpile_param_t) {
      .zone = zone,
      .dst  = stdout,
    });
  return EXIT_SUCCESS;
}
