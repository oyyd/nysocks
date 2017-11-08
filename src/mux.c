#include "mux.h"
#include <stdlib>

static unsigned long DEFAULT_TIMEOUT = 30000;

static unsigned int bytes_to_int(const unsigned char *buffer) {
  return (unsigned int)(buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 |
                        buffer[3]);
}

// Return sid, set cmd and length.
unsigned int kcpuv__mux_decode(const char *buffer, int *cmd, int *length) {
  const unsigned char *buf = (const unsigned char *)buffer;

  *cmd = (int)buffer[0];
  *length = (unsigned int)(buffer[5] << 8 | buffer[6]);

  return bytes_to_int(buf[1]);
}

int kcpuv__mux_encode(char *buffer, int cmd, int length) {
  //
}

static void on_recv_msg(kcpuv_sess *sess, char *data, int len) {
  //
}

// Bind a session that is inited.
int kcpuv_mux_init(kcpuv_mux *mux, kcpuv_sess *sess) {
  mux->count = 0;
  mux->sess = sess;
  sess->data = mux;
  kcpuv_bind_listen(sess, &on_recv_msg);
}

int kcpuv_mux_free(kcpuv_mux *mux) {
  // TODO: free all conns
}

int kcpuv_mux_conn_init(kcpuv_mux *mux, kcpuv_mux_conn *conn) {
  // init conn data
  conn->id = mux->count++;
  conn->timeout = DEFAULT_TIMEOUT;
  conn->ts = 0;

  // add to mux conns
  kcpuv_link *link = kcpuv_link_create(conn);
  kcpuv_link_add(mux->conns, link);
}

int kcpuv_mux_conn_listen(kcpuv_mux_conn *conn, conn_on_msg_cb cb) {
  conn->on_msg_cb = cb;
}

int kcpuv_mux_conn_bind_close(kcpuv_mux_conn *conn, conn_on_close_cb cb) {
  conn->on_close_cb = cb;
}
