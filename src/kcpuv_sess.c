#include "kcpuv_sess.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int debug = 1;

// extern
// TODO: it only accepts int in uv.h
long kcpuv_udp_buf_size = 4 * 1024 * 1024;
uv_loop_t *kcpuv_loop = NULL;

static int use_default_loop = 1;
const int KCPUV_NONCE_LENGTH = 8;
const int KCPUV_PROTOCOL_OVERHEAD = 1;
static const int WND_SIZE = 2048;
static const IUINT32 IKCP_MTU_DEF =
    1400 - KCPUV_PROTOCOL_OVERHEAD - KCPUV_NONCE_LENGTH;
static const IUINT32 IKCP_OVERHEAD = 24;
static const size_t MAX_SENDING_LEN = 65536;
static const size_t BUFFER_LEN = MAX_SENDING_LEN;
static const unsigned int DEFAULT_TIMEOUT = 30000;

static kcpuv_sess_list *sess_list = NULL;
static char *buffer = NULL;
static uv_idle_t *idler = NULL;

// NOTE: Use this after first creation.
kcpuv_sess_list *kcpuv_get_sess_list() { return sess_list; }

void kcpuv_initialize() {
  if (sess_list != NULL) {
    return;
  }

  // check and create list
  sess_list = malloc(sizeof(kcpuv_sess_list));
  sess_list->list = kcpuv_link_create(NULL);

  // init buffer
  buffer = malloc(sizeof(char) * BUFFER_LEN);

  // init loop
  if (!use_default_loop) {
    kcpuv_loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(kcpuv_loop);
  } else {
    kcpuv_loop = uv_default_loop();
  }
}

void kcpuv_destruct() {
  if (sess_list == NULL) {
    return;
  }

  uv_loop_close(kcpuv_loop);
  free(kcpuv_loop);

  kcpuv_link *ptr = sess_list->list;
  kcpuv_link *ptr_next = ptr->next;
  free(buffer);
  buffer = NULL;
  // destruct all nodes
  while (ptr_next) {
    ptr = ptr_next;
    ptr_next = ptr->next;

    if (ptr->node != NULL) {
      kcpuv_sess *sess = ptr->node;
      kcpuv_free(sess);
    }

    kcpuv_link_remove(sess_list->list, ptr);
  }

  free(sess_list->list);
  free(sess_list);
  sess_list = NULL;
}

// Iterate the session_list and update kcp
static void update_kcp_sess(uv_idle_t *idler) {
  if (!sess_list || !sess_list->list) {
    return;
  }

  kcpuv_link *ptr = sess_list->list;

  // TODO: maybe we could assume that ikcp_update won't
  // cost too much time and get the time once
  IUINT32 now = iclock();

  while (ptr->next != NULL) {
    ptr = ptr->next;
    int size;
    kcpuv_sess *sess = (kcpuv_sess *)ptr->node;

    if (now - sess->recv_ts >= sess->timeout) {
      kcpuv_close(sess, 1);
      continue;
    }

    IUINT32 time_to_update = ikcp_check(sess->kcp, now);

    if (time_to_update <= now) {
      ikcp_update(sess->kcp, now);
    }

    // TODO: consider the expected size
    size = ikcp_recv(sess->kcp, buffer, BUFFER_LEN);

    if (size < 0) {
      // rval == -1 queue is empty
      // rval == -2 peeksize is zero(or invalid)
      if (size != -1 && size != -2) {
        // TODO:
        fprintf(stderr, "ikcp_recv() < 0: %d\n", size);
      }
      continue;
    }

    if (sess->on_msg_cb == NULL) {
      continue;
    }

    // update receive data
    kcpuv_listen_cb on_msg_cb = sess->on_msg_cb;
    on_msg_cb(sess, buffer, size);
  }
}

// Start uv kcpuv_loop and updating kcp.
void kcpuv_start_loop() {
  // inited before

  // bind a idle watcher to uv kcpuv_loop for updating kcp
  // TODO: do we need to delete idler?
  idler = malloc(sizeof(uv_idle_t));
  uv_idle_init(kcpuv_loop, idler);
  uv_idle_start(idler, &update_kcp_sess);

  if (!use_default_loop) {
    uv_run(kcpuv_loop, UV_RUN_DEFAULT);
  }
}

void kcpuv_destroy_loop() {
  uv_idle_stop(idler);
  free(idler);

  if (!use_default_loop) {
    uv_stop(kcpuv_loop);
  }
}

// Send msg through kcp.
void kcpuv_send(kcpuv_sess *sess, const char *msg, unsigned long len) {
  unsigned long s = 0;

  while (s < len) {
    unsigned long e = s + MAX_SENDING_LEN;

    if (e > len) {
      e = len;
    }

    unsigned long part_len = e - s;
    char *data = malloc(sizeof(char) * part_len);

    memcpy(data, msg + s, part_len);

    int rval = ikcp_send(sess->kcp, data, part_len);

    if (debug == 1 && rval < 0) {
      // TODO:
      printf("ikcp_send() < 0: %d", rval);
    }

    s = e;
  }
}

static int send_cb(uv_udp_send_t *req, int status) {
  // TODO: should check status?
  free(req);
}

// Func to output data for kcp through udp.
// NOTE: Should call `kcpuv_init_send` with the session before udp_output
static int udp_output(const char *msg, int len, ikcpcb *kcp, void *user) {
  if (debug) {
    printf("output: %d %ld\n", len, iclock64());
    printf("content: ");
    print_as_hex(msg, len);
    printf("\n");
  }

  kcpuv_sess *sess = (kcpuv_sess *)user;

  // TODO: allocate twice
  uv_udp_send_t *req = malloc(sizeof(uv_udp_send_t));
  int write_len = len + KCPUV_PROTOCOL_OVERHEAD + KCPUV_NONCE_LENGTH;
  // ikcp assume we copy and send the msg
  char *plaintext = malloc(sizeof(char) * write_len);

  // encode protocol
  kcpuv_protocol_encode(sess->is_closed, plaintext);
  memcpy(plaintext + KCPUV_PROTOCOL_OVERHEAD + KCPUV_NONCE_LENGTH, msg, len);

  // encrypt
  char *data = kcpuv_cryptor_encrypt(sess->cryptor, plaintext, &write_len);

  // make buffers from string
  uv_buf_t buf = uv_buf_init(data, write_len);

  uv_udp_send(req, sess->handle, &buf, 1, sess->send_addr, &send_cb);

  return 0;
}

// Create a kcpuv session. This is a common structure for
// both of the sending and receiving.
// A session could only have one recv_addr and send_addr.
// TODO: conv
kcpuv_sess *kcpuv_create() {
  kcpuv_sess *sess = malloc(sizeof(kcpuv_sess));
  sess->kcp = ikcp_create(0, sess);
  // ikcp_nodelay(sess->kcp, 0, 10, 0, 0);
  ikcp_nodelay(sess->kcp, 1, 10, 2, 1);
  ikcp_wndsize(sess->kcp, WND_SIZE, WND_SIZE);
  ikcp_setmtu(sess->kcp, IKCP_MTU_DEF);
  // sess->kcp->rmt_wnd = WND_SIZE;
  // sess->kcp->stream = 1;

  sess->handle = malloc(sizeof(uv_udp_t));
  uv_udp_init(kcpuv_loop, sess->handle);

  sess->handle->data = sess;
  sess->send_addr = NULL;
  sess->recv_addr = NULL;
  sess->on_msg_cb = NULL;
  sess->on_close_cb = NULL;
  sess->is_closed = 0;
  sess->recv_ts = iclock();
  sess->timeout = DEFAULT_TIMEOUT;

  sess->cryptor = malloc(sizeof(kcpuv_cryptor));
  // TODO:
  char *key = "Hello";
  unsigned int salt[] = {1, 2};
  kcpuv_cryptor_init(sess->cryptor, key, strlen(key), salt);

  // set output func for kcp
  sess->kcp->output = udp_output;

  // create link and push to the queue
  kcpuv_link *link = kcpuv_link_create(sess);
  kcpuv_link_add(sess_list->list, link);

  sess_list->len += 1;

  return sess;
}

// Free a kcpuv session.
void kcpuv_free(kcpuv_sess *sess) {
  if (sess_list != NULL && sess_list->list != NULL) {
    kcpuv_link_remove(sess_list->list, sess);
    sess_list->len -= 1;
  }

  // kcpuv_cryptor_clean(sess->cryptor);
  free(sess->cryptor);

  if (sess->send_addr != NULL) {
    free(sess->send_addr);
  }
  if (sess->recv_addr != NULL) {
    free(sess->recv_addr);
  }
  free(sess->handle);
  ikcp_release(sess->kcp);
  free(sess);
}

// Set sending info.
void kcpuv_init_send(kcpuv_sess *sess, char *addr, int port) {
  sess->send_addr = malloc(sizeof(struct sockaddr_in));
  uv_ip4_addr(addr, port, sess->send_addr);
}

// Called when uv receives a msg and pass
static void on_recv(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf,
                    const struct sockaddr *addr, unsigned flags) {
  if (nread < 0) {
    // TODO:
    fprintf(stderr, "uv error: %s\n", uv_strerror(nread));
  }

  if (nread > 0) {
    kcpuv_sess *sess = (kcpuv_sess *)handle->data;

    ssize_t read_len = nread;
    char *read_msg = kcpuv_cryptor_decrypt(
        sess->cryptor, (unsigned char *)buf->base, &read_len);

    if (debug) {
      printf("input: %ld\n", iclock64());
      print_as_hex(read_msg, read_len);
      printf("\n");
    }

    // check protocol
    int close = kcpuv_protocol_decode(read_msg);
    if (close) {
      kcpuv_close(sess, 0);
      free(buf->base);
      free(read_msg);
      return;
    }

    // update active time
    sess->recv_ts = iclock();

    int rval = ikcp_input(
        sess->kcp,
        (const char *)(read_msg + KCPUV_PROTOCOL_OVERHEAD + KCPUV_NONCE_LENGTH),
        read_len - KCPUV_PROTOCOL_OVERHEAD - KCPUV_NONCE_LENGTH);

    if (debug == 1 && rval < 0) {
      // TODO:
      printf("ikcp_input() < 0: %d", rval);
    }

    free(read_msg);
  }

  // release uv buf
  free(buf->base);
}

// Set receiving info.
int kcpuv_listen(kcpuv_sess *sess, int port, kcpuv_listen_cb cb) {
  sess->on_msg_cb = cb;
  // get local addr
  sess->recv_addr = malloc(sizeof(struct sockaddr_in));
  uv_ip4_addr("127.0.0.1", port, sess->recv_addr);

  // bind local and start recv
  int rval = uv_udp_bind(sess->handle, (const struct sockaddr *)sess->recv_addr,
                         UV_UDP_REUSEADDR);

  if (rval != 0) {
    fprintf(stderr, "uv error: %s\n", uv_strerror(rval));
    return rval;
  }

  uv_udp_recv_start(sess->handle, alloc_cb, *on_recv);
  // NOTE: it seems setting options before uv_udp_recv_start won't work
  uv_send_buffer_size(sess->handle, &kcpuv_udp_buf_size);
  uv_recv_buffer_size(sess->handle, &kcpuv_udp_buf_size);
}

// Stop listening
int kcpuv_stop_listen(kcpuv_sess *sess) {
  return uv_udp_recv_stop(sess->handle);
}

// Set close msg listener
void kcpuv_bind_close(kcpuv_sess *sess, kcpuv_close_cb cb) {
  sess->on_close_cb = cb;
}

// Sessions won't receive msg anymore after closed.
// We still need to send the close msg to the other side
// and then we can free the sess.
void kcpuv_close(kcpuv_sess *sess, unsigned int send_close_msg) {
  // mark that this sess could be freed
  sess->is_closed = 1;

  if (send_close_msg != 0) {
    // send an packet with empty content
    // TODO: Is there any better choices?
    char *content = "a";
    // TODO: this message may not be sent
    kcpuv_send(sess, content, 1);
    // IUINT32 now = iclock();
    // ikcp_update(sess, now);
  }

  // call callback to inform outside
  if (sess->on_close_cb != NULL) {
    kcpuv_close_cb on_close_cb = sess->on_close_cb;
    on_close_cb(sess);
  }
}
