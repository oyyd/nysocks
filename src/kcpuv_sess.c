#include "kcpuv_sess.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// kpcuv_sess
static int use_default_loop = 0;
static kcpuv_sess_list *sess_list = NULL;
static char *buffer = NULL;
static uv_idle_t *idler = NULL;
// kcpuv_sess extern
// TODO: it only accepts int in uv.h
long kcpuv_udp_buf_size = 4 * 1024 * 1024;
uv_loop_t *kcpuv_loop = NULL;

// NOTE: Use this after first creation.
kcpuv_sess_list *kcpuv_get_sess_list() { return sess_list; }

// NOTE: Don't change this while looping.
void kcpuv_use_default_loop(int value) { use_default_loop = value; }

void kcpuv_initialize() {
  if (sess_list != NULL) {
    return;
  }

  // check and create list
  sess_list = malloc(sizeof(kcpuv_sess_list));
  sess_list->list = kcpuv_link_create(NULL);
  sess_list->len = 0;

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

int kcpuv_destruct() {
  if (sess_list == NULL) {
    return 0;
  }

  if (!use_default_loop && kcpuv_loop != NULL) {
    // NOTE: The closing may failed simply because
    // we don't call the `uv_run` on this loop and
    // then causes a memory leaking.
    int rval = uv_loop_close(kcpuv_loop);
    // if (rval) {
    //   fprintf(stderr, "failed to close the loop: %s\n", uv_strerror(rval));
    //   return -1;
    // }

    free(kcpuv_loop);
    kcpuv_loop = NULL;
  }

  if (buffer != NULL) {
    free(buffer);
    buffer = NULL;
  }

  kcpuv_link *ptr = sess_list->list;
  kcpuv_link *ptr_next = ptr->next;

  // destruct all nodes
  while (ptr_next != NULL) {
    kcpuv_free(ptr_next->node);
    ptr_next = ptr->next;
  }

  free(sess_list->list);
  free(sess_list);
  sess_list = NULL;
  return 0;
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

    if (sess->timeout && now - sess->recv_ts >= sess->timeout) {
      kcpuv_close(sess, 1, "timeout");
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
  if (idler != NULL) {
    uv_idle_stop(idler);
    free(idler);
  }

  if (!use_default_loop && kcpuv_loop != NULL) {
    uv_stop(kcpuv_loop);
  }
}

// Send content through kcp.
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
    free(data);
  }
}

// TODO: should check status?
static void send_cb(uv_udp_send_t *req, int status) {
  kcpuv_send_cb_data *cb_data = (kcpuv_send_cb_data *)req->data;
  // free buffer and request
  free(cb_data->buf_data);
  free(req);

  kcpuv_close_cb cb = cb_data->cb;
  if (cb != NULL) {
    cb(cb_data->sess, cb_data->cus_data);
  }

  free(cb_data);
}

static void udp_send(kcpuv_sess *sess, char *data, int length,
                     kcpuv_close_cb cb, char *cus_data) {
  kcpuv_send_cb_data *cb_data = malloc(sizeof(kcpuv_send_cb_data));
  cb_data->buf_data = data;
  cb_data->cb = cb;
  cb_data->sess = sess;
  cb_data->cus_data = cus_data;

  uv_udp_send_t *req = malloc(sizeof(uv_udp_send_t));
  // for freeing the buf latter
  req->data = cb_data;

  // make buffers from string
  uv_buf_t buf = uv_buf_init(data, length);

  uv_udp_send(req, sess->handle, &buf, 1,
              (const struct sockaddr *)sess->send_addr, &send_cb);
}

static void kcpuv_send_with_protocol(kcpuv_sess *sess, int cmd, const char *msg,
                                     int len, kcpuv_close_cb cb,
                                     char *cus_data) {
  // encode protocol
  int write_len = len + KCPUV_OVERHEAD;
  // ikcp assume we copy and send the msg
  char *plaintext = malloc(sizeof(char) * write_len);

  kcpuv_protocol_encode(cmd, plaintext);

  if (len != 0) {
    memcpy(plaintext + KCPUV_OVERHEAD, msg, len);
  }

  // encrypt
  char *data = (char *)kcpuv_cryptor_encrypt(
      sess->cryptor, (unsigned char *)plaintext, &write_len);

  free(plaintext);

  udp_send(sess, data, write_len, cb, cus_data);
}

// Func to output data for kcp through udp.
// NOTE: Should call `kcpuv_init_send` with the session before udp_output
// TODO: do not allocate twice
static int udp_output(const char *msg, int len, ikcpcb *kcp, void *user) {
  if (debug) {
    printf("output: %d %lld\n", len, iclock64());
    printf("content: ");
    print_as_hex(msg, len);
    printf("\n");
  }

  kcpuv_sess *sess = (kcpuv_sess *)user;

  kcpuv_send_with_protocol(sess, KCPUV_CMD_PUSH, msg, len, NULL, NULL);

  return 0;
}

// Send cmds directly through udp.
void kcpuv_send_cmd(kcpuv_sess *sess, const int cmd, kcpuv_close_cb cb,
                    void *data) {
  kcpuv_send_with_protocol(sess, cmd, NULL, 0, cb, data);
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
  ikcp_wndsize(sess->kcp, INIT_WND_SIZE, INIT_WND_SIZE);
  ikcp_setmtu(sess->kcp, MTU_DEF);
  // sess->kcp->rmt_wnd = INIT_WND_SIZE;
  // sess->kcp->stream = 1;

  sess->handle = malloc(sizeof(uv_udp_t));
  uv_udp_init(kcpuv_loop, sess->handle);

  sess->handle->data = sess;
  sess->send_addr = NULL;
  sess->recv_addr = NULL;
  sess->last_packet_addr = NULL;
  sess->on_msg_cb = NULL;
  sess->on_close_cb = NULL;
  sess->save_last_packet_addr = 0;
  sess->state = KCPUV_STATE_CREATED;
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
    kcpuv_link *ptr = kcpuv_link_get_pointer(sess_list->list, sess);
    if (ptr != NULL) {
      free(ptr);
    }
    sess_list->len -= 1;
  }

  kcpuv_cryptor_clean(sess->cryptor);
  free(sess->cryptor);

  if (sess->last_packet_addr != NULL) {
    free(sess->last_packet_addr);
  }

  if (sess->send_addr != NULL) {
    free(sess->send_addr);
  }
  if (sess->recv_addr != NULL) {
    free(sess->recv_addr);
  }

  // TODO: should stop listening
  free(sess->handle);
  ikcp_release(sess->kcp);
  free(sess);
}

void kcpuv_set_save_last_packet_addr(kcpuv_sess *sess, unsigned short value) {
  sess->save_last_packet_addr = value;
}

int kcpuv_get_last_packet_addr(kcpuv_sess *sess, char *name, int *port) {
  if (sess->last_packet_addr == NULL) {
    return 0;
  }

  if (sess->last_packet_addr->sa_family == AF_INET) {
    uv_ip4_name((const struct sockaddr_in *)sess->last_packet_addr, name,
                IP4_ADDR_LENTH);
    *port = ntohs(((struct sockaddr_in *)sess->last_packet_addr)->sin_port);
    return IP4_ADDR_LENTH;
  }

  uv_ip6_name((const struct sockaddr_in6 *)sess->last_packet_addr, name,
              IP6_ADDR_LENGTH);
  *port = ntohs(((struct sockaddr_in6 *)sess->last_packet_addr)->sin6_port);
  return IP6_ADDR_LENGTH;
}

void init_send(kcpuv_sess *sess, const struct sockaddr *addr) {
  if (sess->send_addr != NULL) {
    free(sess->send_addr);
  }

  sess->send_addr = malloc(sizeof(struct sockaddr));
  memcpy(sess->send_addr, addr, sizeof(struct sockaddr));
}

// Set sending info.
void kcpuv_init_send(kcpuv_sess *sess, char *addr, int port) {
  struct sockaddr addr_info;

  uv_ip4_addr(addr, port, (struct sockaddr_in *)&addr_info);
  init_send(sess, &addr_info);
}

// Update kcp for content transmission.
static int input_kcp(kcpuv_sess *sess, const char *msg, int length) {
  // update active time
  sess->recv_ts = iclock();

  int rval = ikcp_input(sess->kcp, msg, length);

  if (debug == 1 && rval < 0) {
    // TODO:
    fprintf(stderr, "ikcp_input() < 0: %d", rval);
  }
}

// Called when uv receives a msg and pass.
static void on_recv(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf,
                    const struct sockaddr *addr, unsigned flags) {
  if (nread < 0) {
    // TODO:
    fprintf(stderr, "uv error: %s\n", uv_strerror(nread));
  }

  if (nread > 0) {
    kcpuv_sess *sess = (kcpuv_sess *)handle->data;

    if (sess->state == KCPUV_STATE_WAIT_ACK) {
      sess->state = KCPUV_STATE_READY;
      init_send(sess, addr);
    }

    if (sess->save_last_packet_addr) {
      if (sess->last_packet_addr == NULL) {
        sess->last_packet_addr = malloc(sizeof(struct sockaddr));
      }

      memcpy(sess->last_packet_addr, addr, sizeof(struct sockaddr));
    }

    int read_len = nread;
    char *read_msg = (char *)kcpuv_cryptor_decrypt(
        sess->cryptor, (unsigned char *)buf->base, &read_len);

    if (debug) {
      printf("input: %lld\n", iclock64());
      print_as_hex(read_msg, read_len);
      printf("%s\n", read_msg);
    }

    // check protocol
    int cmd = kcpuv_protocol_decode(read_msg);
    if (cmd == KCPUV_CMD_CLS) {
      kcpuv_close(sess, 0, NULL);
      free(read_msg);
      free(buf->base);
      return;
    }

    input_kcp(sess, (const char *)(read_msg + KCPUV_OVERHEAD),
              read_len - KCPUV_OVERHEAD);
    free(read_msg);
  }

  // release uv buf
  free(buf->base);
}

// Set receiving info.
int kcpuv_listen(kcpuv_sess *sess, int port, kcpuv_listen_cb cb) {
  sess->state = KCPUV_STATE_WAIT_ACK;
  sess->on_msg_cb = cb;
  // get local addr
  sess->recv_addr = malloc(sizeof(struct sockaddr));
  uv_ip4_addr("0.0.0.0", port, (struct sockaddr_in *)sess->recv_addr);

  // bind local and start recv
  int rval =
      uv_udp_bind(sess->handle, (const struct sockaddr *)sess->recv_addr, 0);

  if (rval != 0) {
    fprintf(stderr, "uv error: %s\n", uv_strerror(rval));
    return rval;
  }

  uv_udp_recv_start(sess->handle, alloc_cb, *on_recv);
  // NOTE: it seems setting options before uv_udp_recv_start won't work
  uv_send_buffer_size(sess->handle, &kcpuv_udp_buf_size);
  uv_recv_buffer_size(sess->handle, &kcpuv_udp_buf_size);
  return 0;
}

// Stop listening
int kcpuv_stop_listen(kcpuv_sess *sess) {
  return uv_udp_recv_stop(sess->handle);
}

// Get address and port of a sess.
int kcpuv_get_address(kcpuv_sess *sess, char *addr, int *namelen, int *port) {
  // Assume their `sa_family`es are all ipv4.
  struct sockaddr *name = malloc(sizeof(struct sockaddr));
  *namelen = sizeof(struct sockaddr);
  int rval = uv_udp_getsockname((const uv_udp_t *)sess->handle,
                                (struct sockaddr *)name, namelen);

  if (rval) {
    free(name);
    return rval;
  }

  if (name->sa_family == AF_INET) {
    uv_ip4_name((const struct sockaddr_in *)name, addr, IP4_ADDR_LENTH);
    *port = ntohs(((struct sockaddr_in *)name)->sin_port);
  } else {
    uv_ip6_name((const struct sockaddr_in6 *)name, addr, IP6_ADDR_LENGTH);
    *port = ntohs(((struct sockaddr_in6 *)name)->sin6_port);
  }

  free(name);
  return 0;
}

// Set close msg listener
void kcpuv_bind_close(kcpuv_sess *sess, kcpuv_close_cb cb) {
  sess->on_close_cb = cb;
}

// Sessions won't receive msg anymore after closed.
// We still need to send the close msg to the other side
// and then we can free the sess.
// NOTE: Users are expected to `kcpuv_free` sessions manually.
// 1. bindClose 2. close 3. callback
void kcpuv_close(kcpuv_sess *sess, unsigned int send_close_msg,
                 const char *error_msg) {
  // mark that this sess could be freed
  sess->state = KCPUV_STATE_CLOSED;

  if (send_close_msg != 0) {
    kcpuv_send_cmd(sess, KCPUV_CMD_CLS, sess->on_close_cb, error_msg);
    // // send an packet with empty content
    // // TODO: Is there any better choices?
    // char *content = "a";
    // // TODO: this message may not be sent
    // kcpuv_send(sess, content, 1);
    // // IUINT32 now = iclock();
    // // ikcp_update(sess, now);
  } else if (sess->on_close_cb != NULL) {
    // call callback to inform outside
    kcpuv_close_cb on_close_cb = sess->on_close_cb;
    on_close_cb(sess, error_msg);
  }
}
