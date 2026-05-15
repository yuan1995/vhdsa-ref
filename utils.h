
#ifndef VHDSA_UTILS_H
#define VHDSA_UTILS_H

#include <stdlib.h>
#include <stdint.h>
#include "types.h"
#include <stdbool.h>

#include "compat.h"
#include "macros.h"
#include "instances.h"

VHDSA_BEGIN_C_DECL

static inline void xor_u8_array(const u8* a, const u8* b, u8* out, size_t len) {
  for (size_t i = 0; i < len; i++) {
    out[i] = a[i] ^ b[i];
  }
}

static inline void masked_xor_u8_array(const u8* a, const u8* b, u8* out,
                                       u8 mask_bit, size_t len) {
  u8 mask = -(mask_bit & 1);
  for (size_t i = 0; i < len; i++) {
    out[i] = a[i] ^ (b[i] & mask);
  }
}

#define get_bit(value, index) (((value) >> (index)) & 1)
#define set_bit(value, index) ((value) << (index))
#define ptr_get_bit(value, index) (((value)[(index) / 8] >> ((index) % 8)) & 1)
#define ptr_set_bit(dst, index, value)                                                             \
  do {                                                                                             \
    const unsigned int ptr_set_bit_index_mod_8 = (index) % 8;                                      \
    (dst)[(index) / 8] = ((dst)[(index) / 8] & ~(1 << ptr_set_bit_index_mod_8)) |                  \
                         ((value) << ptr_set_bit_index_mod_8);                                     \
  } while (0)

// DecodeAllChall_3
bool decode_all_chall_3(u16* decoded_chall, const u8* chall,
                        const vhdsa_paramset_t* params);

VHDSA_END_C_DECL

#endif
