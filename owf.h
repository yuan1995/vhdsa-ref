#ifndef OWF_H
#define OWF_H

#include "macros.h"

#include <stdint.h>
#include "types.h"

VHDSA_BEGIN_C_DECL

void owf_128(const u8* key, const u8* input, u8* output);
void owf_192(const u8* key, const u8* input, u8* output);
void owf_256(const u8* key, const u8* input, u8* output);

#define vhdsa_128s_owf owf_128
#define vhdsa_128f_owf owf_128
#define vhdsa_192s_owf owf_192
#define vhdsa_192f_owf owf_192
#define vhdsa_256s_owf owf_256
#define vhdsa_256f_owf owf_256

VHDSA_END_C_DECL

#endif
