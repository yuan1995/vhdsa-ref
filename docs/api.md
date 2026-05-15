# 接口说明 API

项目提供 NIST/SUPERCOP 风格接口：

```c
void CRYPT_setup_DSA(CRYPT_state* crypt_state);
int crypto_sign_keypair(unsigned char* pk, unsigned char* sk, CRYPT_state* crypt_state);
int crypto_sign(unsigned char* sm, unsigned long long* smlen,
                const unsigned char* m, unsigned long long mlen,
                const unsigned char* sk, CRYPT_state* crypt_state);
int crypto_sign_open(unsigned char* m, unsigned long long* mlen,
                     const unsigned char* sm, unsigned long long smlen,
                     const unsigned char* pk);
```

[返回目录](../README.md#目录-table-of-contents)

