#include "mux.h"
#include "KcpuvTest.h"
#include <iostream>

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

uv_idle_t close_idle;
static int received_conns = 0;

static void try_to_close_loop(uv_idle_t *idle) {
  if (!kcpuv_stop_loop()) {
    uv_idle_stop(idle);
  }
}

void p2_on_msg(kcpuv_mux_conn *conn, const char *buffer, int length) {
  EXPECT_EQ(length, 4096);
  kcpuv_mux_conn_send(conn, "hello", 5, 0);

  received_conns += 1;

  if (received_conns == 2) {
    uv_idle_start(&close_idle, try_to_close_loop);
  }

  kcpuv_mux_conn_free(conn, NULL);
  free(conn);
}

static void on_p2_conn(kcpuv_mux_conn *conn) {
  kcpuv_mux_conn_listen(conn, p2_on_msg);
}

static void on_data_return(kcpuv_mux_conn *conn, const char *buffer,
                           int length) {
  EXPECT_EQ(length, 5);
}

TEST_F(MuxTest, transmission) {
  kcpuv_initialize();

  kcpuv__add_idle(&close_idle);

  kcpuv_sess *sess_p1 = kcpuv_create();
  kcpuv_sess *sess_p2 = kcpuv_create();
  KCPUV_INIT_ENCRYPTOR(sess_p1);
  KCPUV_INIT_ENCRYPTOR(sess_p2);

  kcpuv_listen(sess_p1, 0, NULL);
  kcpuv_listen(sess_p2, 0, NULL);

  char *addr_p1 = new char[16];
  char *addr_p2 = new char[16];
  int namelen_p1;
  int namelen_p2;
  int port_p1;
  int port_p2;

  kcpuv_get_address(sess_p1, addr_p1, &namelen_p1, &port_p1);
  kcpuv_get_address(sess_p2, addr_p2, &namelen_p2, &port_p2);
  kcpuv_init_send(sess_p1, addr_p2, port_p2);
  kcpuv_init_send(sess_p2, addr_p1, port_p1);

  // mux
  kcpuv_mux mux_p1;
  kcpuv_mux mux_p2;

  kcpuv_mux_init(&mux_p1, sess_p1);
  kcpuv_mux_init(&mux_p2, sess_p2);

  kcpuv_mux_conn mux_p1_conn_p1;
  kcpuv_mux_conn mux_p1_conn_p2;
  kcpuv_mux_conn_init(&mux_p1, &mux_p1_conn_p1);
  kcpuv_mux_conn_init(&mux_p1, &mux_p1_conn_p2);

  int content_len = 4096;
  char *content = new char[content_len];
  memset(content, 65, content_len);

  kcpuv_mux_conn_send(&mux_p1_conn_p1, content, content_len, 0);
  kcpuv_mux_conn_send(&mux_p1_conn_p2, content, content_len, 0);
  kcpuv_mux_conn_listen(&mux_p1_conn_p1, on_data_return);

  kcpuv_mux_bind_connection(&mux_p2, on_p2_conn);

  // loop
  kcpuv_start_loop(kcpuv__mux_updater);

  delete[] content;
  delete[] addr_p1;
  delete[] addr_p2;
  kcpuv_mux_conn_free(&mux_p1_conn_p1, NULL);
  kcpuv_mux_conn_free(&mux_p1_conn_p2, NULL);
  kcpuv_mux_free(&mux_p1);
  kcpuv_mux_free(&mux_p2);
  kcpuv_free(sess_p1);
  kcpuv_free(sess_p2);

  KCPUV_TRY_STOPPING_LOOP();
}

static int get_mux_conns_count(kcpuv_mux *mux) {
  int count = 0;

  kcpuv_link *link = &(mux->conns);

  while (link->next != NULL) {
    count += 1;
    link = link->next;
  }

  return count;
}

// void close_cb(kcpuv_mux_conn *conn, const char *error_msg) {
//   kcpuv_mux *mux = conn->mux;
//   EXPECT_EQ(get_mux_conns_count(mux), 1);
// }

void timeout_close_cb(kcpuv_mux_conn *conn, const char *error_msg) {
  kcpuv_mux *mux = conn->mux;
  EXPECT_EQ(get_mux_conns_count(mux), 1);
  kcpuv_stop_loop();
}

TEST_F(MuxTest, close) {
  kcpuv_initialize();

  kcpuv_sess *sess_p1 = kcpuv_create();
  KCPUV_INIT_ENCRYPTOR(sess_p1);

  kcpuv_mux mux;
  kcpuv_mux_init(&mux, sess_p1);

  // kcpuv_mux_conn sess_p1_conn_p1;
  kcpuv_mux_conn sess_p1_conn_p2;

  // kcpuv_mux_conn_init(&mux, &sess_p1_conn_p1);
  kcpuv_mux_conn_init(&mux, &sess_p1_conn_p2);
  sess_p1_conn_p2.timeout = 50;
  sess_p1_conn_p2.ts = iclock() + 50;

  EXPECT_EQ(get_mux_conns_count(&mux), 1);

  // kcpuv_mux_conn_bind_close(&sess_p1_conn_p1, close_cb);
  // kcpuv_mux_conn_free(&sess_p1_conn_p1, NULL);

  kcpuv_mux_conn_bind_close(&sess_p1_conn_p2, timeout_close_cb);

  kcpuv_start_loop(kcpuv__mux_updater);

  kcpuv_mux_conn_free(&sess_p1_conn_p2, NULL);

  kcpuv_mux_free(&mux);
  // destruction
  kcpuv_free(sess_p1);

  KCPUV_TRY_STOPPING_LOOP();
}

} // namespace kcpuv_test
