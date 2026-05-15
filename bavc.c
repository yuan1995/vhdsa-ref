#include <stdio.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "bavc.h"
#include "random_oracle.h"
#include "compat.h"
#include "aes.h"
#include "instances.h"
#include "universal_hashing.h"

#include <assert.h>
#include <string.h>

#define NODE(nodes, node, lambda_bytes) (&nodes[(node) * (lambda_bytes)])

static void expand_seeds(u8* nodes, const u8* iv, const vhdsa_paramset_t* params) {
  const unsigned int lambda_bytes = params->lambda / 8;

  for (unsigned int alpha = 0; alpha < params->L - 1; ++alpha) {
    // the nodes are located in memory consecutively
    prg_2_lambda(NODE(nodes, alpha, lambda_bytes), iv, alpha,
                 NODE(nodes, 2 * alpha + 1, lambda_bytes), params->lambda);
  }
}

static u8* generate_seeds(const u8* root_seed, const u8* iv,
                               const vhdsa_paramset_t* params) {
  unsigned int lambda_bytes = params->lambda / 8;
  u8* nodes            = calloc(2 * params->L - 1, lambda_bytes);
  assert(nodes);

  memcpy(NODE(nodes, 0, lambda_bytes), root_seed, lambda_bytes);
  expand_seeds(nodes, iv, params);

  return nodes;
}

// VHDSA.LeafCommit
static void vhdsa_leaf_commit(u8* sd, u8* com, const u8* key, const u8* iv,
                              u32 tweak, const u8* uhash, unsigned int lambda) {
  const unsigned int lambda_bytes = lambda / 8;

  u8 buffer[MAX_LAMBDA_BYTES * 4];
  prg_4_lambda(key, iv, tweak, buffer, lambda);
  leaf_hash(com, uhash, buffer, lambda);
  memcpy(sd, buffer, lambda_bytes);
}

#if defined(VHDSA_TESTS)
void leaf_commit(u8* sd, u8* com, const u8* key, const u8* iv, u32 tweak,
                 const u8* uhash, const vhdsa_paramset_t* params) {
  vhdsa_leaf_commit(sd, com, key, iv, tweak, uhash, params->lambda);
}
#endif

// BAVC.PosInTree
ATTR_PURE static inline unsigned int pos_in_tree(unsigned int i, unsigned int j,
                                                 const vhdsa_paramset_t* params) {
  const unsigned int tmp = 1 << (params->k - 1);
  if (j < tmp) {
    return params->L - 1 + params->tau * j + i;
  }
  // mod 2^(k-1) is the same as & 2^(k-1)-1
  const unsigned int mask = tmp - 1;
  return params->L - 1 + params->tau * tmp + params->tau1 * (j & mask) + i;
}

// BAVC.Commit for VHDSA
static void bavc_commit_vhdsa(bavc_t* bavc, const u8* root_key, const u8* iv,
                              const vhdsa_paramset_t* params) {
  const unsigned int lambda       = params->lambda;
  const unsigned int L            = params->L;
  const unsigned int lambda_bytes = lambda / 8;
  const unsigned int com_size     = lambda_bytes * 3; // size of com_ij

  H0_context_t uhash_ctx;
  H0_init(&uhash_ctx, lambda);
  H0_update(&uhash_ctx, iv, IV_SIZE);
  H0_final_for_squeeze(&uhash_ctx);

  H1_context_t h1_com_ctx;
  H1_init(&h1_com_ctx, lambda);

  // Generating the tree (k)
  u8* nodes = generate_seeds(root_key, iv, params);

  // 输出 nodes 变量的前 128 比特（16 字节）
  // for(size_t count = 0; count < 9; count++){
  //   for (unsigned int i = 0; i < 16; i++) {
  //     printf("%02x", nodes[i + count * 16]);  // 打印不同的 16 字节块
  //   }
  //   printf("\n");
  // }
  
  // Initialzing stuff
  bavc->h   = malloc(lambda_bytes * 2);
  bavc->com = malloc(L * com_size);
  bavc->sd  = malloc(L * lambda_bytes);
  assert(bavc->h && bavc->com && bavc->sd);

  // Step: 1..3
  bavc->k = NODE(nodes, 0, lambda_bytes);
  
  assert(bavc->h);
  assert(bavc->com);
  assert(bavc->sd);
  assert(bavc->k);

  // Step: 4..5
  // compute commitments for remaining instances
  for (unsigned int i = 0, offset = 0; i < params->tau; ++i) {
    u8 uhash[MAX_LAMBDA_BYTES * 3];
    H0_squeeze(&uhash_ctx, uhash, 3 * lambda_bytes);

    H1_context_t h1_ctx;
    H1_init(&h1_ctx, lambda);

    const unsigned int N_i = bavc_max_node_index(i, params->tau1, params->k);
    for (unsigned int j = 0; j < N_i; ++j, ++offset) {
      const unsigned int alpha = pos_in_tree(i, j, params);
      vhdsa_leaf_commit(bavc->sd + offset * lambda_bytes, bavc->com + offset * com_size,
                        NODE(nodes, alpha, lambda_bytes), iv, i + L - 1, uhash, lambda);
      H1_update(&h1_ctx, bavc->com + offset * com_size, com_size);
    }

    u8 hi[MAX_LAMBDA_BYTES * 2];
    // Step 11
    H1_final(&h1_ctx, hi, lambda_bytes * 2);
    // Step 12
    H1_update(&h1_com_ctx, hi, lambda_bytes * 2);
  }
  H0_clear(&uhash_ctx);

  // Step 12
  H1_final(&h1_com_ctx, bavc->h, lambda_bytes * 2);
}

void bavc_commit(bavc_t* bavc, const u8* root_key, const u8* iv,
                 const vhdsa_paramset_t* params) {
  bavc_commit_vhdsa(bavc, root_key, iv, params);
}

bool bavc_open(u8* decom_i, const bavc_t* vc, const u16* i_delta,
               const vhdsa_paramset_t* params) {
  const unsigned int lambda       = params->lambda;
  const unsigned int L            = params->L;
  const unsigned int lambda_bytes = lambda / 8;
  const unsigned int k            = params->k;
  const unsigned int tau          = params->tau;
  const unsigned int tau_1        = params->tau1;
  const unsigned int com_size     = 3 * lambda_bytes;

  u8* decom_i_end = decom_i + com_size * tau + params->T_open * lambda_bytes;

  // Step 5
  u8* s = calloc((2 * L - 1 + 7) / 8, 1);
  assert(s);
  // Step 6
  unsigned int nh = 0;

  // Step 7..15
  for (unsigned int i = 0; i < tau; ++i) {
    unsigned int alpha = pos_in_tree(i, i_delta[i], params);
    ptr_set_bit(s, alpha, 1);
    ++nh;

    while (alpha > 0 && ptr_get_bit(s, (alpha - 1) / 2) == 0) {
      alpha = (alpha - 1) / 2;
      ptr_set_bit(s, alpha, 1);
      ++nh;
    }
  }

  // Step 16..17
  if (nh - 2 * tau + 1 > params->T_open) {
    free(s);
    return false;
  }

  // Step 3
  const u8* com = vc->com;
  for (unsigned int i = 0; i < tau; ++i, decom_i += com_size) {
    memcpy(decom_i, com + i_delta[i] * com_size, com_size);
    com += bavc_max_node_index(i, tau_1, k) * com_size;
  }

  // Step 19..25
  for (int i = L - 2; i >= 0; --i) {
    ptr_set_bit(s, i, ptr_get_bit(s, 2 * i + 1) | ptr_get_bit(s, 2 * i + 2));
    if ((ptr_get_bit(s, 2 * i + 1) ^ ptr_get_bit(s, 2 * i + 2)) == 1) {
      const unsigned int alpha = 2 * i + 1 + ptr_get_bit(s, 2 * i + 1);
      memcpy(decom_i, NODE(vc->k, alpha, lambda_bytes), lambda_bytes);
      decom_i += lambda_bytes;
    }
  }

  memset(decom_i, 0, decom_i_end - decom_i);

  free(s);
  return true;
}

static bool reconstruct_keys(u8* s, u8* keys, const u8* decom_i,
                             const u16* i_delta, const u8* iv,
                             const vhdsa_paramset_t* params) {
  const unsigned int lambda       = params->lambda;
  const unsigned int L            = params->L;
  const unsigned int lambda_bytes = lambda / 8;
  const unsigned int tau          = params->tau;

  const u8* nodes = decom_i + 3 * tau * lambda_bytes;
  const u8* end   = nodes + params->T_open * lambda_bytes;

  // Step 7..10
  for (unsigned int i = 0; i < tau; ++i) {
    unsigned int alpha = pos_in_tree(i, i_delta[i], params);
    ptr_set_bit(s, alpha, 1);
  }

  // Step 12.12
  for (int i = L - 2; i >= 0; --i) {
    ptr_set_bit(s, i, ptr_get_bit(s, 2 * i + 1) | ptr_get_bit(s, 2 * i + 2));
    if ((ptr_get_bit(s, 2 * i + 1) ^ ptr_get_bit(s, 2 * i + 2)) == 1) {
      if (nodes == end) {
        return false;
      }

      const unsigned int alpha = 2 * i + 1 + ptr_get_bit(s, 2 * i + 1);
      memcpy(keys + alpha * lambda_bytes, nodes, lambda_bytes);
      nodes += lambda_bytes;
    }
  }

  for (; nodes != end; ++nodes) {
    if (*nodes) {
      return false;
    }
  }

  for (unsigned int i = 0; i != L - 1; ++i) {
    if (!ptr_get_bit(s, i)) {
      prg_2_lambda(keys + i * lambda_bytes, iv, i, keys + (2 * i + 1) * lambda_bytes, lambda);
    }
  }

  return true;
}

static bool bavc_reconstruct_vhdsa(bavc_rec_t* bavc_rec, const u8* decom_i,
                                   const u16* i_delta, const u8* iv,
                                   const vhdsa_paramset_t* params) {
  // Initializing
  const unsigned int lambda       = params->lambda;
  const unsigned int L            = params->L;
  const unsigned int lambda_bytes = lambda / 8;
  const unsigned int k            = params->k;
  const unsigned int tau          = params->tau;
  const unsigned int tau_1        = params->tau1;
  const unsigned int com_size     = lambda_bytes * 3; // size of com_ij

  // Step 6
  u8* s = calloc((2 * L - 1 + 7) / 8, 1);
  assert(s);
  u8* keys = calloc(2 * params->L - 1, lambda_bytes);
  assert(keys);

  if (!reconstruct_keys(s, keys, decom_i, i_delta, iv, params)) {
    free(keys);
    free(s);
    return false;
  }

  H0_context_t uhash_ctx;
  H0_init(&uhash_ctx, lambda);
  H0_update(&uhash_ctx, iv, IV_SIZE);
  H0_final_for_squeeze(&uhash_ctx);

  H1_context_t h1_com_ctx;
  H1_init(&h1_com_ctx, lambda);

  for (unsigned int i = 0, offset = 0; i != tau; ++i) {
    u8 uhash[MAX_LAMBDA_BYTES * 3];
    H0_squeeze(&uhash_ctx, uhash, com_size);

    H1_context_t h1_ctx;
    H1_init(&h1_ctx, lambda);

    const unsigned int N_i = bavc_max_node_index(i, tau_1, k);
    for (unsigned int j = 0; j != N_i; ++j) {
      const unsigned int alpha = pos_in_tree(i, j, params);
      if (ptr_get_bit(s, alpha)) {
        H1_update(&h1_ctx, decom_i + i * com_size, com_size);
      } else {
        u8 com[3 * MAX_LAMBDA_BYTES];
        vhdsa_leaf_commit(bavc_rec->s + offset * lambda_bytes, com, keys + alpha * lambda_bytes, iv,
                          i + L - 1, uhash, lambda);
        ++offset;
        H1_update(&h1_ctx, com, com_size);
      }
    }

    u8 hi[MAX_LAMBDA_BYTES * 2];
    H1_final(&h1_ctx, hi, lambda_bytes * 2);
    H1_update(&h1_com_ctx, hi, lambda_bytes * 2);
  }
  H0_clear(&uhash_ctx);

  H1_final(&h1_com_ctx, bavc_rec->h, lambda_bytes * 2);

  free(keys);
  free(s);
  return true;
}

bool bavc_reconstruct(bavc_rec_t* bavc_rec, const u8* decom_i, const u16* i_delta,
                      const u8* iv, const vhdsa_paramset_t* params) {
  return bavc_reconstruct_vhdsa(bavc_rec, decom_i, i_delta, iv, params);
}

void bavc_clear(bavc_t* com) {
  free(com->sd);
  free(com->com);
  free(com->h);
  free(com->k);
}
