#include "mux.h"
#include "KcpuvTest.h"

// #define INIT_MUX_TEST() \
//   kcpuv_initialize(); \
//   kpcuv_start_loop();

namespace kcpuv_test {
using namespace std;

class MuxTest : public testing::Test {
protected:
  MuxTest(){};
  virtual ~MuxTest(){};
};

TEST_F(MuxTest, mux_encode_and_decode) {
  char *buf = new char[10];

  int cmd = 10;
  int length = 1400;
  unsigned int id = 65535;

  kcpuv__mux_encode(buf, id, cmd, length);

  EXPECT_EQ(buf[0], cmd);
  EXPECT_EQ(buf[1] & 0xFF, (id >> 24) & 0xFF);
  EXPECT_EQ(buf[2] & 0xFF, (id >> 16) & 0xFF);
  EXPECT_EQ(buf[3] & 0xFF, (id >> 8) & 0xFF);
  EXPECT_EQ((buf[4] & 0xFF), (id)&0xFF);
  EXPECT_EQ(buf[5] & 0xFF, (length >> 8) & 0xFF);
  EXPECT_EQ(buf[6] & 0xFF, (length)&0xFF);

  int decoded_cmd;
  int decoded_length;
  unsigned int decoded_id;

  decoded_id = kcpuv__mux_decode(buf, &decoded_cmd, &decoded_length);

  EXPECT_EQ(id, decoded_id);
  EXPECT_EQ(cmd, decoded_cmd);
  EXPECT_EQ(length, decoded_length);

  delete[] buf;
}

} // namespace kcpuv_test
