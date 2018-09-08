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

static void dgramCb(SessUDP *udp, const struct sockaddr *addr, const char *data,
                    int len) {

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
  rval = udp->Bind(0, dgramCb);

  if (rval) {
    fprintf(stderr, "%s\n", uv_strerror(rval));
  }

  EXPECT_GE(rval, 0);
  // Get sockname of A.

  udp->GetAddressPort(&namelength, addr, &port);
  EXPECT_GT(port, 0);

  // Create port b and send a message to port A.
  udp->SetSendAddr(localaddr, port);
  udp->Send(msg, sizeof(msg));

  uv_run(&loop, UV_RUN_DEFAULT);

  delete udp;
}

TEST_F(SessUDPTest, HasSendAddr) {
  uv_loop_t loop;
  uv_loop_init(&loop);

  SessUDP *sessHasAddr = new SessUDP(&loop);
  SessUDP *sessDoesntHasAddr = new SessUDP(&loop);

  char addr[] = "127.0.0.1";
  int port = 0;
  sessHasAddr->SetSendAddr(addr, port);

  EXPECT_EQ(sessHasAddr->HasSendAddr(), 1);
  EXPECT_EQ(sessDoesntHasAddr->HasSendAddr(), 0);

  delete sessHasAddr;
  delete sessDoesntHasAddr;
}

TEST_F(SessUDPTest, SetSendAddrBySockaddr) {
  uv_loop_t loop;
  uv_loop_init(&loop);

  SessUDP *udp = new SessUDP(&loop);
  udp->data = &loop;

  char addrstr[17];
  int port = 12345;
  int namelength = 0;
  int rval;
  char localaddr[] = "0.0.0.0";
  char msg[] = "Hello";
  struct sockaddr *addr = new struct sockaddr;

  // Bind port A.
  rval = udp->Bind(0, dgramCb);
  udp->GetAddressPort(&namelength, addrstr, &port);

  uv_ip4_addr(localaddr, port, reinterpret_cast<struct sockaddr_in *>(addr));

  if (rval) {
    fprintf(stderr, "%s\n", uv_strerror(rval));
  }
  EXPECT_GT(rval, -1);

  // Create port b and send a message to port A.
  udp->SetSendAddrBySockaddr(addr);
  udp->Send(msg, sizeof(msg));

  uv_run(&loop, UV_RUN_DEFAULT);

  delete udp;
}

} // namespace kcpuv_test
