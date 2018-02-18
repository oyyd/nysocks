#include "KcpuvTest.h"
#include <iostream>
#include <string.h>

namespace kcpuv_test {

using namespace std;

static testing::MockFunction<void(const char *)> *test_callback1;

class KcpuvSessTest : public testing::Test {
protected:
  KcpuvSessTest() { kcpuv_sess_enable_timeout(1); };
  virtual ~KcpuvSessTest(){};
};

static kcpuv_sess *test1_sess = NULL;

static void timer_cb(uv_timer_t *timer) {
  kcpuv_free(test1_sess, NULL);
  kcpuv_stop_loop();
}

TEST_F(KcpuvSessTest, push_the_sess_to_sess_list_when_created) {
  kcpuv_initialize();

  test1_sess = kcpuv_create();
  KCPUV_INIT_ENCRYPTOR(test1_sess);
  kcpuv_sess_list *sess_list = kcpuv_get_sess_list();

  EXPECT_EQ(sess_list->len, 1);
  EXPECT_TRUE(sess_list->list->next);

  uv_timer_t *timer = new uv_timer_t;

  kcpuv__add_timer(timer);
  uv_timer_start(timer, timer_cb, 0, 0);
  kcpuv_start_loop(kcpuv__update_kcp_sess);

  delete timer;

  EXPECT_EQ(sess_list->len, 0);
  EXPECT_FALSE(sess_list->list->next);

  // TODO: refactor this
  KCPUV_TRY_STOPPING_LOOP();
}

static kcpuv_sess *test2_sender = NULL;
static kcpuv_sess *test2_recver = NULL;

static void test2_free(uv_timer_t *timer) {
  kcpuv_free(test2_sender, NULL);
  kcpuv_free(test2_recver, NULL);

  uv_close(reinterpret_cast<uv_handle_t *>(timer), free_handle_cb);
  kcpuv_stop_loop();
}

void recver_cb(kcpuv_sess *sess, const char *data, int len) {
  test_callback1->Call(data);
  delete test_callback1;

  uv_timer_t *timer = new uv_timer_t;
  kcpuv__next_tick(timer, test2_free);
}

TEST_F(KcpuvSessTest, transfer_one_packet) {
  kcpuv_initialize();

  test_callback1 = new testing::MockFunction<void(const char *)>();
  test2_sender = kcpuv_create();
  test2_recver = kcpuv_create();
  KCPUV_INIT_ENCRYPTOR(test2_sender);
  KCPUV_INIT_ENCRYPTOR(test2_recver);
  int send_port = 12305;
  int receive_port = 12306;
  char *msg = "Hello";

  // bind local
  kcpuv_listen(test2_recver, receive_port, &recver_cb);
  kcpuv_init_send(test2_sender, "127.0.0.1", receive_port);

  kcpuv_send(test2_sender, msg, strlen(msg));

  EXPECT_CALL(*test_callback1, Call(StrEq("Hello"))).Times(1);

  kcpuv_start_loop(kcpuv__update_kcp_sess);

  KCPUV_TRY_STOPPING_LOOP();
}

static testing::MockFunction<void(int)> *test_callback2;
static kcpuv_sess *test3_sender = NULL;
static kcpuv_sess *test3_recver = NULL;

static void test3_free(uv_timer_t *timer) {
  kcpuv_free(test3_sender, NULL);
  kcpuv_free(test3_recver, NULL);

  uv_close(reinterpret_cast<uv_handle_t *>(timer), free_handle_cb);
  kcpuv_stop_loop();
}

void recver_cb2(kcpuv_sess *sess, const char *data, int len) {
  test_callback2->Call(len);
  delete test_callback2;

  uv_timer_t *timer = new uv_timer_t;
  kcpuv__next_tick(timer, test3_free);
}

TEST_F(KcpuvSessTest, transfer_multiple_packets) {
  kcpuv_initialize();

  test_callback2 = new testing::MockFunction<void(int)>();

  test3_sender = kcpuv_create();
  test3_recver = kcpuv_create();
  KCPUV_INIT_ENCRYPTOR(test3_sender);
  KCPUV_INIT_ENCRYPTOR(test3_recver);
  int send_port = 12002;
  int receive_port = 12003;
  int size = 4096;
  char *msg = new char[size];

  memset(msg, 65, size);

  // bind local
  kcpuv_listen(test3_recver, receive_port, &recver_cb2);
  kcpuv_init_send(test3_sender, "127.0.0.1", receive_port);

  kcpuv_send(test3_sender, msg, size);

  EXPECT_CALL(*test_callback2, Call(size)).Times(1);

  kcpuv_start_loop(kcpuv__update_kcp_sess);

  delete msg;
  KCPUV_TRY_STOPPING_LOOP();
}

static testing::MockFunction<void(int)> *test_callback22;
static kcpuv_sess *test4_sender = NULL;
static kcpuv_sess *test4_receiver = NULL;

static void test4_free(uv_timer_t *timer) {
  kcpuv_free(test4_sender, NULL);
  kcpuv_free(test4_receiver, NULL);

  uv_close(reinterpret_cast<uv_handle_t *>(timer), free_handle_cb);
  kcpuv_stop_loop();
}

void recver_cb22(kcpuv_sess *sess, const char *data, int len) {
  test_callback22->Call(len);
  delete test_callback22;

  uv_timer_t *timer = new uv_timer_t;
  kcpuv__next_tick(timer, test4_free);
}

TEST_F(KcpuvSessTest, mock_implementation) {
  kcpuv_initialize();

  test_callback22 = new testing::MockFunction<void(int)>();

  char *addr = "127.0.0.1";
  int sender_port = 8888;
  int receiver_port = 8889;
  char *msg = "hello";
  int msg_len = 5;

  test4_sender = kcpuv_create();
  test4_receiver = kcpuv_create();
  KCPUV_INIT_ENCRYPTOR(test4_sender);
  KCPUV_INIT_ENCRYPTOR(test4_receiver);

  kcpuv_listen(test4_receiver, receiver_port, &recver_cb22);
  kcpuv_init_send(test4_sender, addr, receiver_port);

  kcpuv_send(test4_sender, msg, msg_len);

  EXPECT_CALL(*test_callback22, Call(_)).Times(1);

  kcpuv_start_loop(kcpuv__update_kcp_sess);

  KCPUV_TRY_STOPPING_LOOP();
}

static testing::MockFunction<void(void)> *test_callback3;
static char *error_msg = "invalid";

static void close_cb(kcpuv_sess *sess) {
  test_callback3->Call();

  delete test_callback3;
}

void do_close_cb(uv_timer_t *timer) {
  kcpuv_sess *sess = static_cast<kcpuv_sess *>(timer->data);
  kcpuv_free(sess, NULL);

  kcpuv_stop_loop();
}

TEST_F(KcpuvSessTest, on_close_cb) {
  kcpuv_initialize();

  test_callback3 = new testing::MockFunction<void(void)>();

  kcpuv_sess *sender = kcpuv_create();
  KCPUV_INIT_ENCRYPTOR(sender);

  kcpuv_bind_close(sender, &close_cb);

  EXPECT_CALL(*test_callback3, Call()).Times(1);

  uv_timer_t *timer = new uv_timer_t;

  timer->data = sender;
  kcpuv__add_timer(timer);
  uv_timer_start(timer, do_close_cb, 0, 0);
  kcpuv_start_loop(kcpuv__update_kcp_sess);

  delete timer;

  // refector
  KCPUV_TRY_STOPPING_LOOP();
}

static testing::MockFunction<void(void)> *test_callback4;
static kcpuv_sess *test6_sender = NULL;
static kcpuv_sess *test6_recver = NULL;

static void test6_free(uv_timer_t *timer) {
  kcpuv_free(test6_sender, NULL);
  kcpuv_free(test6_recver, NULL);

  uv_close(reinterpret_cast<uv_handle_t *>(timer), free_handle_cb);
  kcpuv_stop_loop();
}

static void close_cb2(kcpuv_sess *sess) {
  test_callback4->Call();
  delete test_callback4;

  uv_timer_t *timer = new uv_timer_t;
  kcpuv__next_tick(timer, test6_free);
}

TEST_F(KcpuvSessTest, sending_fin_would_close_the_other_side) {
  kcpuv_initialize();

  test_callback4 = new testing::MockFunction<void(void)>();

  test6_sender = kcpuv_create();
  test6_recver = kcpuv_create();
  KCPUV_INIT_ENCRYPTOR(test6_sender);
  KCPUV_INIT_ENCRYPTOR(test6_recver);
  int send_port = 12004;
  int receive_port = 12005;

  // bind local
  kcpuv_listen(test6_sender, send_port, NULL);
  kcpuv_listen(test6_recver, receive_port, NULL);
  kcpuv_init_send(test6_sender, "127.0.0.1", receive_port);
  kcpuv_init_send(test6_recver, "127.0.0.1", send_port);

  kcpuv_bind_close(test6_recver, &close_cb2);

  EXPECT_CALL(*test_callback4, Call()).Times(1);
  kcpuv_close(test6_sender);

  kcpuv_start_loop(kcpuv__update_kcp_sess);

  kcpuv__check_handles();

  KCPUV_TRY_STOPPING_LOOP();
}

static testing::MockFunction<void(void)> *test_callback5;

static void close_cb3(kcpuv_sess *sess) {
  test_callback5->Call();
  delete test_callback5;

  uv_close(reinterpret_cast<uv_handle_t *>(sess->handle), free_handle_cb);

  kcpuv_stop_loop();
}

TEST_F(KcpuvSessTest, timeout) {
  kcpuv_initialize();

  test_callback5 = new testing::MockFunction<void(void)>();

  kcpuv_sess *sender = kcpuv_create();
  KCPUV_INIT_ENCRYPTOR(sender);

  int send_port = 12007;
  int receive_port = 12306;

  kcpuv_listen(sender, send_port, NULL);
  kcpuv_init_send(sender, "127.0.0.1", receive_port);

  sender->timeout = 100;
  kcpuv_bind_close(sender, &close_cb3);

  EXPECT_CALL(*test_callback5, Call()).Times(1);

  kcpuv_start_loop(kcpuv__update_kcp_sess);
  KCPUV_TRY_STOPPING_LOOP();
}

static void timer_cb2(uv_timer_t *timer) {
  kcpuv_free(static_cast<kcpuv_sess *>(timer->data), NULL);
  kcpuv_stop_loop();
}

TEST_F(KcpuvSessTest, kcpuv_get_address) {
  kcpuv_initialize();

  int bind_port = 8990;
  kcpuv_sess *sess = kcpuv_create();
  KCPUV_INIT_ENCRYPTOR(sess);
  kcpuv_listen(sess, bind_port, NULL);

  char *ip_addr = new char[16];
  int port = 0;
  int namelen = 0;
  int rval = kcpuv_get_address(sess, ip_addr, &namelen, &port);

  EXPECT_EQ(rval, 0);
  EXPECT_EQ(port, bind_port);

  uv_timer_t *timer = new uv_timer_t;
  timer->data = sess;
  kcpuv__add_timer(timer);
  uv_timer_start(timer, timer_cb2, 0, 0);
  kcpuv_start_loop(kcpuv__update_kcp_sess);

  delete timer;
  delete[] ip_addr;
  // kcpuv_stop_listen(sess);
  KCPUV_TRY_STOPPING_LOOP();
}

} // namespace kcpuv_test
