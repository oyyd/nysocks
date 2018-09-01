#include "SessUDP.h"
#include "KcpuvTest.h"
#include <cstring>

namespace kcpuv_test {

using namespace std;
using namespace kcpuv;

class SessUDPTest : public testing::Test {
protected:
  SessUDPTest(){};
  virtual ~SessUDPTest(){};
};

TEST_F(SessUDPTest, DeleteSessUDPTest) {
  uv_loop_t loop;
  uv_loop_init(&loop);

  SessUDP *udp = new SessUDP(&loop);

  delete udp;
}

static void dataCb(SessUDP *udp, const char *data, unsigned int len) {
  if (len > 0) {
    ASSERT_STREQ(data, "Hello");
  } else {
    uv_loop_t *loop = reinterpret_cast<uv_loop_t *>(udp->data);
    uv_stop(loop);
  }
}

TEST_F(SessUDPTest, BindAndSend) {
  uv_loop_t loop;
  uv_loop_init(&loop);

  SessUDP *udp = new SessUDP(&loop);
  udp->data = &loop;

  char addr[17];
  int port = 0;
  int namelength = 0;
  int rval;
  char localaddr[] = "0.0.0.0";
  char msg[] = "Hello";

  // Bind port A.
  rval = udp->Bind(0, dataCb);

  if (rval) {
    fprintf(stderr, "%s\n", uv_strerror(rval));
  }
  EXPECT_GT(rval, -1);
  // Get sockname of A.

  udp->GetAddressPort(&namelength, addr, &port);
  EXPECT_GT(port, 0);

  // Create port b and send a message to port A.
  udp->SetSendAddr(localaddr, port);
  udp->Send(msg, sizeof(msg));

  uv_run(&loop, UV_RUN_DEFAULT);

  delete udp;
}
} // namespace kcpuv_test
