#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
# define unpack_Nbit_(N)  \
    static inline void biner_unpack_l##N(void* v, uint8_t c, size_t s) {  \
      assert(s < N/8);  \
      *((uint8_t*) v + s) = c;  \
    } \
    static inline void biner_unpack_b##N(void* v, uint8_t c, size_t s) {  \
      assert(s < N/8);  \
      biner_unpack_l##N(v, c, N/8-s-1);  \
    }
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
# define unpack_Nbit_(N)  \
    static inline void biner_unpack_b##N(void* v, uint8_t c, size_t s) {  \
      assert(s < N/8);  \
      *((uint8_t*) v + s) = c;  \
    } \
    static inline void biner_unpack_l##N(void* v, uint8_t c, size_t s) {  \
      assert(s < N/8);  \
      biner_unpack_b##N(v, c, N/8-s-1);  \
    }
#else
# error "byte order unknown"
#endif

unpack_Nbit_(8)
unpack_Nbit_(16)
unpack_Nbit_(32)
unpack_Nbit_(64)

#undef unpack_Nbit_
