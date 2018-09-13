#include "Mux.h"
#include "KcpuvSess.h"
#include "KcpuvTest.h"
#include <cassert>
#include <iostream>

namespace kcpuv_test {
using namespace kcpuv;
using namespace std;

class MuxTest : public testing::Test {
protected:
  MuxTest() { Mux::SetEnableTimeout(1); };
  virtual ~MuxTest(){};
};

static void CloseConn(Conn *conn, unsigned int code) { delete conn; }

static void CloseMux(Mux *mux, const char *error_msg) { delete mux; }

TEST_F(MuxTest, EncodeAndDecode) {
  char *buf = new char[10];

  int cmd = 10;
  int length = 1400;
  unsigned int id = 65535;

  Mux::Encode(buf, id, cmd, length);

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

  decoded_id = Mux::Decode(buf, &decoded_cmd, &decoded_length);

  EXPECT_EQ(id, decoded_id);
  EXPECT_EQ(cmd, decoded_cmd);
  EXPECT_EQ(length, decoded_length);

  delete[] buf;
}

static testing::MockFunction<void(void)> *ConnCloseCalled;
static testing::MockFunction<void(void)> *MuxCloseCalled;

static void ConnOnClose(Conn *conn, unsigned int code) {
  EXPECT_EQ(conn->send_state, KCPUV_CONN_SEND_STOPPED);
  EXPECT_EQ(conn->recv_state, KCPUV_CONN_RECV_STOP);
  ConnCloseCalled->Call();
  delete conn;
}

static void MuxOnClose(Mux *mux, const char *error_msg) {
  delete mux;
  MuxCloseCalled->Call();
  Loop::KcpuvStopUpdaterTimer();
}

// static void DeleteSess(KcpuvSess *sess) {
//   delete sess;
//   Loop::KcpuvStopUpdaterTimer();
// }

static void TimerCloseMux(uv_timer_t *timer) {
  Mux *mux = reinterpret_cast<Mux *>(timer->data);
  mux->Close();

  kcpuv__try_close_handle(reinterpret_cast<uv_handle_t *>(timer));
}

TEST_F(MuxTest, NewAndDelete) {
  KcpuvSess::KcpuvInitialize();
  START_TEST_TIMER(TimerCloseMux);
  int connCount = 2;
  unsigned int maxId = 65535;

  ConnCloseCalled = new testing::MockFunction<void(void)>();
  MuxCloseCalled = new testing::MockFunction<void(void)>();

  EXPECT_CALL(*ConnCloseCalled, Call()).Times(connCount);
  EXPECT_CALL(*MuxCloseCalled, Call()).Times(1);

  Mux *mux = new Mux();
  mux->count = maxId;
  kcpuv_link *conns = mux->GetConns_();

  mux->BindClose(MuxOnClose);
  // Assert mux->conns.
  assert(conns->next == NULL);

  for (int i = 0; i < connCount; i += 1) {
    Conn *conn = mux->CreateConn();
    EXPECT_EQ(conn->send_state, KCPUV_CONN_SEND_NOT_CONNECTED);
    EXPECT_EQ(conn->recv_state, KCPUV_CONN_RECV_NOT_CONNECTED);
    EXPECT_EQ(conn->GetId(), i == 0 ? maxId : 1);
    conn->BindClose(ConnOnClose);
  }

  EXPECT_EQ(mux->HasConnWithId(maxId), 1);
  EXPECT_EQ(mux->HasConnWithId(1), 1);

  EXPECT_EQ(mux->count, 1);

  assert(conns->next != NULL);

  timer->data = mux; // From macro.
  Loop::KcpuvStartLoop_(Mux::UpdateMux);

  delete ConnCloseCalled;
  delete MuxCloseCalled;

  KCPUV_TRY_STOPPING_LOOP_AND_DESTRUCT();
}

// TEST_F(MuxTest, ClosingSessShouldMakeMuxClose) {
//   KcpuvSess::KcpuvInitialize();
//   ENABLE_EMPTY_TIMER();
//
//   MuxCloseCalled = new testing::MockFunction<void(void)>();
//   EXPECT_CALL(*MuxCloseCalled, Call()).Times(1);
//
//   Mux *mux = new Mux();
//
//   mux->BindClose(MuxOnClose);
//
//
//   // Loop::KcpuvStartLoop_();
//
//   delete sess;
//   delete MuxCloseCalled;
//
//   CLOSE_TEST_TIMER();
//   KCPUV_TRY_STOPPING_LOOP_AND_DESTRUCT();
// }

static int received_conns = 0;
static KcpuvSess *sess_p1 = nullptr;
static KcpuvSess *sess_p2 = nullptr;
// mux
static Mux *mux_p1 = nullptr;
static Mux *mux_p2 = nullptr;
static Conn *mux_p1_conn_p1 = nullptr;
// static Conn *mux_p1_conn_p2 = nullptr;
static int muxClosed = 0;

static void CloseMuxCounter(Mux *mux, const char *msg) {
  delete mux;

  if (++muxClosed == 2) {
    Loop::KcpuvStopUpdaterTimer();
  }
}

void P2OnMessage(Conn *conn, const char *buffer, int length) {
  EXPECT_EQ(length, 4096);

  conn->Send("hello", 5, 0);

  received_conns += 1;

  if (received_conns != 2) {
    return;
  }

  conn->Close();
  // Close more than once should be okay.
  conn->Close();
}

static void OnP2Conn(Conn *conn) {
  conn->BindClose(CloseConn);
  conn->BindMsg(P2OnMessage);
}

static int receiveHello = 0;

static void OnDateReturn(Conn *conn, const char *buffer, int length) {
  EXPECT_EQ(length, 5);

  if (++receiveHello == 2) {
    conn->Close();
    mux_p1->Close();
    mux_p2->Close();
  }
}

TEST_F(MuxTest, Transmission) {
  KcpuvSess::KcpuvInitialize();

  sess_p1 = new KcpuvSess;
  sess_p2 = new KcpuvSess(1);
  EXPECT_EQ(sess_p2->GetPassive(), 1);

  KCPUV_INIT_ENCRYPTOR(sess_p1);
  KCPUV_INIT_ENCRYPTOR(sess_p2);

  sess_p1->Listen(0, nullptr);
  sess_p2->Listen(0, nullptr);

  char *addr_p1 = new char[16];
  char *addr_p2 = new char[16];
  int namelen_p1;
  int namelen_p2;
  int port_p1;
  int port_p2;

  sess_p1->GetAddressPort(addr_p1, &namelen_p1, &port_p1);
  sess_p2->GetAddressPort(addr_p2, &namelen_p2, &port_p2);
  sess_p1->InitSend(addr_p2, port_p2);
  // sess_p2->InitSend(addr_p1, port_p1);

  EXPECT_EQ(sess_p1->sessUDP->HasSendAddr(), 1);
  EXPECT_EQ(sess_p2->sessUDP->HasSendAddr(), 0);

  mux_p1 = new Mux(sess_p1);
  mux_p1->BindClose(CloseMuxCounter);
  mux_p2 = new Mux(sess_p2);
  mux_p2->BindClose(CloseMuxCounter);

  mux_p1_conn_p1 = mux_p1->CreateConn();
  mux_p1_conn_p1->BindClose(CloseConn);
  // mux_p1_conn_p2 = mux_p2->CreateConn();
  // mux_p1_conn_p2->BindClose(CloseConn);

  int content_len = 4096;
  char *content = new char[content_len];
  memset(content, 65, content_len);

  mux_p1_conn_p1->Send(content, content_len, 0);
  mux_p1_conn_p1->Send(content, content_len, 0);
  // mux_p1_conn_p2->Send(content, content_len, 0);
  mux_p1_conn_p1->BindMsg(OnDateReturn);

  mux_p2->BindConnection(OnP2Conn);

  // loop
  Loop::KcpuvStartLoop_(Mux::UpdateMux);

  delete[] content;
  delete[] addr_p1;
  delete[] addr_p2;

  KCPUV_TRY_STOPPING_LOOP_AND_DESTRUCT();
}

static int get_mux_conns_count(Mux *mux) {
  int count = 0;

  kcpuv_link *link = mux->conns;

  while (link->next != nullptr) {
    count += 1;
    link = link->next;
  }

  return count;
}

static Mux *test_close_mux = nullptr;
static Conn *test_close_sess_p1_conn_p2 = nullptr;

static void CloseMuxAndStopTimer(Mux *mux, const char *msg) {
  delete mux;
  Loop::KcpuvStopUpdaterTimer();
}

void timeout_close_cb(Conn *conn, unsigned int code) {
  Mux *mux = conn->mux;
  EXPECT_EQ(mux->GetConnLength(), 0);

  delete conn;
  mux->Close();
}

TEST_F(MuxTest, ConnTimeout) {
  KcpuvSess::KcpuvInitialize();

  test_close_mux = new Mux();
  KCPUV_INIT_ENCRYPTOR(test_close_mux->sess);

  test_close_mux->BindClose(CloseMuxAndStopTimer);

  // Conn sess_p1_conn_p1;
  test_close_sess_p1_conn_p2 = test_close_mux->CreateConn();

  test_close_sess_p1_conn_p2->SetTimeout(50);
  test_close_sess_p1_conn_p2->ts = iclock() + 50;

  EXPECT_EQ(test_close_mux->GetConnLength(), 1);
  test_close_sess_p1_conn_p2->BindClose(timeout_close_cb);

  Loop::KcpuvStartLoop_(Mux::UpdateMux);

  // TODO:
  // test_close_mux->Close();
  KCPUV_TRY_STOPPING_LOOP_AND_DESTRUCT();
}

static int freedMux = 0;
static Mux *errorCodeMux = NULL;
static Mux *errorCodeMux2 = NULL;

static void CloseMuxAndCount(Mux *mux, const char *err) {
  delete mux;

  if (++freedMux == 2) {
    Loop::KcpuvStopUpdaterTimer();
  }
}

static void TestZeroErrorCode(Conn *conn, unsigned int code) {
  EXPECT_EQ(code, 0);

  delete conn;
}

static void TestErrorCode(Conn *conn, unsigned int code) {
  EXPECT_EQ(code, 104);

  delete conn;

  errorCodeMux->Close();
  errorCodeMux2->Close();
}

static void TestHelloMsg(Conn *conn, const char *buffer, int length) {
  EXPECT_EQ(length, 5);
}

static void BindConnectionToTestErrorCode(Conn *conn) {
  conn->Send("Hello", 5, 0);
  conn->BindClose(TestErrorCode);
  conn->BindMsg(TestHelloMsg);
}

static void TimerCloseConn(uv_timer_t *timer) {
  Conn *conn = reinterpret_cast<Conn *>(timer->data);
  conn->Close(104);

  kcpuv__try_close_handle(reinterpret_cast<uv_handle_t *>(timer));
}

TEST_F(MuxTest, ConnPassErrorCodeWhenClosed) {
  KcpuvSess::KcpuvInitialize();

  char *addr = new char[16];
  int port = 0;
  int namelen;

  KcpuvSess *sess1 = new KcpuvSess(0);
  KcpuvSess *sess2 = new KcpuvSess(1);

  sess1->Listen(0, NULL);
  sess2->Listen(0, NULL);
  sess2->GetAddressPort(addr, &namelen, &port);
  sess1->InitSend(addr, port);

  errorCodeMux = new Mux(sess1);
  errorCodeMux2 = new Mux(sess2);

  KCPUV_INIT_ENCRYPTOR(errorCodeMux->sess);
  KCPUV_INIT_ENCRYPTOR(errorCodeMux2->sess);

  errorCodeMux->BindClose(CloseMuxAndCount);
  errorCodeMux2->BindClose(CloseMuxAndCount);
  errorCodeMux2->BindConnection(BindConnectionToTestErrorCode);

  Conn *errorCodeConn = errorCodeMux->CreateConn();
  errorCodeConn->BindMsg(TestHelloMsg);
  errorCodeConn->Send("Hello", 5, 0);
  errorCodeConn->BindClose(TestZeroErrorCode);

  START_MS_TEST_TIMER(TimerCloseConn, 2000);
  timer->data = errorCodeConn;

  Loop::KcpuvStartLoop_(Mux::UpdateMux);

  delete[] addr;
  KCPUV_TRY_STOPPING_LOOP_AND_DESTRUCT();
}

} // namespace kcpuv_test
