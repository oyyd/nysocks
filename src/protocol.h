#ifndef KCPUV_PROTOCOL_H
#define KCPUV_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "utils.h"
#include <openssl/evp.h>

typedef struct KCPUV_CRYPTOR {
  EVP_CIPHER_CTX *en;
  EVP_CIPHER_CTX *de;
} kcpuv_cryptor;

int kcpuv_cryptor_init(kcpuv_cryptor *, char *key, int key_len,
                       unsigned int salt[]);

void kcpuv_cryptor_clean(kcpuv_cryptor *);

unsigned char *kcpuv_cryptor_encrypt(kcpuv_cryptor *cryptor,
                                     unsigned char *plaintext, int *len);

unsigned char *kcpuv_cryptor_decrypt(kcpuv_cryptor *cryptor,
                                     unsigned char *ciphertext, int *len);

// Get the bit that indicates should we close this socket.
// Return 0 if we don't need. Otherwise, return 1.
int kcpuv_protocol_decode(const char *content);

void kcpuv_protocol_encode(int close, char *content);

#ifdef __cplusplus
}
#endif
#endif // KCPUV_PROTOCOL_H
