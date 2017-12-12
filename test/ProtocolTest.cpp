#include "protocol.h"
#include "KcpuvTest.h"
#include "kcpuv_sess.h"
#include "utils.h"
#include <cstring>

namespace kcpuv_test {
using namespace std;

class ProtocolTest : public testing::Test {
protected:
  ProtocolTest(){};
  virtual ~ProtocolTest(){};
};

TEST_F(ProtocolTest, encode_and_decode_char_pointer) {
  int size = 20;
  char *str = new char[size];

  memset(str, static_cast<int>('a'), size);
  kcpuv_protocol_encode(1, str);

  EXPECT_EQ(static_cast<unsigned int>(str[0]), 1);
  EXPECT_EQ(static_cast<int>(str[2 + KCPUV_NONCE_LENGTH]),
            static_cast<int>('a'));

  int close = kcpuv_protocol_decode(str);

  EXPECT_EQ(close, 1);

  delete str;
}

TEST_F(ProtocolTest, init_and_clean_cryptor) {
  int key_len = 6;
  char *key = new char[key_len];
  unsigned int salt[] = {12345, 23456};
  kcpuv_cryptor *cryptor = new kcpuv_cryptor;

  memset(key, 65, key_len);
  int rval = kcpuv_cryptor_init(cryptor, key, key_len, salt);

  EXPECT_EQ(rval, 0);

  int times = 10;

  for (int i = 0; i < times; i += 1) {
    int content_len = 1400;
    char *content = new char[content_len];
    memset(content, 65, content_len);
    int len = content_len;
    char *ciphertext = reinterpret_cast<char *>(kcpuv_cryptor_encrypt(
        cryptor, reinterpret_cast<unsigned char *>(content), &len));

    int cmpval = strncmp(content, ciphertext, content_len);

    EXPECT_NE(cmpval, 0);

    char *deciphertext = reinterpret_cast<char *>(kcpuv_cryptor_decrypt(
        cryptor, reinterpret_cast<unsigned char *>(ciphertext), &len));

    cmpval = strncmp(content, deciphertext, content_len);

    EXPECT_EQ(cmpval, 0);

    delete[] ciphertext;
    delete[] deciphertext;
    delete[] content;
  }

  kcpuv_cryptor_clean(cryptor);

  delete cryptor;
  delete key;
}

} // namespace kcpuv_test
