#ifndef KCPUV_SESS_H
#define KCPUV_SESS_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "ikcp.h"
#include "kcpuv.h"
#include "loop.h"
#include "protocol.h"
#include "utils.h"
#include "uv.h"

typedef struct KCPUV_SESS kcpuv_sess;

typedef void (*kcpuv_listen_cb)(kcpuv_sess *sess, const char *data, int len);
typedef void (*kcpuv_dgram_cb)(kcpuv_sess *sess, void *data);
typedef void (*kcpuv_udp_send)(kcpuv_sess *sess, uv_buf_t *buf, int buf_count,
                               const struct sockaddr *);

struct KCPUV_SESS {
  // user defined
  void *data;
  // TODO: make mux store sess if a clearer solution
  void *mux;
  ikcpcb *kcp;
  uv_udp_t *handle;
  struct sockaddr *send_addr;
  struct sockaddr *recv_addr;
  int state;
  IUINT32 recv_ts;
  IUINT32 send_ts;
  unsigned int timeout;
  kcpuv_listen_cb on_msg_cb;
  kcpuv_dgram_cb on_close_cb;
  kcpuv_udp_send udp_send;
  kcpuv_cryptor *cryptor;
};

typedef struct KCPUV_SESS_LIST {
  kcpuv_link *list;
  int len;
} kcpuv_sess_list;

kcpuv_sess_list *kcpuv_get_sess_list();

kcpuv_sess *kcpuv_create();

void kcpuv_sess_init_cryptor(kcpuv_sess *sess, const char *key, int len);

void kcpuv_free(kcpuv_sess *sess);

int kcpuv_set_state(kcpuv_sess *sess, int state);

void kcpuv_input(kcpuv_sess *sess, ssize_t nread, const uv_buf_t *buf,
                 const struct sockaddr *addr);

void kcpuv_init_send(kcpuv_sess *sess, char *addr, int port);

int kcpuv_listen(kcpuv_sess *sess, int port, kcpuv_listen_cb cb);

int kcpuv_get_address(kcpuv_sess *sess, char *addr, int *namelen, int *port);

int kcpuv_stop_listen(kcpuv_sess *sess);

void kcpuv_send(kcpuv_sess *sess, const char *msg, unsigned long len);

void kcpuv_close(kcpuv_sess *sess,
                 unsigned int should_send_a_close_cmd_to_the_other_side,
                 const char *error_msg);

void kcpuv_bind_close(kcpuv_sess *, kcpuv_dgram_cb);

void kcpuv_bind_listen(kcpuv_sess *, kcpuv_listen_cb);

void kcpuv_bind_udp_send(kcpuv_sess *sess, kcpuv_udp_send udp_send);

void kcpuv_initialize();

int kcpuv_destruct();

void kcpuv__update_kcp_sess(uv_timer_t *timer);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // KCPUV_KCP_SESS
