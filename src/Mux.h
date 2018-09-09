#ifndef KCPUV_MUX_H
#define KCPUV_MUX_H

#include "KcpuvSess.h"
#include "kcpuv.h"
#include "utils.h"

namespace kcpuv {
typedef struct KCPUV_MUX kcpuv_mux;
typedef struct KCPUV_MUX_CONN kcpuv_mux_conn;

// typedef struct KCPUV_MUX {
//   void *data;
//   unsigned int count;
//   kcpuv_link conns;
//   KcpuvSess *sess;
//   mux_on_connection_cb on_connection_cb;
//   mux_on_close_cb on_close_cb;
// } kcpuv_mux;
//
// typedef struct KCPUV_MUX_CONN {
//   void *data;
//   kcpuv_mux *mux;
//   short send_state;
//   short recv_state;
//   unsigned int id;
//   unsigned long timeout;
//   IUINT32 ts;
//   conn_on_msg_cb on_msg_cb;
//   conn_on_close_cb on_close_cb;
// } kcpuv_mux_conn;

typedef void (*mux_on_connection_cb)(kcpuv_mux_conn *);
typedef void (*mux_on_close_cb)(kcpuv_mux *mux, const char *error_msg);
typedef void (*conn_on_msg_cb)(kcpuv_mux_conn *conn, const char *buffer,
                               int length);
typedef void (*conn_on_close_cb)(kcpuv_mux_conn *conn, const char *error_msg);

class Mux {
public:
  // Set mux timeout.
  // static void kcpuv_set_mux_enable_timeout(short);
  // TODO: Remove this.
  // void kcpuv_mux_init(kcpuv_mux *mux, KcpuvSess *sess);
  // TODO: Remove this.
  // void kcpuv_mux_free(kcpuv_mux *mux);

  // Prevent conns from sending data.
  // void kcpuv_mux_stop(kcpuv_mux *mux);
  //
  // // Bind close event.
  // void kcpuv_mux_bind_close(kcpuv_mux *mux, mux_on_close_cb);
  //
  // // Bind new connections event.
  // void kcpuv_mux_bind_connection(kcpuv_mux *mux, mux_on_connection_cb cb);

protected:
  Mux(KcpuvSess *s);
  ~Mux();

private:
  void *data;
  unsigned int count;
  kcpuv_link conns;
  KcpuvSess *sess;
  mux_on_connection_cb on_connection_cb;
  mux_on_close_cb on_close_cb;
};

// class Conn {
// public:
//   // TODO: remove
//   // void kcpuv_mux_conn_init(kcpuv_mux *, kcpuv_mux_conn *);
//   // int kcpuv_mux_conn_free(kcpuv_mux_conn *, const char *);
//
//   // Tell conn to send data.
//   // int kcpuv_mux_conn_send(kcpuv_mux_conn *, const char *content, int
//   length,
//   //                         int cmd);
//   //
//   // // Tell conn to send closing message.
//   // void kcpuv_mux_conn_send_close(kcpuv_mux_conn *);
//   //
//   // // Bind on message callback.
//   // void kcpuv_mux_conn_listen(kcpuv_mux_conn *, conn_on_msg_cb);
//   //
//   // // Bind on close callback.
//   // void kcpuv_mux_conn_bind_close(kcpuv_mux_conn *, conn_on_close_cb);
//   //
//   // // Trigger close event and prevent conn from sending data.
//   // void kcpuv_mux_conn_emit_close(kcpuv_mux_conn *conn);
//
// protected:
//   Conn();
//   ~Conn();
//
// private:
//   void *data;
//   kcpuv_mux *mux;
//   short send_state;
//   short recv_state;
//   unsigned int id;
//   unsigned long timeout;
//   IUINT32 ts;
//   conn_on_msg_cb on_msg_cb;
//   conn_on_close_cb on_close_cb;
// };

// Get data from raw data.
unsigned int kcpuv__mux_decode(const char *buffer, int *cmd, int *length);

// Add data with protocol info.
void kcpuv__mux_encode(char *buffer, unsigned int id, int cmd, int length);

// Func to digest input data.
void kcpuv__mux_updater(uv_timer_t *timer);
} // namespace kcpuv

#endif
