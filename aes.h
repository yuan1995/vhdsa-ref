#ifndef VHDSA_AES_H
#define VHDSA_AES_H
#include "macros.h"
#include "instances.h"
#include "types.h"
#include <stdint.h>
#include <stdlib.h>
VHDSA_BEGIN_C_DECL
#define AES_ROUNDS_128 10
#define AES_ROUNDS_192 12
#define AES_ROUNDS_256 14
#define AES_MAX_ROUNDS 14
typedef u8 aes_word_t[4];
typedef aes_word_t aes_round_key_t[8];
#define AES_NR 4
typedef aes_word_t aes_block_t[8];
typedef struct {
  aes_round_key_t round_keys[AES_MAX_ROUNDS + 1];
} aes_round_keys_t;
void aes128_init_round_keys(aes_round_keys_t* round_key, const u8* key);
void aes192_init_round_keys(aes_round_keys_t* round_key, const u8* key);
void aes256_init_round_keys(aes_round_keys_t* round_key, const u8* key);
void aes128_encrypt_block(const aes_round_keys_t* key, const u8* plaintext,
                          u8* ciphertext);
void aes192_encrypt_block(const aes_round_keys_t* key, const u8* plaintext,
                          u8* ciphertext);
void aes256_encrypt_block(const aes_round_keys_t* key, const u8* plaintext,
                          u8* ciphertext);
#if defined(VHDSA_TESTS)
void aes_increment_iv(u8* iv);
u8 invnorm(u8 in);
#endif
void aes_extend_witness(u8* w, const u8* key, const u8* in,
                        const vhdsa_paramset_t* params);
void expand_key(aes_round_keys_t* round_keys, const u8* key, unsigned int key_words,
                unsigned int block_words, unsigned int num_rounds);
void prg(const u8* key, const u8* iv, u32 tweak, u8* out, unsigned int bits,
         size_t outlen);
void prg_2_lambda(const u8* key, const u8* iv, u32 tweak, u8* out,
                  unsigned int bits);
void prg_4_lambda(const u8* key, const u8* iv, u32 tweak, u8* out,
                  unsigned int bits);
VHDSA_END_C_DECL
#endif 