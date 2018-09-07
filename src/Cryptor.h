#ifndef KCPUV_CRYPTOR_H
#define KCPUV_CRYPTOR_H

#include "kcpuv.h"
#include "utils.h"
#include <openssl/evp.h>

namespace kcpuv {
typedef struct KCPUV_CRYPTOR {
  EVP_CIPHER_CTX *en;
  EVP_CIPHER_CTX *de;
} kcpuv_cryptor;

class Cryptor {
public:
  Cryptor(){};
  ~Cryptor(){};
  static int KcpuvCryptorInit(kcpuv_cryptor *, const char *key, int key_len,
                              unsigned int salt[]);

  static void KcpuvCryptorClean(kcpuv_cryptor *);

  static unsigned char *KcpuvCryptorEncrypt(kcpuv_cryptor *cryptor,
                                            unsigned char *plaintext, int *len);

  static unsigned char *KcpuvCryptorDecrypt(kcpuv_cryptor *cryptor,
                                            unsigned char *ciphertext,
                                            int *len);

  // Get the bit that indicates should we close this socket.
  // Return 0 if we don't need. Otherwise, return 1.
  static int KcpuvProtocolDecode(const char *content);

  static void KcpuvProtocolEncode(int close, char *content);
};

} // namespace kcpuv

#endif // KCPUV_CRYPTOR_H
