#ifndef KCPUV_SESS_H
#define KCPUV_SESS_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "ikcp.h"
#include "protocol.h"
#include "utils.h"
#include "uv.h"

extern const int KCPUV_NONCE_LENGTH;
extern const int KCPUV_PROTOCOL_OVERHEAD;
extern long kcpuv_udp_buf_size;
extern uv_loop_t *kcpuv_loop;

typedef struct KCPUV_SESS kcpuv_sess;

typedef void (*kcpuv_listen_cb)(kcpuv_sess *sess, char *data, int len);
typedef void (*kcpuv_close_cb)(kcpuv_sess *sess);

struct KCPUV_SESS {
  // user defined
  void *data;
  ikcpcb *kcp;
  uv_udp_t *handle;
  struct sockaddr *send_addr;
  struct sockaddr *recv_addr;
  int is_closed;
  IUINT32 recv_ts;
  unsigned int timeout;
  kcpuv_listen_cb on_msg_cb;
  kcpuv_close_cb on_close_cb;
  kcpuv_cryptor *cryptor;
  // TODO: outher config
};

typedef struct KCPUV_SESS_LIST {
  kcpuv_link *list;
  int len;
} kcpuv_sess_list;

kcpuv_sess_list *kcpuv_get_sess_list();

void kcpuv_use_default_loop(int value);

kcpuv_sess *kcpuv_create();

void kcpuv_free(kcpuv_sess *sess);

void kcpuv_init_send(kcpuv_sess *sess, char *addr, int port);

int kcpuv_listen(kcpuv_sess *sess, int port, kcpuv_listen_cb cb);

int kcpuv_get_address(kcpuv_sess *sess, char *addr, int *namelen, int *port);

int kcpuv_stop_listen(kcpuv_sess *sess);

void kcpuv_send(kcpuv_sess *sess, const char *msg, unsigned long len);

void kcpuv_close(kcpuv_sess *sess,
                 unsigned int should_send_a_close_cmd_to_the_other_side);

void kcpuv_bind_close(kcpuv_sess *, kcpuv_close_cb);

void kcpuv_initialize();

int kcpuv_destruct();

void kcpuv_start_loop();

void kcpuv_destroy_loop();

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // KCPUV_KCP_SESS
