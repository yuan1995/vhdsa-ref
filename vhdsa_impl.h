
#ifndef VHDSA_IMPL_H
#define VHDSA_IMPL_H

#include <stdint.h>
#include "types.h"
#include <stddef.h>

#include "instances.h"

void vhdsa_sign(u8* sig, const u8* msg, size_t msglen, const u8* owf_key,
                const u8* owf_input, const u8* owf_output, const u8* witness,
                const u8* rho, size_t rholen, const vhdsa_paramset_t* params);

int vhdsa_verify(const u8* msg, size_t msglen, const u8* sig, const u8* owf_input,
                 const u8* owf_output, const vhdsa_paramset_t* params);

#endif
