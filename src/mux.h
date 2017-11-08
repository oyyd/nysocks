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

typedef void (*conn_on_msg_cb)(kcpuv_mux_conn *conn, char *buffer, int length);
typedef void (*conn_on_close_cb)(kcpuv_mux_conn *conn, const char *error_msg);

typedef struct KCPUV_MUX {
  unsigned int count;
  kcpuv_link conns;
  kcpuv_sess *sess;
} kcpuv_mux;

typedef struct KCPUV_MUX_CONN {
  unsigned int id;
  unsigned long timeout;
  IUINT32 ts;
  conn_on_msg_cb on_msg_cb;
  conn_on_close_cb on_close_cb;
} kcpuv_mux_conn;

int kcpuv_mux_init(kcpuv_mux *mux, kcpuv_sess *sess);

int kcpuv_mux_free(kcpuv_mux *mux);

int kcpuv_mux_update(kcpuv_mux *);

int kcpuv_mux_conn_init(kcpuv_mux *, kcpuv_mux_conn *);

int kcpuv_mux_conn_free(kcpuv_mux_conn *);

int kcpuv_mux_send(kcpuv_mux_conn *, const char *content, int length);

int kcpuv_mux_conn_listen(kcpuv_mux_conn *, conn_on_msg_cb);

int kcpuv_mux_conn_bind_close(kcpuv_mux_conn *, conn_on_close_cb);

int kcpuv_mux_send_close(kcpuv_mux_conn *);

unsigned int kcpuv__mux_decode(const char *buffer, int *cmd, int *length);

void kcpuv__mux_encode(char *buffer, unsigned int id, int cmd, int length);

#ifdef __cplusplus
}
#endif

#endif
