#ifndef VHDSA_VOLE_H
#define VHDSA_VOLE_H

#include <stdbool.h>
#include "types.h"
#include "bavc.h"

VHDSA_BEGIN_C_DECL

void vole_commit(const u8* rootKey, const u8* iv, unsigned int ellhat,
                 const vhdsa_paramset_t* params, bavc_t* vecCom, u8* c, u8* u,
                 u8** v);

bool vole_reconstruct(u8* com, u8** q, const u8* iv, const u8* chall_3,
                      const u8* decom_i, const u8* c, unsigned int ellhat,
                      const vhdsa_paramset_t* params);

#if defined(VHDSA_TESTS)
unsigned int convert_to_vole(const u8* iv, const u8* sd, bool sd0_bot, unsigned int i,
                             unsigned int outLenBytes, u8* u, u8* v,
                             const vhdsa_paramset_t* params);
#endif

VHDSA_END_C_DECL

#endif
