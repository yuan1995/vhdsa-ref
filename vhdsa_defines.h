#ifndef VHDSA_DEFINES_H
#define VHDSA_DEFINES_H

#if !defined(VHDSA_EXPORT)
#if !defined(VHDSA_STATIC) && (defined(_WIN16) || defined(_WIN32) || defined(_WIN64))
#define VHDSA_EXPORT __declspec(dllimport)
#else
#define VHDSA_EXPORT
#endif
#endif

#if defined(_WIN16) || defined(_WIN32)
#define VHDSA_CALLING_CONVENTION __stdcall
#else
#define VHDSA_CALLING_CONVENTION
#endif

#if defined(__cplusplus)
#define VHDSA_BEGIN_C_DECL extern "C" {
#define VHDSA_END_C_DECL }
#else
#define VHDSA_BEGIN_C_DECL
#define VHDSA_END_C_DECL
#endif

#endif
