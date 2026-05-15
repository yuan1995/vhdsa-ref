#include <stdio.h>
#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include "aes.h"

#include "cpu.h"
#include "fields.h"
#include "compat.h"
#include "utils.h"

#if defined(HAVE_AESNI)
#include "aesni.h"
#endif
#if defined(HAVE_OPENSSL)
#include <openssl/evp.h>
#elif defined(_WIN32)
#include <windows.h>
#endif
#include <string.h>

#define KEY_WORDS_128 4
#define KEY_WORDS_192 6
#define KEY_WORDS_256 8

#define AES_BLOCK_WORDS 4

static const bf8_t round_constants[30] = {
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a,
    0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91,
};

static bf8_t compute_sbox(bf8_t in) {
  bf8_t t  = bf8_inv(in);
  bf8_t t0 = set_bit(parity8(t & (1 | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7))), 0);
  t0 ^= set_bit(parity8(t & (1 | (1 << 1) | (1 << 5) | (1 << 6) | (1 << 7))), 1);
  t0 ^= set_bit(parity8(t & (1 | (1 << 1) | (1 << 2) | (1 << 6) | (1 << 7))), 2);
  t0 ^= set_bit(parity8(t & (1 | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 7))), 3);
  t0 ^= set_bit(parity8(t & (1 | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4))), 4);
  t0 ^= set_bit(parity8(t & ((1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5))), 5);
  t0 ^= set_bit(parity8(t & ((1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6))), 6);
  t0 ^= set_bit(parity8(t & ((1 << 3) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7))), 7);
  return t0 ^ (1 | (1 << 1) | (1 << 5) | (1 << 6));
}

#if defined(VHDSA_TESTS)
void aes_increment_iv(u8* iv)
#else
static inline void aes_increment_iv(u8* iv)
#endif
{
  u32 iv0;
  memcpy(&iv0, iv, sizeof(u32));
  iv0 = htole32(le32toh(iv0) + 1);
  memcpy(iv, &iv0, sizeof(u32));
}

// ## AES ##
// Round Functions
static void add_round_key(unsigned int round, aes_block_t state, const aes_round_keys_t* round_key,
                          unsigned int block_words) {
  for (unsigned int c = 0; c < block_words; c++) {
    xor_u8_array(&state[c][0], &round_key->round_keys[round][c][0], &state[c][0], AES_NR);
  }
}

static void sub_bytes(aes_block_t state, unsigned int block_words) {
  for (unsigned int c = 0; c < block_words; c++) {
    for (unsigned int r = 0; r < AES_NR; r++) {
      state[c][r] = compute_sbox(state[c][r]);
    }
  }
}

static void shift_row(aes_block_t state, unsigned int block_words) {
  aes_block_t new_state;
  switch (block_words) {
  case 4:
  case 6:
    for (unsigned int i = 0; i < block_words; ++i) {
      new_state[i][0] = state[i][0];
      new_state[i][1] = state[(i + 1) % block_words][1];
      new_state[i][2] = state[(i + 2) % block_words][2];
      new_state[i][3] = state[(i + 3) % block_words][3];
    }
    break;
  case 8:
    for (unsigned int i = 0; i < block_words; i++) {
      new_state[i][0] = state[i][0];
      new_state[i][1] = state[(i + 1) % 8][1];
      new_state[i][2] = state[(i + 3) % 8][2];
      new_state[i][3] = state[(i + 4) % 8][3];
    }
    break;
  }

  for (unsigned int i = 0; i < block_words; ++i) {
    memcpy(&state[i][0], &new_state[i][0], AES_NR);
  }
}

static void mix_column(aes_block_t state, unsigned int block_words) {
  for (unsigned int c = 0; c < block_words; c++) {
    bf8_t tmp[4];
    tmp[0] = bf8_mul(state[c][0], 0x02) ^ bf8_mul(state[c][1], 0x03) ^ state[c][2] ^ state[c][3];
    tmp[1] = state[c][0] ^ bf8_mul(state[c][1], 0x02) ^ bf8_mul(state[c][2], 0x03) ^ state[c][3];
    tmp[2] = state[c][0] ^ state[c][1] ^ bf8_mul(state[c][2], 0x02) ^ bf8_mul(state[c][3], 0x03);
    tmp[3] = bf8_mul(state[c][0], 0x03) ^ state[c][1] ^ state[c][2] ^ bf8_mul(state[c][3], 0x02);

    memcpy(state[c], tmp, sizeof(tmp));
  }
}

// Key Expansion functions
static void sub_words(bf8_t* words) {
  words[0] = compute_sbox(words[0]);
  words[1] = compute_sbox(words[1]);
  words[2] = compute_sbox(words[2]);
  words[3] = compute_sbox(words[3]);
}

static void rot_word(bf8_t* words) {
#if 0
  bf8_t tmp = words[0];
  words[0]  = words[1];
  words[1]  = words[2];
  words[2]  = words[3];
  words[3]  = tmp;
#else
  // in the most ideal case, this generates a simple rord instruction
  u32 w;
  memcpy(&w, words, sizeof(w));
  w = htole32(rotr32(le32toh(w), 8));
  memcpy(words, &w, sizeof(w));
#endif
}

void expand_key(aes_round_keys_t* round_keys, const u8* key, unsigned int key_words, // 扩展密钥
                unsigned int block_words, unsigned int num_rounds) {
  for (unsigned int k = 0; k < key_words; k++) {
    memcpy(round_keys->round_keys[k / block_words][k % block_words], &key[4 * k], 4);
  }

  for (unsigned int k = key_words; k < block_words * (num_rounds + 1); ++k) {
    bf8_t tmp[AES_NR];
    memcpy(tmp, round_keys->round_keys[(k - 1) / block_words][(k - 1) % block_words], sizeof(tmp));

    if (k % key_words == 0) {
      rot_word(tmp);
      sub_words(tmp);
      tmp[0] ^= round_constants[(k / key_words) - 1];
    }

    if (key_words > 6 && (k % key_words) == 4) {
      sub_words(tmp);
    }

    const unsigned int m = k - key_words;
    xor_u8_array(round_keys->round_keys[m / block_words][m % block_words], tmp,
                 round_keys->round_keys[k / block_words][k % block_words], 4);
  }
}

// Calling Functions

void aes128_init_round_keys(aes_round_keys_t* round_key, const u8* key) {
  expand_key(round_key, key, KEY_WORDS_128, AES_BLOCK_WORDS, AES_ROUNDS_128);
}

void aes192_init_round_keys(aes_round_keys_t* round_key, const u8* key) {
  expand_key(round_key, key, KEY_WORDS_192, AES_BLOCK_WORDS, AES_ROUNDS_192);
}

void aes256_init_round_keys(aes_round_keys_t* round_key, const u8* key) {
  expand_key(round_key, key, KEY_WORDS_256, AES_BLOCK_WORDS, AES_ROUNDS_256);
}

static void load_state(aes_block_t state, const u8* src, unsigned int block_words) {
  for (unsigned int i = 0; i != block_words * 4; ++i) {
    state[i / 4][i % 4] = bf8_load(&src[i]);
  }
}

ATTR_CONST static bf8_t bf_exp_238(bf8_t x) {
  // 238 == 0b11101110
  bf8_t y = bf8_square(x); // x^2
  x       = bf8_square(y); // x^4
  y       = bf8_mul(x, y);
  x       = bf8_square(x); // x^8
  y       = bf8_mul(x, y);
  x       = bf8_square(x); // x^16
  x       = bf8_square(x); // x^32
  y       = bf8_mul(x, y);
  x       = bf8_square(x); // x^64
  y       = bf8_mul(x, y);
  x       = bf8_square(x); // x^128
  return bf8_mul(x, y);
}

#if !defined(VHDSA_TESTS)
static
#endif
    u8 invnorm(u8 in) {
  // instead of computing in^(-17), we calculate in^238
  in = bf_exp_238(in);
  return set_bit(get_bit(in, 0), 0) ^ set_bit(get_bit(in, 6), 1) ^ set_bit(get_bit(in, 7), 2) ^
         set_bit(get_bit(in, 2), 3);
}

static u8* store_invnorm_state(u8* dst, aes_block_t state, unsigned int block_words) {
  for (unsigned int i = 0; i != block_words * 4; i += 2, ++dst) {
    u8 normstate_lo = invnorm(state[i / 4][i % 4]);
    u8 normstate_hi = invnorm(state[i / 4][(i + 1) % 4]);
    bf8_store(dst, (normstate_hi << 4) | normstate_lo);
  }
  return dst;
}

static u8* store_state(u8* dst, aes_block_t state, unsigned int block_words) {
  for (unsigned int i = 0; i != block_words * 4; ++i, ++dst) {
    bf8_store(dst, state[i / 4][i % 4]);
  }
  return dst;
}

static void aes_encrypt(const aes_round_keys_t* keys, aes_block_t state, unsigned int block_words,
                        unsigned int num_rounds) {
  // first round
  add_round_key(0, state, keys, block_words);

  for (unsigned int round = 1; round < num_rounds; ++round) {
    sub_bytes(state, block_words);
    shift_row(state, block_words);
    mix_column(state, block_words);
    add_round_key(round, state, keys, block_words);
  }

  // last round
  sub_bytes(state, block_words);
  shift_row(state, block_words);
  add_round_key(num_rounds, state, keys, block_words);
}

void aes128_encrypt_block(const aes_round_keys_t* key, const u8* plaintext, // 使用AES-128加密数据块
                          u8* ciphertext) {
  aes_block_t state;
  load_state(state, plaintext, AES_BLOCK_WORDS);
  aes_encrypt(key, state, AES_BLOCK_WORDS, AES_ROUNDS_128);
  store_state(ciphertext, state, AES_BLOCK_WORDS);
}

void aes192_encrypt_block(const aes_round_keys_t* key, const u8* plaintext, // 使用AES-192加密数据块
                          u8* ciphertext) {
  aes_block_t state;
  load_state(state, plaintext, AES_BLOCK_WORDS);
  aes_encrypt(key, state, AES_BLOCK_WORDS, AES_ROUNDS_192);
  store_state(ciphertext, state, AES_BLOCK_WORDS);
}

void aes256_encrypt_block(const aes_round_keys_t* key, const u8* plaintext, // 使用AES-256加密数据块
                          u8* ciphertext) {
  aes_block_t state;
  load_state(state, plaintext, AES_BLOCK_WORDS);
  aes_encrypt(key, state, AES_BLOCK_WORDS, AES_ROUNDS_256);
  store_state(ciphertext, state, AES_BLOCK_WORDS);
}

static void add_to_upper_word(u8* iv, u32 tweak) {
  u32 iv3;
  memcpy(&iv3, iv + IV_SIZE - sizeof(u32), sizeof(u32));
  iv3 = htole32(le32toh(iv3) + tweak);
  memcpy(iv + IV_SIZE - sizeof(u32), &iv3, sizeof(u32));
}

#if defined(HAVE_AESNI)
ATTR_TARGET_AESNI ATTR_ALWAYS_INLINE static inline __m128i sse2_increment_iv(__m128i iv) {
  return _mm_add_epi32(iv, _mm_set_epi32(0, 0, 0, 1));
}

ATTR_TARGET_AESNI static void prg_aesni_128(const u8* key, const u8* iv, u8* out,
                                            size_t outlen) {
  __m128i rk[AES_ROUNDS_128 + 1];
  aes128_expand_key_aesni(rk, key);

  __m128i miv = _mm_loadu_si128((const __m128i_u*)iv);
  for (size_t idx = 0; idx < outlen / IV_SIZE; idx += 1, out += IV_SIZE) {
    __m128i m = _mm_xor_si128(miv, rk[0]);
    for (unsigned int round = 1; round != AES_ROUNDS_128; ++round) {
      m = _mm_aesenc_si128(m, rk[round]);
    }
    m = _mm_aesenclast_si128(m, rk[AES_ROUNDS_128]);
    _mm_storeu_si128((__m128i_u*)out, m);
    miv = sse2_increment_iv(miv);
  }

  if (outlen % IV_SIZE) {
    __m128i m = _mm_xor_si128(miv, rk[0]);
    for (unsigned int round = 1; round != AES_ROUNDS_128; ++round) {
      m = _mm_aesenc_si128(m, rk[round]);
    }
    m = _mm_aesenclast_si128(m, rk[AES_ROUNDS_128]);

    u8 last_block[IV_SIZE];
    _mm_storeu_si128((__m128i_u*)last_block, m);

    memcpy(out, last_block, outlen % IV_SIZE);
  }
}

ATTR_TARGET_AESNI static void prg_2_aesni_128(const u8* key, const u8* iv, u8* out) {
  __m128i rk[AES_ROUNDS_128 + 1];
  aes128_expand_key_aesni(rk, key);

  __m128i temp[2];
  temp[0] = _mm_loadu_si128((const __m128i_u*)iv);
  temp[1] = sse2_increment_iv(temp[0]);
  temp[0] = _mm_xor_si128(temp[0], rk[0]);
  temp[1] = _mm_xor_si128(temp[1], rk[0]);
  for (unsigned int round = 1; round != AES_ROUNDS_128; ++round) {
    temp[0] = _mm_aesenc_si128(temp[0], rk[round]);
    temp[1] = _mm_aesenc_si128(temp[1], rk[round]);
  }
  _mm_storeu_si128((__m128i_u*)out, _mm_aesenclast_si128(temp[0], rk[AES_ROUNDS_128]));
  _mm_storeu_si128((__m128i_u*)out + 1, _mm_aesenclast_si128(temp[1], rk[AES_ROUNDS_128]));
}

ATTR_TARGET_AESNI static void prg_4_aesni_128(const u8* key, const u8* iv, u8* out) {
  __m128i rk[AES_ROUNDS_128 + 1];
  aes128_expand_key_aesni(rk, key);

  __m128i temp[4];
  temp[0] = _mm_loadu_si128((const __m128i_u*)iv);
  temp[1] = sse2_increment_iv(temp[0]);
  temp[2] = sse2_increment_iv(temp[1]);
  temp[3] = sse2_increment_iv(temp[2]);
  temp[0] = _mm_xor_si128(temp[0], rk[0]);
  temp[1] = _mm_xor_si128(temp[1], rk[0]);
  temp[2] = _mm_xor_si128(temp[2], rk[0]);
  temp[3] = _mm_xor_si128(temp[3], rk[0]);
  for (unsigned int round = 1; round != AES_ROUNDS_128; ++round) {
    temp[0] = _mm_aesenc_si128(temp[0], rk[round]);
    temp[1] = _mm_aesenc_si128(temp[1], rk[round]);
    temp[2] = _mm_aesenc_si128(temp[2], rk[round]);
    temp[3] = _mm_aesenc_si128(temp[3], rk[round]);
  }
  _mm_storeu_si128((__m128i_u*)out, _mm_aesenclast_si128(temp[0], rk[AES_ROUNDS_128]));
  _mm_storeu_si128((__m128i_u*)out + 1, _mm_aesenclast_si128(temp[1], rk[AES_ROUNDS_128]));
  _mm_storeu_si128((__m128i_u*)out + 2, _mm_aesenclast_si128(temp[2], rk[AES_ROUNDS_128]));
  _mm_storeu_si128((__m128i_u*)out + 3, _mm_aesenclast_si128(temp[3], rk[AES_ROUNDS_128]));
}

ATTR_TARGET_AESNI static void prg_aesni_192(const u8* key, u8* iv, u8* out,
                                            size_t outlen) {
  __m128i rk[AES_ROUNDS_192 + 1];
  aes192_expand_key_aesni(rk, key);

  __m128i miv = _mm_loadu_si128((const __m128i_u*)iv);
  for (size_t idx = 0; idx < outlen / IV_SIZE; idx += 1, out += IV_SIZE) {
    __m128i m = _mm_xor_si128(miv, rk[0]);
    for (unsigned int round = 1; round != AES_ROUNDS_192; ++round) {
      m = _mm_aesenc_si128(m, rk[round]);
    }
    m = _mm_aesenclast_si128(m, rk[AES_ROUNDS_192]);
    _mm_storeu_si128((__m128i_u*)out, m);
    miv = sse2_increment_iv(miv);
  }

  if (outlen % IV_SIZE) {
    __m128i m = _mm_xor_si128(miv, rk[0]);
    for (unsigned int round = 1; round != AES_ROUNDS_192; ++round) {
      m = _mm_aesenc_si128(m, rk[round]);
    }
    m = _mm_aesenclast_si128(m, rk[AES_ROUNDS_192]);

    u8 last_block[IV_SIZE];
    _mm_storeu_si128((__m128i_u*)last_block, m);

    memcpy(out, last_block, outlen % IV_SIZE);
  }
}

ATTR_TARGET_AESNI static void prg_2_aesni_192(const u8* key, u8* iv, u8* out) {
  __m128i rk[AES_ROUNDS_192 + 1];
  aes192_expand_key_aesni(rk, key);

  __m128i temp[3];
  temp[0] = _mm_loadu_si128((const __m128i_u*)iv);
  temp[1] = sse2_increment_iv(temp[0]);
  temp[2] = sse2_increment_iv(temp[1]);
  temp[0] = _mm_xor_si128(temp[0], rk[0]);
  temp[1] = _mm_xor_si128(temp[1], rk[0]);
  temp[2] = _mm_xor_si128(temp[2], rk[0]);
  for (unsigned int round = 1; round != AES_ROUNDS_192; ++round) {
    temp[0] = _mm_aesenc_si128(temp[0], rk[round]);
    temp[1] = _mm_aesenc_si128(temp[1], rk[round]);
    temp[2] = _mm_aesenc_si128(temp[2], rk[round]);
  }
  _mm_storeu_si128((__m128i_u*)out, _mm_aesenclast_si128(temp[0], rk[AES_ROUNDS_192]));
  _mm_storeu_si128((__m128i_u*)out + 1, _mm_aesenclast_si128(temp[1], rk[AES_ROUNDS_192]));
  _mm_storeu_si128((__m128i_u*)out + 2, _mm_aesenclast_si128(temp[2], rk[AES_ROUNDS_192]));
}

ATTR_TARGET_AESNI static void prg_4_aesni_192(const u8* key, u8* iv, u8* out) {
  __m128i rk[AES_ROUNDS_192 + 1];
  aes192_expand_key_aesni(rk, key);

  __m128i temp[6];
  temp[0] = _mm_loadu_si128((const __m128i_u*)iv);
  for (unsigned int i = 1; i != 6; ++i) {
    temp[i] = sse2_increment_iv(temp[i - 1]);
  }
  for (unsigned int i = 0; i != 6; ++i) {
    temp[i] = _mm_xor_si128(temp[i], rk[0]);
  }
  for (unsigned int round = 1; round != AES_ROUNDS_192; ++round) {
    for (unsigned int i = 0; i != 6; ++i) {
      temp[i] = _mm_aesenc_si128(temp[i], rk[round]);
    }
  }
  for (unsigned int i = 0; i != 6; ++i) {
    _mm_storeu_si128((__m128i_u*)out + i, _mm_aesenclast_si128(temp[i], rk[AES_ROUNDS_192]));
  }
}

ATTR_TARGET_AESNI static void prg_aesni_256(const u8* key, u8* iv, u8* out,
                                            size_t outlen) {
  __m128i rk[AES_ROUNDS_256 + 1];
  aes256_expand_key_aesni(rk, key);

  __m128i miv = _mm_loadu_si128((const __m128i_u*)iv);
  for (size_t idx = 0; idx < outlen / IV_SIZE; idx += 1, out += IV_SIZE) {
    __m128i m = _mm_xor_si128(miv, rk[0]);
    for (unsigned int round = 1; round != AES_ROUNDS_256; ++round) {
      m = _mm_aesenc_si128(m, rk[round]);
    }
    m = _mm_aesenclast_si128(m, rk[AES_ROUNDS_256]);
    _mm_storeu_si128((__m128i_u*)out, m);
    miv = sse2_increment_iv(miv);
  }

  if (outlen % IV_SIZE) {
    __m128i m = _mm_xor_si128(miv, rk[0]);
    for (unsigned int round = 1; round != AES_ROUNDS_256; ++round) {
      m = _mm_aesenc_si128(m, rk[round]);
    }
    m = _mm_aesenclast_si128(m, rk[AES_ROUNDS_256]);

    u8 last_block[IV_SIZE];
    _mm_storeu_si128((__m128i_u*)last_block, m);

    memcpy(out, last_block, outlen % IV_SIZE);
  }
}

ATTR_TARGET_AESNI static void prg_2_aesni_256(const u8* key, u8* iv, u8* out) {
  __m128i rk[AES_ROUNDS_256 + 1];
  aes256_expand_key_aesni(rk, key);

  __m128i temp[4];
  temp[0] = _mm_loadu_si128((const __m128i_u*)iv);
  for (unsigned int i = 1; i != 4; ++i) {
    temp[i] = sse2_increment_iv(temp[i - 1]);
  }
  for (unsigned int i = 0; i != 4; ++i) {
    temp[i] = _mm_xor_si128(temp[i], rk[0]);
  }
  for (unsigned int round = 1; round != AES_ROUNDS_256; ++round) {
    for (unsigned int i = 0; i != 4; ++i) {
      temp[i] = _mm_aesenc_si128(temp[i], rk[round]);
    }
  }
  for (unsigned int i = 0; i != 4; ++i) {
    _mm_storeu_si128((__m128i_u*)out + i, _mm_aesenclast_si128(temp[i], rk[AES_ROUNDS_256]));
  }
}

ATTR_TARGET_AESNI static void prg_4_aesni_256(const u8* key, u8* iv, u8* out) {
  __m128i rk[AES_ROUNDS_256 + 1];
  aes256_expand_key_aesni(rk, key);

  __m128i temp[8];
  temp[0] = _mm_loadu_si128((const __m128i_u*)iv);
  for (unsigned int i = 1; i != 8; ++i) {
    temp[i] = sse2_increment_iv(temp[i - 1]);
  }
  for (unsigned int i = 0; i != 8; ++i) {
    temp[i] = _mm_xor_si128(temp[i], rk[0]);
  }
  for (unsigned int round = 1; round != AES_ROUNDS_256; ++round) {
    for (unsigned int i = 0; i != 8; ++i) {
      temp[i] = _mm_aesenc_si128(temp[i], rk[round]);
    }
  }
  for (unsigned int i = 0; i != 8; ++i) {
    _mm_storeu_si128((__m128i_u*)out + i, _mm_aesenclast_si128(temp[i], rk[AES_ROUNDS_256]));
  }
}

#if defined(HAVE_AVX2)
ATTR_TARGET_AESNI_AVX2 static void prg_aesni_avx_128(const u8* key, const u8* iv,
                                                     u8* out, size_t outlen) {
  __m128i rk[AES_ROUNDS_128 + 1];
  aes128_expand_key_aesni_avx2(rk, key);

  __m128i miv = _mm_loadu_si128((const __m128i_u*)iv);
  for (size_t idx = 0; idx < outlen / IV_SIZE; idx += 1, out += IV_SIZE) {
    __m128i m = _mm_xor_si128(miv, rk[0]);
    for (unsigned int round = 1; round != AES_ROUNDS_128; ++round) {
      m = _mm_aesenc_si128(m, rk[round]);
    }
    m = _mm_aesenclast_si128(m, rk[AES_ROUNDS_128]);
    _mm_storeu_si128((__m128i_u*)out, m);
    miv = sse2_increment_iv(miv);
  }

  if (outlen % IV_SIZE) {
    __m128i m = _mm_xor_si128(miv, rk[0]);
    for (unsigned int round = 1; round != AES_ROUNDS_128; ++round) {
      m = _mm_aesenc_si128(m, rk[round]);
    }
    m = _mm_aesenclast_si128(m, rk[AES_ROUNDS_128]);

    u8 last_block[IV_SIZE];
    _mm_storeu_si128((__m128i_u*)last_block, m);

    memcpy(out, last_block, outlen % IV_SIZE);
  }
}

ATTR_TARGET_AESNI_AVX2 static void prg_2_aesni_avx_128(const u8* key, const u8* iv,
                                                       u8* out) {
  __m128i rk[AES_ROUNDS_128 + 1];
  aes128_expand_key_aesni_avx2(rk, key);
  __m128i temp[2];
  temp[0] = _mm_loadu_si128((const __m128i_u*)iv);
  
  temp[0] = _mm_xor_si128(temp[0], rk[0]);
  
  for (unsigned int round = 1; round != AES_ROUNDS_128; ++round) {
    temp[0] = _mm_aesenc_si128(temp[0], rk[round]);
  }
  temp[0] = _mm_aesenclast_si128(temp[0], rk[AES_ROUNDS_128]);
  temp[1] = _mm_xor_si128(temp[0], _mm_loadu_si128((const __m128i_u*)key));
  
  _mm_storeu_si128((__m128i_u*)out, temp[0]);
  _mm_storeu_si128((__m128i_u*)(out + IV_SIZE), temp[1]);
}

ATTR_TARGET_AESNI_AVX2 static void prg_4_aesni_avx_128(const u8* key, const u8* iv,
                                                       u8* out) {
  __m128i rk[AES_ROUNDS_128 + 1];
  aes128_expand_key_aesni_avx2(rk, key);

  __m128i temp[4];
  temp[0] = _mm_loadu_si128((const __m128i_u*)iv);
  temp[1] = sse2_increment_iv(temp[0]);
  temp[2] = sse2_increment_iv(temp[1]);
  temp[3] = sse2_increment_iv(temp[2]);
  temp[0] = _mm_xor_si128(temp[0], rk[0]);
  temp[1] = _mm_xor_si128(temp[1], rk[0]);
  temp[2] = _mm_xor_si128(temp[2], rk[0]);
  temp[3] = _mm_xor_si128(temp[3], rk[0]);
  for (unsigned int round = 1; round != AES_ROUNDS_128; ++round) {
    temp[0] = _mm_aesenc_si128(temp[0], rk[round]);
    temp[1] = _mm_aesenc_si128(temp[1], rk[round]);
    temp[2] = _mm_aesenc_si128(temp[2], rk[round]);
    temp[3] = _mm_aesenc_si128(temp[3], rk[round]);
  }
  _mm_storeu_si128((__m128i_u*)out, _mm_aesenclast_si128(temp[0], rk[AES_ROUNDS_128]));
  _mm_storeu_si128((__m128i_u*)out + 1, _mm_aesenclast_si128(temp[1], rk[AES_ROUNDS_128]));
  _mm_storeu_si128((__m128i_u*)out + 2, _mm_aesenclast_si128(temp[2], rk[AES_ROUNDS_128]));
  _mm_storeu_si128((__m128i_u*)out + 3, _mm_aesenclast_si128(temp[3], rk[AES_ROUNDS_128]));
}

ATTR_TARGET_AESNI_AVX2 static void prg_aesni_avx_192(const u8* key, u8* iv, u8* out,
                                                     size_t outlen) {
  u8 key_256[32] = {0}; // 创建256位(32字节)的密钥空间
  memcpy(key_256, key, 24); // 复制原始192位密钥
  __m128i rk[AES_ROUNDS_256 + 1];
  aes256_expand_key_aesni_avx2(rk, key_256);

  __m128i miv = _mm_loadu_si128((const __m128i_u*)iv);
  for (size_t idx = 0; idx < outlen / IV_SIZE; idx += 1, out += IV_SIZE) {
    __m128i m = _mm_xor_si128(miv, rk[0]);
    for (unsigned int round = 1; round != AES_ROUNDS_256; ++round) {
      m = _mm_aesenc_si128(m, rk[round]);
    }
    m = _mm_aesenclast_si128(m, rk[AES_ROUNDS_256]);
    _mm_storeu_si128((__m128i_u*)out, m);
    miv = sse2_increment_iv(miv);
  }

  if (outlen % IV_SIZE) {
    __m128i m = _mm_xor_si128(miv, rk[0]);
    for (unsigned int round = 1; round != AES_ROUNDS_256; ++round) {
      m = _mm_aesenc_si128(m, rk[round]);
    }
    m = _mm_aesenclast_si128(m, rk[AES_ROUNDS_256]);

    u8 last_block[IV_SIZE];
    _mm_storeu_si128((__m128i_u*)last_block, m);

    memcpy(out, last_block, outlen % IV_SIZE);
  }
}

ATTR_TARGET_AESNI_AVX2 static void prg_2_aesni_avx_192(const u8* key, u8* iv,
                                                       u8* out) {
  u8 key_256[32] = {0}; // 创建256位(32字节)的密钥空间
  memcpy(key_256, key, 24); // 复制原始192位密钥
  __m128i rk[AES_ROUNDS_256 + 1];
  aes256_expand_key_aesni_avx2(rk, key_256);

  __m128i temp[3];
  temp[0] = _mm_loadu_si128((const __m128i_u*)iv);
  temp[1] = sse2_increment_iv(temp[0]);
  
  temp[0] = _mm_xor_si128(temp[0], rk[0]);
  temp[1] = _mm_xor_si128(temp[1], rk[0]);
  
  for (unsigned int round = 1; round != AES_ROUNDS_256; ++round) {
    temp[0] = _mm_aesenc_si128(temp[0], rk[round]);
    temp[1] = _mm_aesenc_si128(temp[1], rk[round]);
    
  }
  temp[0] = _mm_aesenclast_si128(temp[0], rk[AES_ROUNDS_256]);
  temp[1] = _mm_aesenclast_si128(temp[1], rk[AES_ROUNDS_256]);

    // 计算 temp[0] 与 key 的前 128 位异或
  __m128i temp_2 = _mm_xor_si128(temp[0], _mm_loadu_si128((const __m128i_u*)key));
  
    // 将 temp_2 分解为前 64 位和后 64 位
  u64 temp2_64_127 = _mm_cvtsi128_si64(_mm_srli_si128(temp_2, 8));
  u64 temp2_0_63 = _mm_cvtsi128_si64(temp_2);
  
  // 计算 temp[1] 的低 64 位（0-63位）与 key 的后 64 位异或
  u64 temp1_0_63 = _mm_cvtsi128_si64(temp[1]);
  u64 key_last64;
  memcpy(&key_last64, key + 16, sizeof(u64));
  u64 result64 = temp1_0_63 ^ key_last64;
  
  // 将 temp_2 的前 64 位放到 temp[1] 的后 64 位中
  temp[1] = _mm_unpacklo_epi64(temp[1], _mm_cvtsi64_si128(temp2_0_63));
  
  // 创建 temp[2]，将 temp_2 的低 64 位与 result64 拼接
  temp[2] = _mm_set_epi64x(result64, temp2_64_127);
  
  _mm_storeu_si128((__m128i_u*)out, temp[0]);
  _mm_storeu_si128((__m128i_u*)out + 1, temp[1]);
  _mm_storeu_si128((__m128i_u*)out + 2, temp[2]);
}


ATTR_TARGET_AESNI_AVX2 static void prg_4_aesni_avx_192(const u8* key, u8* iv,
                                                       u8* out) {
  u8 key_256[32] = {0}; // 创建256位(32字节)的密钥空间
  memcpy(key_256, key, 24); // 复制原始192位密钥
  __m128i rk[AES_ROUNDS_256 + 1];
  aes256_expand_key_aesni(rk, key_256);

  __m128i temp[6];
  temp[0] = _mm_loadu_si128((const __m128i_u*)iv);
  for (unsigned int i = 1; i != 6; ++i) {
    temp[i] = sse2_increment_iv(temp[i - 1]);
  }
  for (unsigned int i = 0; i != 6; ++i) {
    temp[i] = _mm_xor_si128(temp[i], rk[0]);
  }
  for (unsigned int round = 1; round != AES_ROUNDS_256; ++round) {
    for (unsigned int i = 0; i != 6; ++i) {
      temp[i] = _mm_aesenc_si128(temp[i], rk[round]);
    }
  }
  for (unsigned int i = 0; i != 6; ++i) {
    _mm_storeu_si128((__m128i_u*)out + i, _mm_aesenclast_si128(temp[i], rk[AES_ROUNDS_256]));
  }
}

ATTR_TARGET_AESNI_AVX2 static void prg_aesni_avx_256(const u8* key, u8* iv, u8* out,
                                                     size_t outlen) {
  __m128i rk[AES_ROUNDS_256 + 1];
  aes256_expand_key_aesni_avx2(rk, key);

  __m128i miv = _mm_loadu_si128((const __m128i_u*)iv);
  for (size_t idx = 0; idx < outlen / IV_SIZE; idx += 1, out += IV_SIZE) {
    __m128i m = _mm_xor_si128(miv, rk[0]);
    for (unsigned int round = 1; round != AES_ROUNDS_256; ++round) {
      m = _mm_aesenc_si128(m, rk[round]);
    }
    m = _mm_aesenclast_si128(m, rk[AES_ROUNDS_256]);
    _mm_storeu_si128((__m128i_u*)out, m);
    miv = sse2_increment_iv(miv);
  }

  if (outlen % IV_SIZE) {
    __m128i m = _mm_xor_si128(miv, rk[0]);
    for (unsigned int round = 1; round != AES_ROUNDS_256; ++round) {
      m = _mm_aesenc_si128(m, rk[round]);
    }
    m = _mm_aesenclast_si128(m, rk[AES_ROUNDS_256]);

    u8 last_block[IV_SIZE];
    _mm_storeu_si128((__m128i_u*)last_block, m);

    memcpy(out, last_block, outlen % IV_SIZE);
  }
}

ATTR_TARGET_AESNI_AVX2 static void prg_2_aesni_avx_256(const u8* key, u8* iv,
                                                       u8* out) {
  __m128i rk[AES_ROUNDS_256 + 1];
  aes256_expand_key_aesni(rk, key);

  __m128i temp[4];
  temp[0] = _mm_loadu_si128((const __m128i_u*)iv);
  for (unsigned int i = 1; i != 2; ++i) {
    temp[i] = sse2_increment_iv(temp[i - 1]);
  }
  for (unsigned int i = 0; i != 2; ++i) {
    temp[i] = _mm_xor_si128(temp[i], rk[0]);
  }
  for (unsigned int round = 1; round != AES_ROUNDS_256; ++round) {
    for (unsigned int i = 0; i != 2; ++i) {
      temp[i] = _mm_aesenc_si128(temp[i], rk[round]);
    }
  }
  temp[0] = _mm_aesenclast_si128(temp[0], rk[AES_ROUNDS_256]);
  temp[1] = _mm_aesenclast_si128(temp[1], rk[AES_ROUNDS_256]);

  temp[2] = _mm_xor_si128(temp[0], _mm_loadu_si128((const __m128i_u*)key));
  temp[3] = _mm_xor_si128(temp[1], _mm_loadu_si128((const __m128i_u*)(key + 16)));
  for (unsigned int i = 0; i != 4; ++i) {
    _mm_storeu_si128((__m128i_u*)out + i, temp[i]);
  }
}

ATTR_TARGET_AESNI_AVX2 static void prg_4_aesni_avx_256(const u8* key, u8* iv,
                                                       u8* out) {
  __m128i rk[AES_ROUNDS_256 + 1];
  aes256_expand_key_aesni(rk, key);

  __m128i temp[8];
  temp[0] = _mm_loadu_si128((const __m128i_u*)iv);
  for (unsigned int i = 1; i != 8; ++i) {
    temp[i] = sse2_increment_iv(temp[i - 1]);
  }
  for (unsigned int i = 0; i != 8; ++i) {
    temp[i] = _mm_xor_si128(temp[i], rk[0]);
  }
  for (unsigned int round = 1; round != AES_ROUNDS_256; ++round) {
    for (unsigned int i = 0; i != 8; ++i) {
      temp[i] = _mm_aesenc_si128(temp[i], rk[round]);
    }
  }
  for (unsigned int i = 0; i != 8; ++i) {
    _mm_storeu_si128((__m128i_u*)out + i, _mm_aesenclast_si128(temp[i], rk[AES_ROUNDS_256]));
  }
}
#endif
#endif

static void generic_prg(const u8* key, u8* internal_iv, u8* out, unsigned int seclvl,
                        size_t outlen) {
#if defined(HAVE_OPENSSL)
  const EVP_CIPHER* cipher;
  switch (seclvl) {
  case 256:
    cipher = EVP_aes_256_ecb();
    break;
  case 192:
    cipher = EVP_aes_192_ecb();
    break;
  default:
    cipher = EVP_aes_128_ecb();
    break;
  }
  assert(cipher);

  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  assert(ctx);

  EVP_EncryptInit_ex(ctx, cipher, NULL, key, NULL);

  int len = 0;
  for (size_t idx = 0; idx < outlen / IV_SIZE; idx += 1, out += IV_SIZE) {
    int ret = EVP_EncryptUpdate(ctx, out, &len, internal_iv, IV_SIZE);
    assert(ret == 1);
    assert(len == (int)IV_SIZE);
    (void)ret;
    (void)len;
    aes_increment_iv(internal_iv);
  }
  if (outlen % IV_SIZE) {
    u8 last_block[IV_SIZE];
    int ret = EVP_EncryptUpdate(ctx, last_block, &len, internal_iv, IV_SIZE);
    assert(ret == 1);
    assert(len == (int)IV_SIZE);
    (void)ret;
    (void)len;
    memcpy(out, last_block, outlen % IV_SIZE);
  }
  EVP_CIPHER_CTX_free(ctx);
#elif defined(_WIN32)
  BCRYPT_ALG_HANDLE aes_handle = NULL;
  BCRYPT_KEY_HANDLE key_handle = NULL;

  NTSTATUS ret = BCryptOpenAlgorithmProvider(&aes_handle, BCRYPT_AES_ALGORITHM, NULL, 0);
  assert(BCRYPT_SUCCESS(ret));
  ret = BCryptGenerateSymmetricKey(aes_handle, &key_handle, NULL, 0, (PUCHAR)key, seclvl / 8, 0);
  assert(BCRYPT_SUCCESS(ret));

  ret = BCryptSetProperty(aes_handle, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_ECB,
                          (ULONG)((wcslen(BCRYPT_CHAIN_MODE_ECB) + 1) * sizeof(wchar_t)), 0);
  assert(BCRYPT_SUCCESS(ret));

  for (size_t idx = 0; idx < outlen / IV_SIZE; idx += 1, out += IV_SIZE) {
    ULONG len = 0;
    ret = BCryptEncrypt(key_handle, internal_iv, IV_SIZE, NULL, NULL, 0, out, IV_SIZE, &len, 0);
    assert(BCRYPT_SUCCESS(ret));
    assert(len == (ULONG)IV_SIZE);
    (void)ret;
    (void)len;
    aes_increment_iv(internal_iv);
  }
  if (outlen % IV_SIZE) {
    u8 last_block[IV_SIZE];
    ULONG len = 0;
    ret = BCryptEncrypt(key_handle, internal_iv, IV_SIZE, NULL, NULL, 0, last_block, IV_SIZE, &len,
                        0);
    assert(BCRYPT_SUCCESS(ret));
    assert(len == (ULONG)IV_SIZE);
    (void)ret;
    (void)len;
    memcpy(out, last_block, outlen % IV_SIZE);
  }

  BCryptDestroyKey(key_handle);
  BCryptCloseAlgorithmProvider(aes_handle, 0);
#else
  aes_round_keys_t round_key;

  switch (seclvl) {
  case 256:
    aes256_init_round_keys(&round_key, key);
    size_t count_total_256 = outlen/64;
    for(size_t count = 0; count < count_total_256; count++){
      aes_block_t state[4];
      load_state(state[0], internal_iv, AES_BLOCK_WORDS);
      aes_encrypt(&round_key, state[0], AES_BLOCK_WORDS, AES_ROUNDS_256);
      store_state(out, state[0], AES_BLOCK_WORDS);
      aes_increment_iv(internal_iv);
      out += 16;
      load_state(state[1], internal_iv, AES_BLOCK_WORDS);
      aes_encrypt(&round_key, state[1], AES_BLOCK_WORDS, AES_ROUNDS_256);
      store_state(out, state[1], AES_BLOCK_WORDS);
      out += 16;
      // 将state[0]与key的前16字节进行异或，并将结果存储在state[2]中
      for (size_t i = 0; i < AES_BLOCK_WORDS; i++) {
        for (size_t j = 0; j < 4; j++) {
          state[2][i][j] = state[0][i][j] ^ key[(i * 4 + j) % 16];
        }
      }
      // 将异或结果存储到输出缓冲区
      store_state(out, state[2], AES_BLOCK_WORDS);
      out += 16;
      
      // 将state[1]与key的16-32字节进行异或，并将结果存储在state[3]中
      for (size_t i = 0; i < AES_BLOCK_WORDS; i++) {
        for (size_t j = 0; j < 4; j++) {
          state[3][i][j] = state[1][i][j] ^ key[16 + (i * 4 + j) % 16];
        }
      }
      // 将异或结果存储到输出缓冲区
      store_state(out, state[3], AES_BLOCK_WORDS);
      out += 16;
      outlen -= 64;
    }
    if (outlen) {
      aes_block_t state;
      load_state(state, internal_iv, AES_BLOCK_WORDS);
      aes_encrypt(&round_key, state, AES_BLOCK_WORDS, AES_ROUNDS_256);
      u8 tmp[16];
      store_state(tmp, state, AES_BLOCK_WORDS);
      memcpy(out, tmp, outlen);
    }
    return;
  case 192:
    {
      u8 key_256[32] = {0}; // 创建256位(32字节)的密钥空间
      memcpy(key_256, key, 24); // 复制原始192位密钥
      // 剩余8字节已在初始化时用0填充
      aes256_init_round_keys(&round_key, key_256);
    }
    size_t count_total_192 = outlen/48;
    for(size_t count = 0; count < count_total_192; count++){
      aes_block_t state[2];
      
      load_state(state[0], internal_iv, AES_BLOCK_WORDS);
      aes_encrypt(&round_key, state[0], AES_BLOCK_WORDS, AES_ROUNDS_256);
      store_state(out, state[0], AES_BLOCK_WORDS);
      aes_increment_iv(internal_iv);
      out += 16;

      load_state(state[1], internal_iv, AES_BLOCK_WORDS);
      aes_encrypt(&round_key, state[1], AES_BLOCK_WORDS, AES_ROUNDS_256);
      store_state(out, state[1], AES_BLOCK_WORDS/2);
      out += 8;
      // 将state[0]与key的前16字节进行异或
      // 并将结果存储到输出缓冲区
      u8 temp_out[16];
      for (size_t i = 0; i < AES_BLOCK_WORDS; i++) {
        for (size_t j = 0; j < 4; j++) {
          temp_out[i*4 + j] = state[0][i][j] ^ key[(i * 4 + j) % 16];
        }
      }
      memcpy(out, temp_out, 16); // 直接复制异或后的结果到输出缓冲区
      out += 16;
      // 将state[1]的前8字节与key的末尾8字节进行异或
      u8 temp_bytes[8];
      // 将state[1]的前8字节提取到临时数组
      for (size_t i = 0; i < 2; i++) { // AES_BLOCK_WORDS/2 = 2
        for (size_t j = 0; j < 4; j++) {
          temp_bytes[i*4 + j] = state[1][i][j] ^ key[16 + (i*4 + j) % 8]; // key的16-24字节
        }
      }
      memcpy(out, temp_bytes, 8); // 直接复制异或后的结果到输出缓冲区
      out += 8;
      outlen -= 48;
    }

    if (outlen) {
      aes_block_t state;
      load_state(state, internal_iv, AES_BLOCK_WORDS);
      aes_encrypt(&round_key, state, AES_BLOCK_WORDS, AES_ROUNDS_256);
      u8 tmp[16];
      store_state(tmp, state, AES_BLOCK_WORDS);
      memcpy(out, tmp, outlen);
    }
    return;
  default:
    aes128_init_round_keys(&round_key, key);
    size_t count_total = outlen/32;
    for(size_t count = 0; count < count_total; count++){
      aes_block_t state;
      load_state(state, internal_iv, AES_BLOCK_WORDS);
      aes_encrypt(&round_key, state, AES_BLOCK_WORDS, AES_ROUNDS_128);
      store_state(out, state, AES_BLOCK_WORDS);
      out += 16;
      // 需要逐字节异或，无法直接对aes_block_t使用^操作符
      aes_block_t xored_state;
      for (size_t i = 0; i < AES_BLOCK_WORDS; i++) {
        for (size_t j = 0; j < 4; j++) {
          xored_state[i][j] = state[i][j] ^ key[(i * 4 + j) % 16];
        }
      }
      store_state(out, xored_state, AES_BLOCK_WORDS);
      out += 16;
      outlen -= 32;
    }
    
    if (outlen) {
      aes_block_t state;
      load_state(state, internal_iv, AES_BLOCK_WORDS);
      aes_encrypt(&round_key, state, AES_BLOCK_WORDS, AES_ROUNDS_128);
      u8 tmp[16];
      store_state(tmp, state, AES_BLOCK_WORDS);
      memcpy(out, tmp, outlen);
    }
    return;
  }
#endif
}

void prg(const u8* key, const u8* iv, u32 tweak, u8* out, unsigned int seclvl,
         size_t outlen) {
  u8 internal_iv[IV_SIZE];
  memcpy(internal_iv, iv, IV_SIZE);
  add_to_upper_word(internal_iv, tweak);

#if defined(HAVE_AESNI)
// use AES-NI if possible
#if defined(HAVE_AVX2)
  if (CPU_SUPPORTS_AESNI_AVX2) {
    switch (seclvl) {
    case 256:
      prg_aesni_avx_256(key, internal_iv, out, outlen);
      return;
    case 192:
      prg_aesni_avx_192(key, internal_iv, out, outlen);
      return;
    default:
      prg_aesni_avx_128(key, internal_iv, out, outlen);
      return;
    }
  }
#endif

  if (CPU_SUPPORTS_AESNI) {
    switch (seclvl) {
    case 256:
      prg_aesni_256(key, internal_iv, out, outlen);
      return;
    case 192:
      prg_aesni_192(key, internal_iv, out, outlen);
      return;
    default:
      prg_aesni_128(key, internal_iv, out, outlen);
      return;
    }
  }
#endif

  generic_prg(key, internal_iv, out, seclvl, outlen);
}

void prg_2_lambda(const u8* key, const u8* iv, u32 tweak, u8* out, // 2λ伪随机生成器
                  unsigned int seclvl) {
  u8 internal_iv[IV_SIZE];
  memcpy(internal_iv, iv, IV_SIZE);
  add_to_upper_word(internal_iv, tweak);

#if defined(HAVE_AESNI)
// use AES-NI if possible
#if defined(HAVE_AVX2)
  if (CPU_SUPPORTS_AESNI_AVX2) {
    switch (seclvl) {
    case 256:
      prg_2_aesni_avx_256(key, internal_iv, out);
      return;
    case 192:
      prg_2_aesni_avx_192(key, internal_iv, out);
      return;
    default:
      prg_2_aesni_avx_128(key, internal_iv, out);
      return;
    }
  }
#endif

  if (CPU_SUPPORTS_AESNI) {
    switch (seclvl) {
    case 256:
      prg_2_aesni_256(key, internal_iv, out);
      return;
    case 192:
      prg_2_aesni_192(key, internal_iv, out);
      return;
    default:
      prg_2_aesni_128(key, internal_iv, out);
      return;
    }
  }
#endif

  generic_prg(key, internal_iv, out, seclvl, seclvl * 2 / 8);
}

void prg_4_lambda(const u8* key, const u8* iv, u32 tweak, u8* out, // 4λ伪随机生成器
                  unsigned int seclvl) {
  u8 internal_iv[IV_SIZE];
  memcpy(internal_iv, iv, IV_SIZE);
  add_to_upper_word(internal_iv, tweak);

#if defined(HAVE_AESNI)
// use AES-NI if possible
#if defined(HAVE_AVX2)
  if (CPU_SUPPORTS_AESNI_AVX2) {
    switch (seclvl) {
    case 256:
      prg_4_aesni_avx_256(key, internal_iv, out);
      return;
    case 192:
      prg_4_aesni_avx_192(key, internal_iv, out);
      return;
    default:
      prg_4_aesni_avx_128(key, internal_iv, out);
      return;
    }
  }
#endif

  if (CPU_SUPPORTS_AESNI) {
    switch (seclvl) {
    case 256:
      prg_4_aesni_256(key, internal_iv, out);
      return;
    case 192:
      prg_4_aesni_192(key, internal_iv, out);
      return;
    default:
      prg_4_aesni_128(key, internal_iv, out);
      return;
    }
  }
#endif

  generic_prg(key, internal_iv, out, seclvl, seclvl * 4 / 8);
}

void aes_extend_witness(u8* w, const u8* key, const u8* in, // 扩展见证
                        const vhdsa_paramset_t* params) {
  const unsigned int lambda      = params->lambda;
  const unsigned int num_rounds  = params->R;
  const unsigned int blocksize   = 32 * params->Nst;
  const unsigned int beta        = (lambda + blocksize - 1) / blocksize;
  const unsigned int block_words = blocksize / 32;

#if !defined(NDEBUG)
  u8* const w_out = w;
#endif

  // Step 3
  aes_round_keys_t round_keys;
  switch (lambda) {
  case 256:
    aes256_init_round_keys(&round_keys, key);
    break;
  case 192:
    aes192_init_round_keys(&round_keys, key);
    break;
  default:
    aes128_init_round_keys(&round_keys, key);
    break;
  }

  // Step 4
  const unsigned int nk = lambda / 32;
  for (unsigned int i = 0; i != nk; ++i) {
    memcpy(w, round_keys.round_keys[i / 4][i % 4], sizeof(aes_word_t));
    w += sizeof(aes_word_t);
  }

  const unsigned int S_ke = params->Ske;
  for (unsigned int j = 0, ik = nk; j < S_ke / 4; ++j) {
    memcpy(w, round_keys.round_keys[ik / 4][ik % 4], sizeof(aes_word_t));
    w += sizeof(aes_word_t);
    ik += lambda == 192 ? 6 : 4;
  }

  assert(w - w_out == params->Lke / 8);

  // Step 10
  // common part for AES-128, first part for AES-192 and AES-256
  {
    // Step 12
    aes_block_t state;
    load_state(state, in, block_words);

    // Step 13
    add_round_key(0, state, &round_keys, block_words);

    for (unsigned int round = 1; round < num_rounds; ++round) {
      if (round % 2 == 1) {
        // save inverse norm of the S-box inputs, in coloumn major order
        w = store_invnorm_state(w, state, block_words);
      }
      // Step 15
      sub_bytes(state, block_words);
      // Step 16
      shift_row(state, block_words);
      if (round % 2 == 0) {
        // Step 17
        w = store_state(w, state, block_words);
      }
      // Step 18
      mix_column(state, block_words);
      // Step 19
      add_round_key(round, state, &round_keys, block_words);
    }
    // last round is not commited to, so not computed
  }

  if (beta == 2) {
    // AES-192 and AES-256
    u8 buf[16];
    memcpy(buf, in, sizeof(buf));
    buf[0] ^= 0x1;

    // Step 12
    aes_block_t state;
    load_state(state, buf, block_words);

    // Step 13
    add_round_key(0, state, &round_keys, block_words);

    for (unsigned int round = 1; round < num_rounds; ++round) {
      if (round % 2 == 1) {
        // save inverse norm of the S-box inputs, in coloumn major order
        w = store_invnorm_state(w, state, block_words);
      }
      // Step 15
      sub_bytes(state, block_words);
      // Step 16
      shift_row(state, block_words);
      if (round % 2 == 0) {
        // Step 17
        w = store_state(w, state, block_words);
      }
      // Step 18
      mix_column(state, block_words);
      // Step 19
      add_round_key(round, state, &round_keys, block_words);
    }
    // last round is not commited to, so not computed
  }

  assert(w - w_out == params->l / 8);
}
