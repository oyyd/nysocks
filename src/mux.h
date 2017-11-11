#ifndef KCPUV_MUX_H
#define KCPUV_MUX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "kcpuv.h"
#include "kcpuv_sess.h"
#include "utils.h"

typedef struct KCPUV_MUX kcpuv_mux;
typedef struct KCPUV_MUX_CONN kcpuv_mux_conn;

typedef void (*mux_on_connection_cb)(kcpuv_mux_conn *);
typedef void (*conn_on_msg_cb)(kcpuv_mux_conn *conn, char *buffer, int length);
typedef void (*conn_on_close_cb)(kcpuv_mux_conn *conn, const char *error_msg);

typedef struct KCPUV_MUX {
  unsigned int count;
  kcpuv_link conns;
  kcpuv_sess *sess;
  mux_on_connection_cb on_connection_cb;
} kcpuv_mux;

typedef struct KCPUV_MUX_CONN {
  kcpuv_mux *mux;
  unsigned int id;
  unsigned long timeout;
  IUINT32 ts;
  conn_on_msg_cb on_msg_cb;
  conn_on_close_cb on_close_cb;
} kcpuv_mux_conn;

void kcpuv_mux_init(kcpuv_mux *mux, kcpuv_sess *sess);

void kcpuv_mux_free(kcpuv_mux *mux);

void kcpuv_mux_conn_init(kcpuv_mux *, kcpuv_mux_conn *);

int kcpuv_mux_conn_free(kcpuv_mux_conn *, const char *);

void kcpuv_mux_send(kcpuv_mux_conn *, const char *content, int length, int cmd);

void kcpuv_mux_bind_connection(kcpuv_mux *mux, mux_on_connection_cb cb);

void kcpuv_mux_conn_listen(kcpuv_mux_conn *, conn_on_msg_cb);

void kcpuv_mux_conn_bind_close(kcpuv_mux_conn *, conn_on_close_cb);

void kcpuv_mux_send_close(kcpuv_mux_conn *);

unsigned int kcpuv__mux_decode(const char *buffer, int *cmd, int *length);

void kcpuv__mux_encode(char *buffer, unsigned int id, int cmd, int length);

void kcpuv__mux_updater(uv_idle_t *idler);

#ifdef __cplusplus
}
#endif

#endif
