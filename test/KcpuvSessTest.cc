#include "KcpuvSess.h"
#include "KcpuvTest.h"
#include <iostream>
#include <string.h>

namespace kcpuv_test {

using namespace kcpuv;
using namespace std;

static testing::MockFunction<void(const char *)> *test_callback1;

class KcpuvSessTest : public testing::Test {
protected:
  KcpuvSessTest() { KcpuvSess::KcpuvSessEnableTimeout(1); };
  virtual ~KcpuvSessTest(){};
};

static int loopCallbackCalled = 0;
static void LoopCallback(uv_timer_t *timer) {
  loopCallbackCalled = 1;
  Loop::KcpuvStopUpdaterTimer();
  // Loop::KcpuvCheckHandles_();
}

TEST_F(KcpuvSessTest, StartLoopAndExit) {
  KcpuvSess::KcpuvInitialize();

  Loop::KcpuvStartLoop_(LoopCallback);

  EXPECT_EQ(loopCallbackCalled, 1);

  KCPUV_TRY_STOPPING_LOOP_AND_DESTRUCT();
}

static KcpuvSess *test1_sess = NULL;

static void DeleteSessAndStopTimer(KcpuvSess *sess) {
  delete sess;
  Loop::KcpuvStopUpdaterTimer();
}

static void timer_cb(uv_timer_t *timer) {
  kcpuv__try_close_handle(reinterpret_cast<uv_handle_t *>(timer));
  test1_sess->Close();
}

TEST_F(KcpuvSessTest, PushSessToSessList) {
  KcpuvSess::KcpuvInitialize();

  test1_sess = new KcpuvSess();
  KCPUV_INIT_ENCRYPTOR(test1_sess);
  kcpuv_sess_list *sess_list = KcpuvSess::KcpuvGetSessList();

  test1_sess->BindClose(DeleteSessAndStopTimer);

  EXPECT_EQ(sess_list->len, 1);
  EXPECT_TRUE(sess_list->list->next);

  uv_timer_t *timer = new uv_timer_t;

  Loop::KcpuvAddTimer_(timer);
  uv_timer_start(timer, timer_cb, 10, 0);
  Loop::KcpuvStartLoop_(KcpuvSess::KcpuvUpdateKcpSess_);

  EXPECT_EQ(sess_list->len, 0);
  EXPECT_FALSE(sess_list->list->next);

  // TODO: refactor this
  KCPUV_TRY_STOPPING_LOOP_AND_DESTRUCT();
}

static KcpuvSess *test2_sender = NULL;
static KcpuvSess *test2_recver = NULL;

static void test2_free(uv_timer_t *timer) {
  delete test2_sender;
  delete test2_recver;

  uv_close(reinterpret_cast<uv_handle_t *>(timer), free_handle_cb);
  Loop::KcpuvStopUpdaterTimer();
}

void recver_cb(KcpuvSess *sess, const char *data, unsigned int len) {
  test_callback1->Call(data);
  delete test_callback1;

  uv_timer_t *timer = new uv_timer_t;
  Loop::KcpuvNextTick_(timer, test2_free);
}

TEST_F(KcpuvSessTest, TransferOnePacket) {
  KcpuvSess::KcpuvInitialize();

  test_callback1 = new testing::MockFunction<void(const char *)>();
  test2_sender = new KcpuvSess;
  test2_recver = new KcpuvSess(1);
  KCPUV_INIT_ENCRYPTOR(test2_sender);
  KCPUV_INIT_ENCRYPTOR(test2_recver);
  int send_port = 12305;
  int receive_port = 12306;
  char msg[] = "Hello";
  char addr[] = "127.0.0.1";

  test2_sender->Listen(0, NULL);
  // bind local
  int rval = test2_recver->Listen(receive_port, &recver_cb);

  EXPECT_GE(rval, 0);

  test2_sender->InitSend(addr, receive_port);

  test2_sender->Send(msg, strlen(msg));

  EXPECT_CALL(*test_callback1, Call(StrEq("Hello"))).Times(1);

  Loop::KcpuvStartLoop_(KcpuvSess::KcpuvUpdateKcpSess_);

  // TODO: refactor
  // Instances are freed in callbacks.
  KCPUV_TRY_STOPPING_LOOP_AND_DESTRUCT();
}

TEST_F(KcpuvSessTest, TakeNextDgramSource) {
  KcpuvSess::KcpuvInitialize();

  test_callback1 = new testing::MockFunction<void(const char *)>();
  test2_sender = new KcpuvSess;
  test2_recver = new KcpuvSess;
  KCPUV_INIT_ENCRYPTOR(test2_sender);
  KCPUV_INIT_ENCRYPTOR(test2_recver);
  int send_port = 12305;
  int receive_port = 12306;
  char msg[] = "Hello";
  char addr[] = "127.0.0.1";

  test2_recver->SetPassive(1);
  test2_sender->Listen(0, NULL);
  // bind local
  int rval = test2_recver->Listen(receive_port, &recver_cb);

  EXPECT_GE(rval, 0);
  EXPECT_EQ(test2_recver->sessUDP->HasSendAddr(), 0);

  test2_sender->InitSend(addr, receive_port);
  test2_sender->Send(msg, strlen(msg));

  EXPECT_CALL(*test_callback1, Call(StrEq("Hello"))).Times(1);

  Loop::KcpuvStartLoop_(KcpuvSess::KcpuvUpdateKcpSess_);

  EXPECT_EQ(test2_recver->sessUDP->HasSendAddr(), 1);

  // Instances are freed in callbacks.
  KCPUV_TRY_STOPPING_LOOP_AND_DESTRUCT();
}

static testing::MockFunction<void(int)> *test_callback2;
static KcpuvSess *test3_sender = NULL;
static KcpuvSess *test3_recver = NULL;

static void test3_free(uv_timer_t *timer) {
  delete test3_sender;
  delete test3_recver;

  uv_close(reinterpret_cast<uv_handle_t *>(timer), free_handle_cb);
  Loop::KcpuvStopUpdaterTimer();
}

void recver_cb2(KcpuvSess *sess, const char *data, unsigned int len) {
  test_callback2->Call(len);
  delete test_callback2;

  uv_timer_t *timer = new uv_timer_t;
  Loop::KcpuvNextTick_(timer, test3_free);
}

TEST_F(KcpuvSessTest, TransferMultiplePackets) {
  KcpuvSess::KcpuvInitialize();

  test_callback2 = new testing::MockFunction<void(int)>();

  test3_sender = new KcpuvSess();
  test3_recver = new KcpuvSess(1);
  KCPUV_INIT_ENCRYPTOR(test3_sender);
  KCPUV_INIT_ENCRYPTOR(test3_recver);
  int send_port = 12002;
  int receive_port = 12003;
  int size = 4096;
  char *msg = new char[size];
  char addr[] = "127.0.0.1";

  memset(msg, 65, size);

  test3_sender->Listen(0, NULL);
  // bind local
  test3_recver->Listen(receive_port, &recver_cb2);
  test3_sender->InitSend(addr, receive_port);

  test3_sender->Send(msg, size);

  EXPECT_CALL(*test_callback2, Call(size)).Times(1);

  Loop::KcpuvStartLoop_(KcpuvSess::KcpuvUpdateKcpSess_);

  delete[] msg;
  KCPUV_TRY_STOPPING_LOOP_AND_DESTRUCT();
}

static testing::MockFunction<void(int)> *test_callback22;
static KcpuvSess *test4_sender = NULL;
static KcpuvSess *test4_receiver = NULL;

static void test4_free(uv_timer_t *timer) {
  delete test4_sender;
  delete test4_receiver;

  uv_close(reinterpret_cast<uv_handle_t *>(timer), free_handle_cb);
  Loop::KcpuvStopUpdaterTimer();
}

void recver_cb22(KcpuvSess *sess, const char *data, unsigned int len) {
  test_callback22->Call(len);
  delete test_callback22;

  uv_timer_t *timer = new uv_timer_t;
  Loop::KcpuvNextTick_(timer, test4_free);
}

TEST_F(KcpuvSessTest, MockImplementation) {
  KcpuvSess::KcpuvInitialize();

  test_callback22 = new testing::MockFunction<void(int)>();

  char addr[] = "127.0.0.1";
  int sender_port = 8888;
  int receiver_port = 8889;
  char msg[] = "hello";
  int msg_len = 5;

  test4_sender = new KcpuvSess;
  test4_receiver = new KcpuvSess(1);
  KCPUV_INIT_ENCRYPTOR(test4_sender);
  KCPUV_INIT_ENCRYPTOR(test4_receiver);

  test4_sender->Listen(0, NULL);
  test4_receiver->Listen(receiver_port, &recver_cb22);
  test4_sender->InitSend(addr, receiver_port);

  test4_sender->Send(msg, msg_len);

  EXPECT_CALL(*test_callback22, Call(_)).Times(1);

  Loop::KcpuvStartLoop_(KcpuvSess::KcpuvUpdateKcpSess_);

  KCPUV_TRY_STOPPING_LOOP_AND_DESTRUCT();
}

static testing::MockFunction<void(void)> *test_callback3;
static testing::MockFunction<void(void)> *beforeCloseCallback;
static bool closeCalled = 0;

static void close_cb(KcpuvSess *sess) {
  test_callback3->Call();
  closeCalled = 1;

  delete test_callback3;
  delete sess;

  Loop::KcpuvStopUpdaterTimer();
  // Loop::KcpuvCheckHandles_();
}

static void BeforeClose(KcpuvSess *sess) {
  EXPECT_EQ(closeCalled, 0);
  beforeCloseCallback->Call();
  delete beforeCloseCallback;
}

static void do_close_cb(uv_timer_t *timer) {
  KcpuvSess *sess = static_cast<KcpuvSess *>(timer->data);
  uv_timer_stop(timer);
  kcpuv__try_close_handle(reinterpret_cast<uv_handle_t *>(timer));
  sess->Close();
}

TEST_F(KcpuvSessTest, OnCloseCb) {
  KcpuvSess::KcpuvInitialize();

  test_callback3 = new testing::MockFunction<void(void)>();
  beforeCloseCallback = new testing::MockFunction<void(void)>();

  KcpuvSess *sender = new KcpuvSess();
  KCPUV_INIT_ENCRYPTOR(sender);

  sender->BindClose(&close_cb);
  sender->BindBeforeClose(&BeforeClose);

  EXPECT_CALL(*test_callback3, Call()).Times(1);
  EXPECT_CALL(*beforeCloseCallback, Call()).Times(1);

  uv_timer_t *close_timer = new uv_timer_t;

  close_timer->data = sender;
  Loop::KcpuvAddTimer_(close_timer);
  uv_timer_start(close_timer, do_close_cb, 10, 0);

  Loop::KcpuvStartLoop_(KcpuvSess::KcpuvUpdateKcpSess_);

  // refector
  KCPUV_TRY_STOPPING_LOOP_AND_DESTRUCT();
}

static testing::MockFunction<void(void)> *test_callback4 = NULL;
static KcpuvSess *test6_sender = NULL;
static KcpuvSess *test6_recver = NULL;

static void close_cb2(KcpuvSess *sess) {
  delete sess;

  if (test_callback4 == NULL) {
    Loop::KcpuvStopUpdaterTimer();
    return;
  }

  test_callback4->Call();
  delete test_callback4;
  test_callback4 = NULL;
}

static void CloseSender(uv_timer_t *timer) {
  KcpuvSess *sender = reinterpret_cast<KcpuvSess *>(timer->data);
  sender->Close();
  kcpuv__try_close_handle(reinterpret_cast<uv_handle_t *>(timer));
}

TEST_F(KcpuvSessTest, SendindFinWouldCloseOtherSide) {
  KcpuvSess::KcpuvInitialize();

  test_callback4 = new testing::MockFunction<void(void)>();

  test6_sender = new KcpuvSess;
  test6_recver = new KcpuvSess;
  test6_sender->BindClose(close_cb2);
  test6_recver->BindClose(close_cb2);
  KCPUV_INIT_ENCRYPTOR(test6_sender);
  KCPUV_INIT_ENCRYPTOR(test6_recver);
  int send_port = 12004;
  int receive_port = 12005;
  char localaddr[] = "127.0.0.1";

  // bind local
  test6_sender->Listen(send_port, NULL);
  test6_recver->Listen(receive_port, NULL);
  test6_sender->InitSend(localaddr, receive_port);
  test6_recver->InitSend(localaddr, send_port);
  test6_sender->Send("Hello", 5);
  test6_recver->Send("Hello", 5);

  EXPECT_CALL(*test_callback4, Call()).Times(1);

  uv_timer_t *close_timer = new uv_timer_t;

  close_timer->data = test6_sender;
  Loop::KcpuvAddTimer_(close_timer);
  uv_timer_start(close_timer, CloseSender, 1000, 0);

  Loop::KcpuvStartLoop_(KcpuvSess::KcpuvUpdateKcpSess_);

  KCPUV_TRY_STOPPING_LOOP_AND_DESTRUCT();
}

static testing::MockFunction<void(void)> *test_callback5;

static void close_cb3(KcpuvSess *sess) {
  test_callback5->Call();
  delete test_callback5;

  delete sess;

  Loop::KcpuvStopUpdaterTimer();
}

TEST_F(KcpuvSessTest, Timeout) {
  KcpuvSess::KcpuvInitialize();

  test_callback5 = new testing::MockFunction<void(void)>();

  KcpuvSess *sender = new KcpuvSess;
  KCPUV_INIT_ENCRYPTOR(sender);

  int send_port = 12007;
  int receive_port = 12306;
  char localaddr[] = "127.0.0.1";

  sender->Listen(send_port, NULL);
  sender->InitSend(localaddr, receive_port);

  sender->SetTimeout(100);
  sender->BindClose(&close_cb3);

  EXPECT_CALL(*test_callback5, Call()).Times(1);

  Loop::KcpuvStartLoop_(KcpuvSess::KcpuvUpdateKcpSess_);

  KCPUV_TRY_STOPPING_LOOP_AND_DESTRUCT();
}

static void timer_cb2(uv_timer_t *timer) {
  KcpuvSess *sess = static_cast<KcpuvSess *>(timer->data);
  delete sess;
  kcpuv__try_close_handle(reinterpret_cast<uv_handle_t *>(timer));
  Loop::KcpuvStopUpdaterTimer();
}

TEST_F(KcpuvSessTest, GetAddressPort) {
  KcpuvSess::KcpuvInitialize();

  int bind_port = 8990;
  KcpuvSess *sess = new KcpuvSess;
  KCPUV_INIT_ENCRYPTOR(sess);
  sess->Listen(bind_port, NULL);

  char *ip_addr = new char[IP4_ADDR_LENTH];
  int port = 0;
  int namelen = 0;
  int rval = sess->GetAddressPort(ip_addr, &namelen, &port);

  EXPECT_EQ(rval, 0);
  EXPECT_EQ(port, bind_port);

  uv_timer_t *timer = new uv_timer_t;
  timer->data = sess;
  Loop::KcpuvAddTimer_(timer);
  uv_timer_start(timer, timer_cb2, 10, 0);
  Loop::KcpuvStartLoop_(KcpuvSess::KcpuvUpdateKcpSess_);

  delete[] ip_addr;

  KCPUV_TRY_STOPPING_LOOP_AND_DESTRUCT();
}

static testing::MockFunction<void(void)> *WaitFinCloseCb;

static void DeleteSessOnly(KcpuvSess *sess) {
  delete sess;
  WaitFinCloseCb->Call();
}

static void CloseBothSess(uv_timer_t *timer) {
  KcpuvSess *sender = reinterpret_cast<KcpuvSess *>(timer->data);
  sender->Close();
  KcpuvSess *recver = reinterpret_cast<KcpuvSess *>(sender->data);
  recver->Close();
  kcpuv__try_close_handle(reinterpret_cast<uv_handle_t *>(timer));
}

TEST_F(KcpuvSessTest, WaitFinTimer) {
  KcpuvSess::KcpuvInitialize();

  WaitFinCloseCb = new testing::MockFunction<void(void)>();
  EXPECT_CALL(*WaitFinCloseCb, Call()).Times(1);

  KcpuvSess *sender = new KcpuvSess(0);
  KcpuvSess *recver = new KcpuvSess(1);
  recver->SetWaitFinTimeout(1000);
  recver->SetWaitFinTimeout(1000);

  sender->BindClose(DeleteSessAndStopTimer);
  recver->BindClose(DeleteSessOnly);

  KCPUV_INIT_ENCRYPTOR(sender);
  KCPUV_INIT_ENCRYPTOR(recver);
  int send_port = 12004;
  int receive_port = 12005;
  char localaddr[] = "127.0.0.1";

  // bind local
  sender->Listen(send_port, NULL);
  recver->Listen(receive_port, NULL);
  sender->InitSend(localaddr, receive_port);
  recver->InitSend(localaddr, send_port);
  sender->Send("Hello", 5);

  uv_timer_t *close_timer = new uv_timer_t;

  close_timer->data = sender;
  sender->data = recver;
  Loop::KcpuvAddTimer_(close_timer);
  uv_timer_start(close_timer, CloseBothSess, 1000, 0);

  Loop::KcpuvStartLoop_(KcpuvSess::KcpuvUpdateKcpSess_);

  delete WaitFinCloseCb;
  KCPUV_TRY_STOPPING_LOOP_AND_DESTRUCT();
}

} // namespace kcpuv_test
