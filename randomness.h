
#ifndef RANDOMNESS_H
#define RANDOMNESS_H

#include "macros.h"

#include <stddef.h>
#include <stdint.h>
#include "types.h"

VHDSA_BEGIN_C_DECL

int rand_bytes(u8* dst, size_t num_bytes);

VHDSA_END_C_DECL

#endif
