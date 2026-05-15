#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include "instances.h"
#include "parameters.h"

const char* vhdsa_get_param_name(vhdsa_paramid_t paramid) {
  switch (paramid) {
  case PARAMETER_SET_INVALID:
    return "PARAMETER_SET_INVALID";
  case VHDSA_128S:
    return "VHDSA_128S";
  case VHDSA_128F:
    return "VHDSA_128F";
  case VHDSA_192S:
    return "VHDSA_192S";
  case VHDSA_192F:
    return "VHDSA_192F";
  case VHDSA_256S:
    return "VHDSA_256S";
  case VHDSA_256F:
    return "VHDSA_256F";
  default:
    return "PARAMETER_SET_MAX_INDEX";
  }
}

#define CALC_TAU1(name) ((name##_LAMBDA - name##_W_GRIND) % name##_TAU)
#define CALC_TAU0(name) (name##_TAU - CALC_TAU1(name))
#define CALC_L(name)                                                                               \
  (CALC_TAU1(name) * (1 << CALC_K(name)) + CALC_TAU0(name) * (1 << (CALC_K(name) - 1)))
#define CALC_K(name) (((name##_LAMBDA - name##_W_GRIND) / (name##_TAU)) + 1)

#define PARAMS(name)                                                                               \
  {                                                                                                \
      name##_LAMBDA,                                                                               \
      name##_TAU,                                                                                  \
      name##_W_GRIND,                                                                              \
      name##_T_OPEN,                                                                               \
      name##_ELL,                                                                                  \
      CALC_K(name),                                                                                \
      CALC_TAU0(name),                                                                             \
      CALC_TAU1(name),                                                                             \
      CALC_L(name),                                                                                \
      name##_Nst,                                                                                  \
      name##_Ske,                                                                                  \
      name##_R,                                                                                    \
      name##_Senc,                                                                                 \
      name##_Lke,                                                                                  \
      name##_Lenc,                                                                                 \
      name##_SIG_SIZE,                                                                             \
      name##_OWF_INPUT_SIZE,                                                                       \
      name##_OWF_OUTPUT_SIZE,                                                                      \
  }

#define VHDSA_128S_PARAMS PARAMS(VHDSA_128S)
#define VHDSA_128F_PARAMS PARAMS(VHDSA_128F)
#define VHDSA_192S_PARAMS PARAMS(VHDSA_192S)
#define VHDSA_192F_PARAMS PARAMS(VHDSA_192F)
#define VHDSA_256S_PARAMS PARAMS(VHDSA_256S)
#define VHDSA_256F_PARAMS PARAMS(VHDSA_256F)

#define CASE_PARAM(P)                                                                              \
  case P: {                                                                                        \
    static const vhdsa_paramset_t params = P##_PARAMS;                                             \
    return &params;                                                                                \
  }

const vhdsa_paramset_t* vhdsa_get_paramset(vhdsa_paramid_t paramid) {
  switch (paramid) {
    CASE_PARAM(VHDSA_128S)
    CASE_PARAM(VHDSA_128F)
    CASE_PARAM(VHDSA_192S)
    CASE_PARAM(VHDSA_192F)
    CASE_PARAM(VHDSA_256S)
    CASE_PARAM(VHDSA_256F)
  default:
    return NULL;
  }
}
