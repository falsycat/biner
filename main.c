#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "./transpile.h"
#include "./tree.h"

int main(int argc, char** argv) {
  const uint8_t* zone = biner_tree_parse(stdin);
  if (zone == NULL) return EXIT_FAILURE;

  if (argc == 0) {
    fprintf(stderr, "assertion failure: argc > 1");
    return EXIT_FAILURE;
  }

  biner_transpile_c(&(biner_transpile_param_t) {
      .zone = zone,
      .dst  = stdout,
      .argc = argc-1,
      .argv = argv+1,
    });
  return EXIT_SUCCESS;
}
