#include "Mux.h"
#include "KcpuvSess.h"
#include <string.h>

namespace kcpuv {

static unsigned long MUX_CONN_DEFAULT_TIMEOUT = 30000;

static short enable_timeout = KCPUV_MUX_CONN_TIMEOUT;

void kcpuv_set_mux_enable_timeout(short value) { enable_timeout = value; }

static unsigned int bytes_to_int(const unsigned char *buffer) {
  return (unsigned int)(buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 |
                        buffer[3]);
}

// TODO: drop invalid msg
static void OnRecvMsg(KcpuvSess *sess, const char *data, unsigned int len) {
  int cmd;
  int content_length;
  unsigned int id;
  unsigned int offset = 0;

  while (offset < len) {
    id =
        kcpuv__mux_decode((const char *)(data + offset), &cmd, &content_length);

    // fprintf(stderr, "len: %d\n", content_length);
    // fprintf(stderr, "%d %d %d %d %d\n", id, cmd, content_length, offset,
    // len);
    // TODO: Enable this.
    // put_data_to_conn(
    //     sess, (const char *)(data + offset + KCPUV_MUX_PROTOCOL_OVERHEAD),
    //     content_length, id, cmd);

    offset += content_length + KCPUV_MUX_PROTOCOL_OVERHEAD;
  }
}

static void OnSessClose(KcpuvSess *sess) {
  if (sess->mux == nullptr) {
    // mux has been freed
    return;
  }

  // TODO: enable
  // kcpuv_mux_free(sess->mux);
}

static void OnBeforeFree(KcpuvSess *sess) {
  if (sess->mux == nullptr) {
    // mux has been freed
    return;
  }

  // TODO: Enable this.
  // kcpuv_mux_stop(sess->mux);
}

static void int_to_bytes(unsigned char *buffer, unsigned int id) {
  buffer[0] = (id >> 24) & 0xFF;
  buffer[1] = (id >> 16) & 0xFF;
  buffer[2] = (id >> 8) & 0xFF;
  buffer[3] = id & 0xFF;
}

Mux::Mux(KcpuvSess *s) {
  count = 0;
  sess = s;
  conns.next = nullptr;
  on_connection_cb = nullptr;
  on_close_cb = nullptr;
  sess->mux = this;

  sess->BindListen(OnRecvMsg);
  sess->BindClose(OnSessClose);
  sess->BindBeforeClose(OnBeforeFree);
}

Mux::~Mux() {
  kcpuv_link *link = &this->conns;

  while (link->next != nullptr) {
    // fprintf(stderr, "%s\n", "mux_conn_free");
    // TODO: enable
    // kcpuv_mux_conn *conn = (kcpuv_mux_conn *)link->next->node;
    // kcpuv_mux_conn_emit_close(conn);
    // TODO:
    // NOTE: expect js to call the free func
    // kcpuv_mux_conn_free(conn, nullptr);
    // link = link->next;
  }

  // TODO: enable
  // kcpuv_mux_emit_close(this);

  this->sess->mux = nullptr;
  this->sess = nullptr;
  this->on_connection_cb = nullptr;
  this->on_close_cb = nullptr;
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

// static void put_data_to_conn(KcpuvSess *sess, const char *data,
//                              unsigned int len, unsigned int id, int cmd) {
//   kcpuv_mux *mux = (kcpuv_mux *)sess->mux;
//   kcpuv_mux_conn *conn = nullptr;
//
//   // find conn
//   kcpuv_link *link = mux->conns.next;
//
//   while (link != nullptr && ((kcpuv_mux_conn *)link->node)->id != id) {
//     link = link->next;
//   }
//
//   if (link != nullptr) {
//     conn = (kcpuv_mux_conn *)link->node;
//   }
//
//   if (cmd == KCPUV_MUX_CMD_CONNECT) {
//     // when the conn is connected
//     if (conn != nullptr) {
//       if (conn->recv_state != 0) {
//         // NOTE: drop invalid conn
//         fprintf(stderr, "%s: %d\n", "receive invalid CONNECT cmd, state",
//                 conn->recv_state);
//         return;
//       }
//     } else if (id == mux->count) {
//       // create new one
//       conn = malloc(sizeof(kcpuv_mux_conn));
//
//       kcpuv_mux_conn_init(mux, conn);
//
//       if (mux->on_connection_cb != nullptr) {
//         mux_on_connection_cb cb = mux->on_connection_cb;
//         cb(conn);
//       }
//       // else {
//       //   fprintf(stderr, "%s\n", "'on_connection_cb' is not specified");
//       // }
//     } else {
//       fprintf(stderr, "receive invalid msg with id: %d, currenct: %d\n", id,
//               mux->count);
//     }
//   }
//
//   // NOTE: happend when the conn has been freed
//   if (conn == nullptr || conn->recv_state == 2) {
//     if (KCPUV_DEBUG) {
//       fprintf(stderr, "%s %d %d\n", "DROP", id, cmd);
//     }
//     return;
//   }
//
//   // ready for data
//   conn->recv_state = 1;
//   // update ts
//   conn->ts = sess->recv_ts;
//
//   // handle cmd
//   if (cmd == KCPUV_MUX_CMD_PUSH || cmd == KCPUV_MUX_CMD_CONNECT) {
//     if (conn->on_msg_cb != nullptr) {
//       conn_on_msg_cb cb = conn->on_msg_cb;
//       cb(conn, data, len);
//     } else {
//       // TODO: there would be conns are not with on_msg_cb unexpectly
//       fprintf(stderr, "%s: %d %d\n", "no callback specified on conn",
//       conn->id,
//               conn->send_state);
//       kcpuv_mux_conn_emit_close(conn);
//     }
//   } else if (cmd == KCPUV_MUX_CMD_CLS) {
//     kcpuv_mux_conn_emit_close(conn);
//   } else {
//     // drop invalid cmd
//     fprintf(stderr, "%s\n", "receive invalid cmd");
//   }
// }

// void kcpuv_mux_conn_emit_close(kcpuv_mux_conn *conn) {
//   // TODO: when get a cmd to close a closed conn
//   conn_on_close_cb cb = conn->on_close_cb;
//   conn->recv_state = 2;
//
//   if (conn->on_close_cb != nullptr) {
//     cb(conn, nullptr);
//   } else {
//     kcpuv_mux_conn_free(conn, nullptr);
//   }
// }

// static void kcpuv_mux_emit_close(kcpuv_mux *mux) {
//   if (mux->on_close_cb != nullptr) {
//     mux_on_close_cb cb = (mux_on_close_cb)mux->on_close_cb;
//     // TODO:
//     cb(mux, nullptr);
//   }
// }

// // Bind a session that is inited.
// void kcpuv_mux_init(kcpuv_mux *mux, KcpuvSess *sess) {
//   mux->count = 0;
//   mux->sess = sess;
//   mux->conns.next = nullptr;
//   mux->on_connection_cb = nullptr;
//   mux->on_close_cb = nullptr;
//   sess->mux = mux;
//
//   kcpuv_bind_listen(sess, OnRecvMsg);
//   kcpuv_bind_close(sess, OnSessClose);
//   kcpuv_bind_before_free(sess, OnBeforeFree);
// }
//
// void kcpuv_mux_stop(kcpuv_mux *mux) {
//   kcpuv_link *link = &mux->conns;
//
//   while (link->next != nullptr) {
//     kcpuv_mux_conn *conn = (kcpuv_mux_conn *)link->next->node;
//     conn->send_state = 2;
//     link = link->next;
//   }
// }
//
//
// void kcpuv_mux_bind_connection(kcpuv_mux *mux, mux_on_connection_cb cb) {
//   mux->on_connection_cb = cb;
// }
//
// void kcpuv_mux_bind_close(kcpuv_mux *mux, mux_on_close_cb cb) {
//   mux->on_close_cb = cb;
// }
//
// void kcpuv_mux_conn_init(kcpuv_mux *mux, kcpuv_mux_conn *conn) {
//   // init conn data
//   // TODO: over two bytes
//   // side when connecting
//   conn->id = mux->count++;
//
//   if (mux->count >= 65535) {
//     mux->count = 0;
//   }
//
//   conn->timeout = MUX_CONN_DEFAULT_TIMEOUT;
//   conn->ts = iclock();
//   conn->mux = mux;
//   conn->on_msg_cb = nullptr;
//   conn->on_close_cb = nullptr;
//   conn->recv_state = 0;
//   conn->send_state = 0;
//
//   // add to mux conns
//   kcpuv_link *link = kcpuv_link_create(conn);
//   kcpuv_link_add(&(mux->conns), link);
// }
//
// // Popout the link from the queue and close it.
// // Users have to free kcpuv_mux_conn mannually.
// int kcpuv_mux_conn_free(kcpuv_mux_conn *conn, const char *error_msg) {
//   kcpuv_link *ptr = kcpuv_link_get_pointer(&(conn->mux->conns), conn);
//
//   if (ptr == nullptr) {
//     return -1;
//   }
//
//   // if (conn->on_close_cb != nullptr) {
//   //   conn_on_close_cb cb = (conn_on_close_cb *)conn->on_close_cb;
//   //   cb(conn, error_msg);
//   // }
//
//   free(ptr);
//   return 0;
// }
//
// void kcpuv_mux_conn_listen(kcpuv_mux_conn *conn, conn_on_msg_cb cb) {
//   conn->on_msg_cb = cb;
// }
//
// void kcpuv_mux_conn_bind_close(kcpuv_mux_conn *conn, conn_on_close_cb cb) {
//   conn->on_close_cb = cb;
// }
//
// // NOTE: Conns don't need complex mechannisms to close.
// int kcpuv_mux_conn_send(kcpuv_mux_conn *conn, const char *content, int len,
//                         int cmd) {
//   kcpuv_mux *mux = conn->mux;
//   KcpuvSess *sess = mux->sess;
//
//   unsigned long s = 0;
//   int send_cmd = cmd;
//
//   // prevent sending when closed
//   if (conn->send_state == 2) {
//     return -1;
//   }
//
//   // send cmd with empty content
//   if (len == 0) {
//     if (!cmd) {
//       if (conn->send_state == 0) {
//         conn->send_state = 1;
//         send_cmd = KCPUV_MUX_CMD_CONNECT;
//       } else {
//         send_cmd = KCPUV_MUX_CMD_PUSH;
//       }
//     }
//
//     unsigned int total_len = KCPUV_MUX_PROTOCOL_OVERHEAD;
//     char *encoded_content = malloc(sizeof(total_len));
//     kcpuv__mux_encode(encoded_content, conn->id, send_cmd, len);
//     kcpuv_send(sess, encoded_content, total_len);
//
//     free(encoded_content);
//     return 0;
//   }
//
//   while (s < (unsigned int)len) {
//     if (!cmd) {
//       if (conn->send_state == 0) {
//         conn->send_state = 1;
//         send_cmd = KCPUV_MUX_CMD_CONNECT;
//       } else {
//         send_cmd = KCPUV_MUX_CMD_PUSH;
//       }
//     }
//
//     unsigned long e = s + MAX_MUX_CONTENT_LEN;
//
//     if (e > (unsigned int)len) {
//       e = len;
//     }
//
//     unsigned int part_len = e - s;
//     unsigned int total_len = part_len + KCPUV_MUX_PROTOCOL_OVERHEAD;
//
//     // fprintf(stderr, "PART_LEN %d %d %d\n", part_len, cmd,
//     conn->send_state);
//
//     // TODO: refactor: copy only once when sending
//     char *encoded_content = malloc(sizeof(char) * total_len);
//     kcpuv__mux_encode(encoded_content, conn->id, send_cmd, part_len);
//
//     memcpy(encoded_content + KCPUV_MUX_PROTOCOL_OVERHEAD, content + s,
//            part_len);
//
//     kcpuv_send(sess, encoded_content, total_len);
//
//     free(encoded_content);
//     s = e;
//   }
//
//   return 0;
// }
//
// void kcpuv_mux_conn_send_close(kcpuv_mux_conn *conn) {
//   kcpuv_mux_conn_send(conn, nullptr, 0, KCPUV_MUX_CMD_CLS);
// }
//
// static void kcpuv_mux_check(kcpuv_mux *mux) {
//   IUINT32 current = iclock();
//   kcpuv_link *link = &mux->conns;
//
//   while (link->next != nullptr) {
//     kcpuv_mux_conn *conn = (kcpuv_mux_conn *)link->next->node;
//
//     // check conns timeout
//     if (enable_timeout) {
//       if (conn->timeout) {
//         if (conn->ts + conn->timeout <= current) {
//           kcpuv_mux_conn_emit_close(conn);
//         }
//       }
//     }
//
//     // TODO: check closing mark
//     link = link->next;
//   }
// }
//
// void kcpuv__mux_updater(uv_timer_t *timer) {
//   // update sessions
//   kcpuv__update_kcp_sess(timer);
//
//   // TODO: depending on kcpuv_sess_list may cause
//   // some mux without sess to be ignored
//   if (kcpuv_get_sess_list() == nullptr) {
//     return;
//   }
//
//   kcpuv_link *link = kcpuv_get_sess_list()->list;
//
//   while (link->next != nullptr) {
//     link = link->next;
//
//     kcpuv_mux *mux = (kcpuv_mux *)(((KcpuvSess *)(link->node))->mux);
//
//     if (mux != nullptr) {
//       kcpuv_mux_check(mux);
//     }
//   }
// }

} // namespace kcpuv
