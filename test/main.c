#undef NDEBUG

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../c/pack.h"
#include "../c/result.h"
#include "../c/unpack.h"

static inline void* biner_malloc_(size_t sz, void* udata) {
  (void) udata;

  /* returning NULL can abort the unpacking */
  return malloc(sz);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include "generated/user.biner.h"
#pragma GCC diagnostic pop

static inline void print_user_(const user_t* u) {
  assert(u != NULL);

  const char* sex =
    u->sex == SEX_MALE?   "male":
    u->sex == SEX_FEMALE? "female":
    u->sex == SEX_CAT?    "cat":
    "unknown";
  const char* role =
    u->role == ROLE_TEACHER? "teacher":
    u->role == ROLE_STUDENT? "student":
    "unknown";

  printf("name: %.*s\n", (int) u->name.len, u->name.ptr);
  printf("age: %" PRIu8 "\n", u->age);
  printf("sex: %s\n", sex);
  printf("role: %s\n", role);

  printf("properties:\n");
  for (size_t i = 0; i < u->property_count; ++i) {
    printf("  %.*s: %.*s\n",
      (int) u->properties[i].key.len,   u->properties[i].key.ptr,
      (int) u->properties[i].value.len, u->properties[i].value.ptr);
  }

  switch (u->role) {
  case ROLE_TEACHER:
    printf("salary: %" PRIu32 "\n", u->teacher.salary);
    printf("payday: %" PRIu8 "/%" PRIu8 "\n",
      u->teacher.payday.mon, u->teacher.payday.day);
    break;
  case ROLE_STUDENT:
    printf("scores:\n");
    for (size_t i = 0; i < 10; ++i) {
      printf("  - %" PRIu8 "\n", u->student.scores[i]);
    }
    printf("absents: %" PRIu8 "\n", u->student.absents);
    break;
  }
}

int main(void) {
# define str_(v) { .ptr = (uint8_t*) (v), .len = strlen(v), }
  user_t src = {
    .name = str_("neko"),
    .age  = 4,
    .sex  = SEX_CAT,
    .role = ROLE_STUDENT,

    .property_count = 3,
    .properties = (strpair_t[]) {
      { .key = str_("like"),  .value = str_("rat"), },
      { .key = str_("hate"),  .value = str_("dog"), },
      { .key = str_("empty"), .value = str_(""), },
    },

    .student = {
      .scores  = { 100, 90, 80, 70, 60, 50, 40, 30, 20, 10, },
      .absents = 1,
    },
  };
# undef str_

  user_pack_context_t pack_ctx   = {0};
  user_pack_context_t unpack_ctx = {0};
  user_t dst = {0};

  size_t b = 0;
  biner_result_t result = BINER_RESULT_CONTINUE;
  while (result == BINER_RESULT_CONTINUE) {
    uint8_t c;
    result = user_pack(&pack_ctx, &src, &c);
    if (result >= BINER_RESULT_ERROR_THRESHOULD_) {
      fprintf(stderr, "unexpected error happened\n");
      return EXIT_FAILURE;
    }

    if (user_unpack(&unpack_ctx, &dst, c) != result) {
      fprintf(stderr, "fail at the %zu byte: ", b);
      if (result == BINER_RESULT_COMPLETED) {
        fprintf(stderr, "pack function finsihed but unpack not\n");
      } else {
        fprintf(stderr, "unpack function finsihed but pack not\n");
      }
      return EXIT_FAILURE;
    }
    ++b;
  }

  printf("[src]\n");
  print_user_(&src);
  printf("[dst]\n");
  print_user_(&dst);
  return EXIT_SUCCESS;
}
