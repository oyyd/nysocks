#include "kcpuv_sess.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// kpcuv_sess
static kcpuv_sess_list *sess_list = NULL;
static char *buffer = NULL;
// kcpuv_sess extern
// TODO: it only accepts int in uv.h
long kcpuv_udp_buf_size = 4 * 1024 * 1024;

// NOTE: Use this after first creation.
kcpuv_sess_list *kcpuv_get_sess_list() { return sess_list; }

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
}

int kcpuv_destruct() {
  if (sess_list == NULL) {
    return 0;
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

// Send content through kcp.
void kcpuv_send(kcpuv_sess *sess, const char *msg, unsigned long len) {
  unsigned long s = 0;

  while (s < len) {
    unsigned long e = s + MAX_SENDING_LEN;

    if (e > len) {
      e = len;
    }

    unsigned long part_len = e - s;

    int rval = ikcp_send(sess->kcp, msg + s, part_len);

    if (KCPUV_DEBUG == 1 && rval < 0) {
      // TODO:
      printf("ikcp_send() < 0: %d", rval);
    }

    s = e;
  }
}

// TODO: We don't need a callback to notify the end of sending now.
static void sess_send(kcpuv_sess *sess, char *data, int length,
                      kcpuv_dgram_cb cb, char *cus_data) {
  // make buffers from string
  uv_buf_t buf = uv_buf_init(data, length);

  if (sess->state != KCPUV_STATE_FREED) {
    if (sess->udp_send != NULL) {
      kcpuv_udp_send udp_send = sess->udp_send;
      udp_send(sess, &buf, 1, (const struct sockaddr *)sess->send_addr);
    } else {
      uv_udp_try_send(sess->handle, &buf, 1,
                      (const struct sockaddr *)sess->send_addr);
    }

    if (cb != NULL) {
      cb(sess, cus_data);
    }
  }

  free(data);
}

static void kcpuv_send_with_protocol(kcpuv_sess *sess, int cmd, const char *msg,
                                     int len, kcpuv_dgram_cb cb,
                                     char *cus_data) {
  sess->send_ts = iclock();
  // encode protocol
  int write_len = len + KCPUV_OVERHEAD;
  // ikcp assume we copy and send the msg
  char *plaintext = malloc(sizeof(char) * write_len);

  kcpuv_protocol_encode(cmd, plaintext);

  if (len != 0 && msg != NULL) {
    memcpy(plaintext + KCPUV_OVERHEAD, msg, len);
  }

  // encrypt
  char *data = (char *)kcpuv_cryptor_encrypt(
      sess->cryptor, (unsigned char *)plaintext, &write_len);

  free(plaintext);

  sess_send(sess, data, write_len, cb, cus_data);
}

// Func to output data for kcp through udp.
// NOTE: Should call `kcpuv_init_send` with the session before kcp_output
// TODO: do not allocate twice
static int kcp_output(const char *msg, int len, ikcpcb *kcp, void *user) {
  if (KCPUV_DEBUG) {
    kcpuv__print_sockaddr(((kcpuv_sess *)user)->send_addr);
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
void kcpuv_send_cmd(kcpuv_sess *sess, const int cmd, kcpuv_dgram_cb cb,
                    void *data) {
  kcpuv_send_with_protocol(sess, cmd, NULL, 0, cb, data);
}

// Create a kcpuv session. This is a common structure for
// both of the sending and receiving.
// A session could only have one recv_addr and send_addr.
// TODO: conv
kcpuv_sess *kcpuv_create() {
  IUINT32 now = iclock();

  kcpuv_sess *sess = malloc(sizeof(kcpuv_sess));
  // NOTE: Do not use stream mode.
  sess->kcp = ikcp_create(0, sess);
  // ikcp_nodelay(sess->kcp, 0, 10, 0, 0);
  ikcp_nodelay(sess->kcp, 1, 10, 2, 1);
  ikcp_wndsize(sess->kcp, INIT_WND_SIZE, INIT_WND_SIZE);
  ikcp_setmtu(sess->kcp, MTU_DEF);
  // sess->kcp->rmt_wnd = INIT_WND_SIZE;

  sess->handle = malloc(sizeof(uv_udp_t));
  uv_udp_init(kcpuv_get_loop(), sess->handle);

  sess->mux = NULL;
  sess->handle->data = sess;
  sess->send_addr = NULL;
  sess->recv_addr = NULL;
  sess->on_msg_cb = NULL;
  sess->on_close_cb = NULL;
  sess->udp_send = NULL;
  sess->state = KCPUV_STATE_CREATED;
  sess->recv_ts = now;
  sess->send_ts = now;
  sess->timeout = DEFAULT_TIMEOUT;
  sess->cryptor = NULL;

  // set output func for kcp
  sess->kcp->output = kcp_output;

  // create link and push to the queue
  kcpuv_link *link = kcpuv_link_create(sess);
  kcpuv_link_add(sess_list->list, link);

  sess_list->len += 1;

  return sess;
}

// TODO: export salt
void kcpuv_sess_init_cryptor(kcpuv_sess *sess, const char *key, int len) {
  unsigned int salt[] = {1, 2};
  sess->cryptor = malloc(sizeof(kcpuv_cryptor));
  kcpuv_cryptor_init(sess->cryptor, key, len, salt);
}

// Free a kcpuv session.
void kcpuv_free(kcpuv_sess *sess) {
  sess->state = KCPUV_STATE_FREED;

  if (sess_list != NULL && sess_list->list != NULL) {
    kcpuv_link *ptr = kcpuv_link_get_pointer(sess_list->list, sess);
    if (ptr != NULL) {
      free(ptr);
    }
    sess_list->len -= 1;
  }

  if (sess->cryptor != NULL) {
    kcpuv_cryptor_clean(sess->cryptor);
    free(sess->cryptor);
  }

  if (sess->send_addr != NULL) {
    free(sess->send_addr);
  }
  if (sess->recv_addr != NULL) {
    free(sess->recv_addr);
  }

  // TODO: should stop listening
  if (!uv_is_closing((uv_handle_t *)sess->handle)) {
    uv_close((uv_handle_t *)sess->handle, free_handle_cb);
  } else {
    free(sess->handle);
  }

  ikcp_release(sess->kcp);
  free(sess);
}

// int kcpuv_get_last_packet_addr(kcpuv_sess *sess, char *name, int *port) {
//   if (sess->last_packet_addr == NULL) {
//     return 0;
//   }
//
//   if (sess->last_packet_addr->sa_family == AF_INET) {
//     uv_ip4_name((const struct sockaddr_in *)sess->last_packet_addr, name,
//                 IP4_ADDR_LENTH);
//     *port = ntohs(((struct sockaddr_in *)sess->last_packet_addr)->sin_port);
//     return IP4_ADDR_LENTH;
//   }
//
//   uv_ip6_name((const struct sockaddr_in6 *)sess->last_packet_addr, name,
//               IP6_ADDR_LENGTH);
//   *port = ntohs(((struct sockaddr_in6 *)sess->last_packet_addr)->sin6_port);
//   return IP6_ADDR_LENGTH;
// }

static void init_send(kcpuv_sess *sess, const struct sockaddr *addr) {
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
  int rval = ikcp_input(sess->kcp, msg, length);

  if (KCPUV_DEBUG == 1 && rval < 0) {
    // TODO:
    fprintf(stderr, "ikcp_input() < 0: %d\n", rval);
  }

  return rval;
}

// Input dgram mannualy
void kcpuv_input(kcpuv_sess *sess, ssize_t nread, const uv_buf_t *buf,
                 const struct sockaddr *addr) {
  if (nread < 0) {
    // TODO:
    fprintf(stderr, "uv error: %s\n", uv_strerror(nread));
  }

  if (nread > 0) {
    if (sess->state == KCPUV_STATE_WAIT_ACK) {
      sess->state = KCPUV_STATE_READY;
      init_send(sess, addr);
      if (KCPUV_DEBUG) {
        kcpuv__print_sockaddr(addr);
      }
    }

    int read_len = nread;
    char *read_msg = (char *)kcpuv_cryptor_decrypt(
        sess->cryptor, (unsigned char *)buf->base, &read_len);

    if (KCPUV_DEBUG) {
      printf("input: %lld\n", iclock64());
      print_as_hex(read_msg, read_len);
      printf("%s\n", read_msg);
    }

    // update active time
    sess->recv_ts = iclock();

    // check protocol
    int cmd = kcpuv_protocol_decode(read_msg);

    // NOTE: We don't need to make sure the KCPUV_CMD_NOO
    // to be received as messages sended by kcp will also
    // refresh the recv_ts to avoid timeout.
    if (cmd == KCPUV_CMD_NOO) {
      free(read_msg);
      free(buf->base);
      return;
    } else if (cmd == KCPUV_CMD_CLS) {
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

// Bind a proxy function for sending udp.
void kcpuv_bind_udp_send(kcpuv_sess *sess, kcpuv_udp_send udp_send) {
  sess->udp_send = udp_send;
}

// Called when uv receives a msg and pass.
static void on_recv(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf,
                    const struct sockaddr *addr, unsigned flags) {
  kcpuv_sess *sess = (kcpuv_sess *)handle->data;
  kcpuv_input(sess, nread, buf, addr);
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
  uv_send_buffer_size((uv_handle_t *)sess->handle, &kcpuv_udp_buf_size);
  uv_recv_buffer_size((uv_handle_t *)sess->handle, &kcpuv_udp_buf_size);
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
void kcpuv_bind_close(kcpuv_sess *sess, kcpuv_dgram_cb cb) {
  sess->on_close_cb = cb;
}

void kcpuv_bind_listen(kcpuv_sess *sess, kcpuv_listen_cb cb) {
  sess->on_msg_cb = cb;
}

// TODO: state condition
int kcpuv_set_state(kcpuv_sess *sess, int state) {
  if (state != KCPUV_STATE_WAIT_FREE && sess->state == KCPUV_STATE_WAIT_FREE) {
    return -1;
  }

  sess->state = KCPUV_STATE_WAIT_FREE;
  return 0;
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
  // uv_close(sess->handle, NULL);

  if (send_close_msg != 0) {
    kcpuv_send_cmd(sess, KCPUV_CMD_CLS, sess->on_close_cb, (void *)error_msg);
  } else if (sess->on_close_cb != NULL) {
    // call callback to inform outside
    kcpuv_dgram_cb on_close_cb = sess->on_close_cb;
    on_close_cb(sess, (void *)error_msg);
  }
}

// Iterate the session_list and update kcp
void kcpuv__update_kcp_sess(uv_timer_t *timer) {
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

    if (KCPUV_SESS_TIMEOUT) {
      if (sess->timeout && now - sess->recv_ts >= sess->timeout) {
        kcpuv_close(sess, 1, "timeout");
        continue;
      }
    }

    IUINT32 time_to_update = ikcp_check(sess->kcp, now);

    // NOTE: We have to call ikcp_update after the first calling
    // of the ikcp_input or we will get Segmentation fault.
    if (time_to_update <= now) {
      ikcp_update(sess->kcp, now);
    }

    size = ikcp_recv(sess->kcp, buffer, BUFFER_LEN);
    // TODO: consider the expected size
    while (size > 0) {
      if (sess->on_msg_cb == NULL) {
        continue;
      }

      // update receive data
      kcpuv_listen_cb on_msg_cb = sess->on_msg_cb;
      on_msg_cb(sess, (const char *)buffer, size);
      size = ikcp_recv(sess->kcp, buffer, BUFFER_LEN);
    }

    if (size < 0) {
      // rval == -1 queue is empty
      // rval == -2 peeksize is zero(or invalid)
      if (size != -1 && size != -2) {
        // TODO:
        fprintf(stderr, "ikcp_recv() < 0: %d\n", size);
      }
    }
  }

  ptr = sess_list->list;

  while (ptr->next != NULL) {
    kcpuv_sess *sess = (kcpuv_sess *)ptr->next->node;

    // free the sess if marked
    if (sess->state == KCPUV_STATE_WAIT_FREE) {
      kcpuv_free(sess);
    } else {
      if (KCPUV_SESS_HEARTBEAT_ACTIVE) {
        if (sess->send_addr != NULL &&
            sess->send_ts + KCPUV_SESS_HEARTBEAT_INTERVAL <= now) {
          kcpuv_send_cmd(sess, KCPUV_CMD_NOO, NULL, NULL);
        }
      }
      ptr = ptr->next;
    }
  }
}
