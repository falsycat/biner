#pragma once

typedef enum biner_result_t {
  BINER_RESULT_COMPLETED,
  BINER_RESULT_CONTINUE,

  BINER_RESULT_ERROR_THRESHOULD_,

  BINER_RESULT_MEMORY_ERROR,
} biner_result_t;
