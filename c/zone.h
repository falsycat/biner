#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define biner_zone_ptr(T) uintptr_t

typedef struct biner_zone_t {
  uintptr_t tail;
  size_t    size;
  uint8_t*  ptr;
} biner_zone_t;

#define BINER_ZONE_RESERVE 1024

static inline uintptr_t biner_zone_alloc(biner_zone_t* z, size_t sz) {
  assert(z != NULL);
  assert(sz > 0);

  if (z->ptr == NULL) {
    z->ptr = calloc(1, sz);
    if (z->ptr == NULL) {
      fprintf(stderr, "malloc failure\n");
      abort();
    }
  }
  const uintptr_t oldtail = z->tail;
  z->tail += sz;
  if (z->tail > z->size) {
    z->size = z->tail;
    z->ptr  = realloc(z->ptr, z->size);
  }
  memset(z->ptr+oldtail, 0, sz);
  return oldtail;
}

static inline uintptr_t biner_zone_strnew(biner_zone_t* z, const char* str) {
  assert(z != NULL);

  const uintptr_t ret = biner_zone_alloc(z, strlen(str)+1);
  strcpy((char*) (z->ptr+ret), str);
  return ret;
}

static inline void biner_zone_deinitialize(biner_zone_t* z) {
  assert(z != NULL);

  if (z->ptr != NULL) free(z->ptr);
}
