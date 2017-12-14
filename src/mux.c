#include "mux.h"
#include "kcpuv_sess.h"
#include <string.h>

static unsigned long MUX_CONN_DEFAULT_TIMEOUT = 30000;

static unsigned int bytes_to_int(const unsigned char *buffer) {
  return (unsigned int)(buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 |
                        buffer[3]);
}

static void int_to_bytes(unsigned char *buffer, unsigned int id) {
  buffer[0] = (id >> 24) & 0xFF;
  buffer[1] = (id >> 16) & 0xFF;
  buffer[2] = (id >> 8) & 0xFF;
  buffer[3] = id & 0xFF;
}

// Return sid, set cmd and length.
unsigned int kcpuv__mux_decode(const char *buffer, int *cmd, int *length) {
  const unsigned char *buf = (const unsigned char *)buffer;

  *cmd = (int)buf[0];
  *length = (unsigned int)(buf[5] << 8 | buf[6]);

  return bytes_to_int(buf + 1);
}

void kcpuv__mux_encode(char *buffer, unsigned int id, int cmd, int length) {
  unsigned char *buf = (unsigned char *)buffer;
  buf[0] = cmd;
  int_to_bytes(buf + 1, id);
  buf[5] = (length >> 8) & 0xFF;
  buf[6] = (length)&0xFF;
}

void kcpuv_mux_conn_emit_close(kcpuv_mux_conn *conn) {
  // TODO: when get a cmd to close a closed conn
  conn_on_close_cb cb = conn->on_close_cb;
  conn->recv_state = 2;

  if (conn->on_close_cb != NULL) {
    cb(conn, NULL);
  }
}

// TODO: drop invalid msg
static void on_recv_msg(kcpuv_sess *sess, const char *data, int len) {
  int cmd;
  int length;
  unsigned int id = kcpuv__mux_decode((const char *)data, &cmd, &length);
  kcpuv_mux *mux = (kcpuv_mux *)sess->mux;
  kcpuv_mux_conn *conn = NULL;

  // find conn
  kcpuv_link *link = mux->conns.next;

  while (link != NULL && ((kcpuv_mux_conn *)link->node)->id != id) {
    link = link->next;
  }

  if (link != NULL) {
    conn = (kcpuv_mux_conn *)link->node;
  }

  if (cmd == KCPUV_MUX_CMD_CONNECT) {
    // when the conn is connected
    if (conn != NULL) {
      if (conn->recv_state != 0) {
        // NOTE: drop invalid conn
        fprintf(stderr, "%s: %d\n", "receive invalid CONNECT cmd, state",
                conn->recv_state);
        return;
      }
    } else {
      // create new one
      conn = malloc(sizeof(kcpuv_mux_conn));
      kcpuv_mux_conn_init(mux, conn);

      if (mux->on_connection_cb != NULL) {
        mux_on_connection_cb cb = mux->on_connection_cb;
        cb(conn);
      }
      // else {
      //   fprintf(stderr, "%s\n", "'on_connection_cb' is not specified");
      // }
    }
  }

  // NOTE: happend when the conn has been freed
  if (conn == NULL || conn->recv_state == 2) {
    if (KCPUV_DEBUG) {
      fprintf(stderr, "%s\n", "DROP");
    }
    return;
  }

  // ready for data
  conn->recv_state = 1;
  // update ts
  conn->ts = sess->recv_ts;

  // handle cmd
  if (cmd == KCPUV_MUX_CMD_PUSH || cmd == KCPUV_MUX_CMD_CONNECT) {
    if (conn->on_msg_cb != NULL) {
      conn_on_msg_cb cb = conn->on_msg_cb;
      cb(conn, data + KCPUV_MUX_PROTOCOL_OVERHEAD,
         len - KCPUV_MUX_PROTOCOL_OVERHEAD);
    } else {
      // TODO: there would be conns are not with on_msg_cb unexpectly
      // fprintf(stderr, "%s\n", "NO_CB");
      kcpuv_mux_conn_emit_close(conn);
    }
  } else if (cmd == KCPUV_MUX_CMD_CLS) {
    kcpuv_mux_conn_emit_close(conn);
  } else {
    // drop invalid cmd
  }
}

static void on_sess_close(kcpuv_sess *sess, void *data) {
  kcpuv_mux *mux = (kcpuv_mux *)sess->mux;

  if (mux->on_close_cb != NULL) {
    mux_on_close_cb cb = (mux_on_close_cb)mux->on_close_cb;
    // TODO: invalid type
    cb(mux, (const char *)data);
  }
}

// Bind a session that is inited.
void kcpuv_mux_init(kcpuv_mux *mux, kcpuv_sess *sess) {
  mux->count = 0;
  mux->sess = sess;
  mux->conns.next = NULL;
  mux->on_connection_cb = NULL;
  mux->on_close_cb = NULL;
  sess->mux = mux;

  kcpuv_bind_listen(sess, on_recv_msg);
  kcpuv_bind_close(sess, on_sess_close);
}

void kcpuv_mux_free(kcpuv_mux *mux) {
  kcpuv_link *link = mux->conns.next;

  while (link != NULL) {
    // NOTE: Conns will be freed by v8 gc so that we don't need to free them
    // here in node.
    kcpuv_mux_conn *conn = (kcpuv_mux_conn *)link->node;
    kcpuv_mux_conn_emit_close(conn);
    kcpuv_mux_conn_free(conn, NULL);
    link = mux->conns.next;
  }
}

void kcpuv_mux_bind_connection(kcpuv_mux *mux, mux_on_connection_cb cb) {
  mux->on_connection_cb = cb;
}

void kcpuv_mux_bind_close(kcpuv_mux *mux, mux_on_close_cb cb) {
  mux->on_close_cb = cb;
}

void kcpuv_mux_conn_init(kcpuv_mux *mux, kcpuv_mux_conn *conn) {
  // init conn data
  // TODO: should use the id from the remote
  // side when connecting
  conn->id = mux->count++;
  conn->timeout = MUX_CONN_DEFAULT_TIMEOUT;
  conn->ts = iclock();
  conn->mux = mux;
  conn->on_msg_cb = NULL;
  conn->on_close_cb = NULL;
  conn->recv_state = 0;
  conn->send_state = 0;

  // add to mux conns
  kcpuv_link *link = kcpuv_link_create(conn);
  kcpuv_link_add(&(mux->conns), link);
}

// Popout the link from the queue and close it.
// Users have to free kcpuv_mux_conn mannually.
int kcpuv_mux_conn_free(kcpuv_mux_conn *conn, const char *error_msg) {
  kcpuv_link *ptr = kcpuv_link_get_pointer(&(conn->mux->conns), conn);

  if (ptr == NULL) {
    return -1;
  }

  // if (conn->on_close_cb != NULL) {
  //   conn_on_close_cb cb = (conn_on_close_cb *)conn->on_close_cb;
  //   cb(conn, error_msg);
  // }

  free(ptr);
  return 0;
}

void kcpuv_mux_conn_listen(kcpuv_mux_conn *conn, conn_on_msg_cb cb) {
  conn->on_msg_cb = cb;
}

void kcpuv_mux_conn_bind_close(kcpuv_mux_conn *conn, conn_on_close_cb cb) {
  conn->on_close_cb = cb;
}

// NOTE: Conns don't need complex mechannisms to close.
void kcpuv_mux_conn_send(kcpuv_mux_conn *conn, const char *content, int len,
                         int cmd) {
  kcpuv_mux *mux = conn->mux;
  kcpuv_sess *sess = mux->sess;

  unsigned long s = 0;

  // send cmd with empty content
  if (len == 0) {
    if (!cmd) {
      if (conn->send_state == 0) {
        conn->send_state = 1;
        cmd = KCPUV_MUX_CMD_CONNECT;
      } else {
        cmd = KCPUV_MUX_CMD_PUSH;
      }
    }

    unsigned int total_len = KCPUV_MUX_PROTOCOL_OVERHEAD;
    char *encoded_content = malloc(sizeof(total_len));
    kcpuv__mux_encode(encoded_content, conn->id, cmd, len);
    kcpuv_send(sess, encoded_content, total_len);

    free(encoded_content);
    return;
  }

  while (s < (unsigned int)len) {
    if (!cmd) {
      if (conn->send_state == 0) {
        conn->send_state = 1;
        cmd = KCPUV_MUX_CMD_CONNECT;
      } else {
        cmd = KCPUV_MUX_CMD_PUSH;
      }
    }

    unsigned long e = s + MAX_MUX_CONTENT_LEN;

    if (e > (unsigned int)len) {
      e = len;
    }

    unsigned int part_len = e - s;
    unsigned int total_len = part_len + KCPUV_MUX_PROTOCOL_OVERHEAD;

    // TODO: refactor: copy only once when sending
    char *encoded_content = malloc(sizeof(char) * total_len);
    kcpuv__mux_encode(encoded_content, conn->id, cmd, part_len);

    memcpy(encoded_content + KCPUV_MUX_PROTOCOL_OVERHEAD, content + s,
           part_len);

    kcpuv_send(sess, encoded_content, total_len);

    free(encoded_content);
    s = e;
  }
}

void kcpuv_mux_conn_send_close(kcpuv_mux_conn *conn) {
  kcpuv_mux_conn_send(conn, NULL, 0, KCPUV_MUX_CMD_CLS);
}

static void kcpuv_mux_check(kcpuv_mux *mux) {
  IUINT32 current = iclock();
  kcpuv_link *link = &mux->conns;

  while (link->next != NULL) {
    kcpuv_mux_conn *conn = (kcpuv_mux_conn *)link->next->node;

    // check conns timeout
    if (KCPUV_MUX_CONN_TIMEOUT) {
      if (conn->timeout) {
        if (conn->ts + conn->timeout <= current) {
          kcpuv_mux_conn_emit_close(conn);
        }
      }
    }

    // TODO: check closing mark
    link = link->next;
  }
}

void kcpuv__mux_updater(uv_timer_t *timer) {
  // update sessions
  kcpuv__update_kcp_sess(timer);

  // TODO: depending on kcpuv_sess_list may cause
  // some mux without sess to be ignored
  if (kcpuv_get_sess_list() == NULL) {
    return;
  }

  kcpuv_link *link = kcpuv_get_sess_list()->list;
  while (link->next != NULL) {

    link = link->next;
    kcpuv_mux *mux = (kcpuv_mux *)(((kcpuv_sess *)(link->node))->mux);
    if (mux != NULL) {
      kcpuv_mux_check(mux);
    }
  }
}
