#ifndef VHDSA_VC_H
#define VHDSA_VC_H
#include <assert.h>
#include <stdint.h>
#include "types.h"
#include "instances.h"
#include "utils.h"
VHDSA_BEGIN_C_DECL
typedef struct bavc_t {
  u8* h;
  u8* k;
  u8* com;
  u8* sd;
} bavc_t;
typedef struct bavc_rec_t {
  u8* h;
  u8* s;
} bavc_rec_t;
static inline ATTR_CONST unsigned int bavc_max_node_depth(unsigned int i, unsigned int tau_1,
                                                          unsigned int k) {
  return (i < tau_1) ? k : (k - 1);
}
static inline ATTR_CONST unsigned int bavc_max_node_index(unsigned int i, unsigned int tau_1,
                                                          unsigned int k) {
  assert(k >= 1 && k <= MAX_DEPTH);
  return 1u << ((i < tau_1) ? k : (k - 1));
}
void bavc_commit(bavc_t* bavc, const u8* root_key, const u8* iv,
                 const vhdsa_paramset_t* params);
bool bavc_open(u8* decom_i, const bavc_t* vc, const u16* i_delta,
               const vhdsa_paramset_t* params);
bool bavc_reconstruct(bavc_rec_t* bavc_rec, const u8* decom_i, const u16* i_delta,
                      const u8* iv, const vhdsa_paramset_t* params);
void bavc_clear(bavc_t* com);
#if defined(VHDSA_TESTS)
void leaf_commit(u8* sd, u8* com, const u8* key, const u8* iv, u32 tweak,
                 const u8* uhash, const vhdsa_paramset_t* params);
#endif
VHDSA_END_C_DECL
#endif 