#ifndef VHDSA_RANDOM_ORACLE_H
#define VHDSA_RANDOM_ORACLE_H

#include "hash_shake.h"
#include "macros.h"
#include "types.h"

VHDSA_BEGIN_C_DECL

// H_0的实现

typedef hash_context H0_context_t;

void H0_init(H0_context_t* H0_ctx, unsigned int security_param);
void H0_update(H0_context_t* H0_ctx, const u8* src, size_t len);
void H0_final(H0_context_t* H0_ctx, u8* seed, size_t seed_len, u8* commitment,
              size_t commitment_len);
void H0_final_for_squeeze(H0_context_t* H0_ctx);
// 仅用于H0_final_for_squeeze
void H0_squeeze(H0_context_t* H0_ctx, u8* dst, size_t len);
void H0_clear(H0_context_t* H0_ctx);

typedef hash_context_x4 H0_context_x4_t;

void H0_x4_init(H0_context_x4_t* H0_ctx, unsigned int security_param);
void H0_x4_update(H0_context_x4_t* H0_ctx, const u8* src0, const u8* src1,
                  const u8* src2, const u8* src3, size_t len);
void H0_x4_final(H0_context_x4_t* ctx, u8* seed0, u8* seed1, u8* seed2,
                 u8* seed3, size_t seed_len, u8* commitment0, u8* commitment1,
                 u8* commitment2, u8* commitment3, size_t commitment_len);

// H_1的实现

typedef hash_context H1_context_t;

void H1_init(H1_context_t* H1_ctx, unsigned int security_param);
void H1_update(H1_context_t* H1_ctx, const u8* src, size_t len);
void H1_final(H1_context_t* H1_ctx, u8* digest, size_t len);

// H_2的实现

typedef hash_context H2_context_t;

void H2_init(H2_context_t* ctx, unsigned int security_param);
void H2_copy(H2_context_t* new_ctx, const H2_context_t* ctx);
void H2_update(H2_context_t* ctx, const u8* src, size_t len);
void H2_update_u32_le(H2_context_t* ctx, u32 v);
void H2_0_final(H2_context_t* ctx, u8* digest, size_t len);
void H2_1_final(H2_context_t* ctx, u8* digest, size_t len);
void H2_2_final(H2_context_t* ctx, u8* digest, size_t len);
void H2_3_final(H2_context_t* ctx, u8* digest, size_t len);

// H_3的实现

typedef hash_context H3_context_t;

void H3_init(H3_context_t* ctx, unsigned int security_param);
void H3_update(H3_context_t* ctx, const u8* src, size_t len);
void H3_final(H3_context_t* ctx, u8* digest, size_t len, u8* iv);

// H_4的实现

typedef hash_context H4_context_t;

void H4_init(H4_context_t* ctx, unsigned int security_param);
void H4_update(H4_context_t* ctx, const u8* pre_iv);
void H4_final(H4_context_t* ctx, u8* iv);

VHDSA_END_C_DECL

#endif
