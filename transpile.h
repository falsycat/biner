#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct biner_transpile_param_t {
  const uint8_t* zone;
  FILE*          dst;

  size_t argc;
  char** argv;
} biner_transpile_param_t;

bool
biner_transpile_c(
  const biner_transpile_param_t* p
);

static inline bool biner_transpile_is_option_enabled(
    const biner_transpile_param_t* p, const char* name) {
  assert(p    != NULL);
  assert(name != NULL);

  for (size_t i = 0; i < p->argc; ++i) {
    if (strcmp(p->argv[i], name) == 0) return true;
  }
  return false;
}
