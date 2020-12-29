#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct biner_transpile_param_t {
  const uint8_t* zone;
  FILE*          dst;
} biner_transpile_param_t;

bool
biner_transpile_c(
  const biner_transpile_param_t* p
);
