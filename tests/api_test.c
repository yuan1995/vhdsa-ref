#include "api.h"
#include "macros.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

// 添加打印十六进制数据的辅助函数
void print_hex(const char* label, const unsigned char* data, size_t len) {
  printf("%s (%zu bytes): ", label, len);
  for (size_t i = 0; i < len; i++) {
    printf("%02x", data[i]);
    if ((i + 1) % 16 == 0 && i + 1 < len) {
      printf("\n%*s", (int)strlen(label) + 12, "");
    }
  }
  printf("\n");
}

#if defined(__WIN32__)
#define LL_FMT "%I64u"
#else
#define LL_FMT "%llu"
#endif

#if defined(WITH_VALGRIND)
#include <valgrind/memcheck.h>
#else
#define VALGRIND_MAKE_MEM_UNDEFINED(p, size)
#define VALGRIND_MAKE_MEM_DEFINED(p, size)
#endif

int main(void) {
  unsigned char pk[CRYPTO_PUBLICKEYBYTES] = {0};
  unsigned char sk[CRYPTO_SECRETKEYBYTES] = {0};
  const unsigned char message[32]         = {0};
  unsigned char omessage[sizeof(message)] = {0};
  unsigned char* sm = calloc(sizeof(message) + CRYPTO_BYTES, sizeof(unsigned char));

  CRYPT_state crypt_state;

  CRYPT_setup_DSA(&crypt_state);

  // 测量KeyGen运行时间
  struct timeval start_keygen, end_keygen;
  gettimeofday(&start_keygen, NULL);
  int ret = crypto_sign_keypair(pk, sk, &crypt_state);
  gettimeofday(&end_keygen, NULL);
  double elapsed_keygen = (end_keygen.tv_sec - start_keygen.tv_sec) * 1000.0 + (end_keygen.tv_usec - start_keygen.tv_usec) / 1000.0;
  printf("KeyGen time: %.3f ms\n", elapsed_keygen);
  
  VALGRIND_MAKE_MEM_DEFINED(&ret, sizeof(ret));
  if (ret != 0) {
    printf("Failed to generate key pair\n");
    free(sm);
    return -1;
  }

  VALGRIND_MAKE_MEM_UNDEFINED(sk, sizeof(sk));
  VALGRIND_MAKE_MEM_UNDEFINED(message, sizeof(message));
  unsigned long long smlen = sizeof(sm);
  // 测量运行时间
  struct timeval start_sign, end_sign;
  gettimeofday(&start_sign, NULL);
  ret = crypto_sign(sm, &smlen, message, sizeof(message), sk, &crypt_state);
  gettimeofday(&end_sign, NULL);
  double elapsed_sign = (end_sign.tv_sec - start_sign.tv_sec) * 1000.0 + (end_sign.tv_usec - start_sign.tv_usec) / 1000.0;
  printf("Signing time: %.3f ms\n", elapsed_sign);
  if (ret != 0) {
    printf("Failed to sign\n");
    free(sm);
    return -1;
  }

  VALGRIND_MAKE_MEM_DEFINED(sm, smlen);
  VALGRIND_MAKE_MEM_DEFINED(sk, sizeof(sk));
  VALGRIND_MAKE_MEM_DEFINED(message, sizeof(message));

  unsigned long long mlen = sizeof(omessage);
  // sm[0] ^= 1; // 修改签名的第一个字节的最低位
  // 测量运行时间
  struct timeval start_verify, end_verify;
  gettimeofday(&start_verify, NULL);
  ret = crypto_sign_open(omessage, &mlen, sm, smlen, pk);
  gettimeofday(&end_verify, NULL);
  double elapsed_verify = (end_verify.tv_sec - start_verify.tv_sec) * 1000.0 + (end_verify.tv_usec - start_verify.tv_usec) / 1000.0;
  printf("Verification time: %.3f ms\n", elapsed_verify);
  if (ret != 0) {
    printf("Failed to verify (ret = %d)\n", ret);
    free(sm);
    return -1;
  }

  if (mlen != sizeof(message)) {
    printf("length of message after verify incorrect, got " LL_FMT ", expected " SIZET_FMT "\n",
           mlen, sizeof(message));
    free(sm);
    return -1;
  }
  if (memcmp(message, omessage, sizeof(message)) != 0) {
    printf("message mismatch after verification\n");
    free(sm);
    return -1;
  }

  // test special case where message and signature overlap
  // memcpy(sm, message, sizeof(message));

  // smlen = sizeof(sm);
  // ret   = crypto_sign(sm, &smlen, sm, sizeof(message), sk);
  // if (ret != 0) {
  //   printf("Failed to sign\n");
  //   free(sm);
  //   return -1;
  // }

  // mlen = smlen;
  // ret  = crypto_sign_open(sm, &mlen, sm, smlen, pk);
  // if (ret != 0) {
  //   printf("Failed to verify (ret = %d)\n", ret);
  //   free(sm);
  //   return -1;
  // }

  // if (mlen != sizeof(message)) {
  //   printf("length of message after verify incorrect, got " LL_FMT ", expected " SIZET_FMT "\n",
  //          mlen, sizeof(message));
  //   free(sm);
  //   return -1;
  // }
  // if (memcmp(message, sm, sizeof(message)) != 0) {
  //   printf("message mismatch after verification\n");
  //   free(sm);
  //   return -1;
  // }

  printf("Sign/Verify test passed\n");
  free(sm);

  return 0;
}
