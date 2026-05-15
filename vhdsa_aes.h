
#ifndef VHDSA_VHDSA_AES_H
#define VHDSA_VHDSA_AES_H

#include <stdint.h>
#include "types.h"

#include "instances.h"
#include "aes.h"

VHDSA_BEGIN_C_DECL

void aes_128_prover(u8* a0_tilde, u8* a1_tilde, u8* a2_tilde, const u8* w,
                    const u8* u, u8** V, const u8* owf_in, const u8* owf_out,
                    const u8* chall_2, const vhdsa_paramset_t* params);

void aes_192_prover(u8* a0_tilde, u8* a1_tilde, u8* a2_tilde, const u8* w,
                    const u8* u, u8** V, const u8* owf_in, const u8* owf_out,
                    const u8* chall_2, const vhdsa_paramset_t* params);

void aes_256_prover(u8* a0_tilde, u8* a1_tilde, u8* a2_tilde, const u8* w,
                    const u8* u, u8** V, const u8* owf_in, const u8* owf_out,
                    const u8* chall_2, const vhdsa_paramset_t* params);

void aes_128_verifier(u8* a0_tilde, const u8* d, u8** Q, const u8* owf_in,
                      const u8* owf_out, const u8* chall_2, const u8* chall_3,
                      const u8* a1_tilde, const u8* a2_tilde,
                      const vhdsa_paramset_t* params);

void aes_192_verifier(u8* a0_tilde, const u8* d, u8** Q, const u8* owf_in,
                      const u8* owf_out, const u8* chall_2, const u8* chall_3,
                      const u8* a1_tilde, const u8* a2_tilde,
                      const vhdsa_paramset_t* params);

void aes_256_verifier(u8* a0_tilde, const u8* d, u8** Q, const u8* owf_in,
                      const u8* owf_out, const u8* chall_2, const u8* chall_3,
                      const u8* a1_tilde, const u8* a2_tilde,
                      const vhdsa_paramset_t* params);

VHDSA_END_C_DECL

#endif
