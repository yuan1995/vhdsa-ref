#ifndef INSTANCES_H
#define INSTANCES_H

#include "macros.h"

#include <stddef.h>
#include <stdint.h>
#include "types.h"
#include <stdbool.h>

#define MAX_LAMBDA 256
#define MAX_LAMBDA_BYTES (MAX_LAMBDA / 8)
#define MAX_DEPTH 12
#define MAX_TAU 32
#define UNIVERSAL_HASH_B_BITS 16
#define UNIVERSAL_HASH_B (UNIVERSAL_HASH_B_BITS / 8)
#define IV_SIZE 16

VHDSA_BEGIN_C_DECL

typedef enum vhdsa_paramid_t {
  PARAMETER_SET_INVALID   = 0,
  VHDSA_128S              = 1,
  VHDSA_128F              = 2,
  VHDSA_192S              = 3,
  VHDSA_192F              = 4,
  VHDSA_256S              = 5,
  VHDSA_256F              = 6,
  PARAMETER_SET_MAX_INDEX = 7
} vhdsa_paramid_t;

typedef struct vhdsa_paramset_t {
  // main parameters
  u16 lambda;
  u8 tau;
  u8 w_grind;
  u16 T_open;
  u16 l;

  // extra parameters
  u16 k;
  u8 tau0;
  u8 tau1;
  u32 L;

  // OWF parameters
  u16 Nst;
  u16 Ske;
  u16 R;
  u16 Senc;
  u16 Lke;
  u16 Lenc;

  // additional parameters
  u16 sig_size;
  u8 owf_input_size;
  u8 owf_output_size;
} vhdsa_paramset_t;

const char* ATTR_CONST vhdsa_get_param_name(vhdsa_paramid_t paramid);
const vhdsa_paramset_t* ATTR_CONST vhdsa_get_paramset(vhdsa_paramid_t paramid);

static inline bool ATTR_PURE vhdsa_is_em(const vhdsa_paramset_t* params) {
  // EM instances do not have key expansion constraints
  return params->Ske == 0;
}

VHDSA_END_C_DECL

#endif
