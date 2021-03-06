#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
# define pack_Nbit_(N)  \
    static inline void biner_pack_l##N(const void* v, uint8_t* c, size_t s) {  \
      assert(s < N/8);  \
      *c = *((uint8_t*) v + s);  \
    } \
    static inline void biner_pack_b##N(const void* v, uint8_t* c, size_t s) {  \
      assert(s < N/8);  \
      biner_pack_l##N(v, c, N/8-s-1);  \
    }
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
# define pack_Nbit_(N)  \
    static inline void biner_pack_b##N(const void* v, uint8_t* c, size_t s) {  \
      assert(s < N/8);  \
      *c = *((uint8_t*) v + s);  \
    } \
    static inline void biner_pack_l##N(const void* v, uint8_t* c, size_t s) {  \
      assert(s < N/8);  \
      biner_pack_b##N(v, c, N/8-s-1);  \
    }
#else
# error "byte order unknown"
#endif

pack_Nbit_(8)
pack_Nbit_(16)
pack_Nbit_(32)
pack_Nbit_(64)

#undef pack_Nbit_
