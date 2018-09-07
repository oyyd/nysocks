#include "KcpuvSess.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace kcpuv {
// kpcuv_sess
static kcpuv_sess_list *sess_list = NULL;
static char *buffer = NULL;
// KcpuvSess extern
// TODO: it only accepts int in uv.h
long kcpuv_udp_buf_size = 4 * 1024 * 1024;

static short enable_timeout = KCPUV_SESS_TIMEOUT;

// int kcpuv__should_output_buf(unsigned int recvBufLength,
//                              unsigned int content_length) {
//   return ((long)(recvBufLength + content_length)) >= KCPUV_SESS_BUFFER_SIZE;
// }
//
// void kcpuv__replace_buf(KcpuvSess *sess, const char *content,
//                         unsigned int content_length) {
//   recvBufLength = content_length;
//   memcpy(recvBuf, content, content_length);
// }
//
// void kcpuv__put_buf(KcpuvSess *sess, const char *content,
//                     unsigned int content_length) {
//   memcpy(recvBuf + recvBufLength, content, content_length);
//   recvBufLength += content_length;
// }
//
// void KcpuvPrintSessList_() {
//   if (sess_list == NULL) {
//     fprintf(stderr, "sess_list_length: 0\n");
//   }
//
//   kcpuv_link *link = sess_list->list;
//   int count = 0;
//
//   while (link != NULL) {
//     count++;
//     link = link->next;
//   }
//
//   fprintf(stderr, "sess_list_length: %d\n", count - 1);
// }

void KcpuvSess::KcpuvSessEnableTimeout(short value) { enable_timeout = value; }

// NOTE: Use this after first creation.
kcpuv_sess_list *KcpuvSess::KcpuvGetSessList() { return sess_list; }

void KcpuvSess::KcpuvInitialize() {
  if (sess_list != NULL) {
    return;
  }

  // check and create list
  sess_list = new kcpuv_sess_list;
  sess_list->list = kcpuv_link_create(NULL);
  sess_list->len = 0;

  // init buffer
  buffer = new char[BUFFER_LEN];
}

int KcpuvSess::KcpuvDestruct() {
  if (sess_list == NULL) {
    return 0;
  }

  if (buffer != NULL) {
    delete buffer;
    buffer = NULL;
  }

  kcpuv_link *ptr = sess_list->list;
  kcpuv_link *ptr_next = ptr->next;

  // destruct all nodes
  while (ptr_next != NULL) {
    KcpuvSess *sess = reinterpret_cast<KcpuvSess *>(ptr_next->node);
    delete sess;
    ptr_next = ptr->next;
  }

  delete sess_list->list;
  delete sess_list;
  sess_list = NULL;

  return 0;
}

// // Send raw data through kcp.
// void kcpuv_raw_send(KcpuvSess *sess, const int cmd, const char *msg,
//                     unsigned long len) {
//   if (KcpuvIsFreed(sess)) {
//     if (KCPUV_DEBUG) {
//       fprintf(stderr, "input with invalid state");
//     }
//     return;
//   }
//
//   sendTs = iclock();
//
//   // encode protocol
//   // int write_len = len + KCPUV_OVERHEAD;
//   //
//   // // ikcp assume we copy and send the msg
//   // char *plaintext = malloc(sizeof(char) * write_len);
//   //
//   // kcpuv_protocol_encode(cmd, plaintext);
//   //
//   // if (len != 0 && msg != NULL) {
//   //   memcpy(plaintext + KCPUV_OVERHEAD, msg, len);
//   // }
//
//   // split content and send
//   unsigned long s = 0;
//
//   while (s == 0 || s < len) {
//     // The position that we have send the data before it.
//     unsigned long e = s + MAX_SENDING_LEN - KCPUV_OVERHEAD;
//
//     if (e > len) {
//       e = len;
//     }
//
//     unsigned long part_len = e - s;
//
//     char *plaintext = malloc(sizeof(char) * (part_len + KCPUV_OVERHEAD));
//     kcpuv_protocol_encode(cmd, plaintext);
//
//     if (part_len != 0 && msg != NULL) {
//       memcpy(plaintext + KCPUV_OVERHEAD, msg + s, part_len);
//     }
//
//     int rval = ikcp_send(kcp, plaintext, part_len + KCPUV_OVERHEAD);
//
//     if (KCPUV_DEBUG == 1 && rval < 0) {
//       // TODO:
//       printf("ikcp_send() < 0: %d", rval);
//     }
//
//     s = e;
//     free(plaintext);
//
//     if (s == 0) {
//       break;
//     }
//   }
// }
//
// // Send app data through kcp.
// void KcpuvSend(KcpuvSess *sess, const char *msg, unsigned long len) {
//   kcpuv_raw_send(sess, KCPUV_CMD_PUSH, msg, len);
// }

// Func to output data for kcp through udp.
// NOTE: Should call `KcpuvInitSend` with the session before kcp_output
// TODO: do not allocate twice
// static int kcp_output(const char *msg, int len, ikcpcb *kcp, void *user) {
//   KcpuvSess *sess = (KcpuvSess *)user;
//
//   if (sess->sendAddr == NULL) {
//     // fprintf(stderr, "%s\n", "NULL sendAddr");
//     return -1;
//   }
//
//   if (KcpuvIsFreed(sess)) {
//     fprintf(stderr, "%s\n", "output with invalid state");
//     return -1;
//   }
//
//   if (KCPUV_DEBUG) {
//     // kcpuv__print_sockaddr(((KcpuvSess *)user)->sendAddr);
//     printf("output: %d %lld\n", len, iclock64());
//     printf("content: ");
//     print_as_hex(msg, len);
//     printf("\n");
//   }
//
//   // encrypt
//   char *data =
//       (char *)kcpuv_cryptor_encrypt(sess->cryptor, (unsigned char *)msg,
//       &len);
//
//   // make buffers from string
//   uv_buf_t buf = uv_buf_init(data, len);
//
//   if (sess->state != KCPUV_STATE_FREED) {
//     if (sess->udpSend != NULL) {
//       kcpuv_udp_send udpSend = sess->udpSend;
//       udpSend(sess, &buf, 1, (const struct sockaddr *)sess->sendAddr);
//     } else {
//       uv_udp_try_send(sess->handle, &buf, 1,
//                       (const struct sockaddr *)sess->sendAddr);
//     }
//   }
//
//   delete buf.base;
//
//   return 0;
// }
//
// // Send cmds.
// void KcpuvSendCMD(KcpuvSess *sess, const int cmd) {
//   kcpuv_raw_send(sess, cmd, NULL, 0);
// }

// Create a kcpuv session. This is a common structure for
// both of the sending and receiving.
// A session could only have one recvAddr and sendAddr.
KcpuvSess::KcpuvSess() {
  IUINT32 now = iclock();

  // KcpuvSess *sess = malloc(sizeof(KcpuvSess));
  kcp = ikcp_create(0, this);
  // NOTE: Support stream mode
  // kcp->stream = 1;
  // ikcp_nodelay(kcp, 0, 10, 0, 0);
  ikcp_nodelay(kcp, 1, 10, 2, 1);
  ikcp_wndsize(kcp, INIT_WND_SIZE, INIT_WND_SIZE);
  ikcp_setmtu(kcp, MTU_DEF);
  // kcp->rmt_wnd = INIT_WND_SIZE;

  // NOTE: uv_handle_t will be freed automatically
  handle = new uv_udp_t;
  uv_udp_init(Loop::kcpuv_get_loop(), handle);

  recvBufLength = 0;
  recvBuf = new char[KCPUV_SESS_BUFFER_SIZE];
  mux = NULL;
  handle->data = this;
  sendAddr = NULL;
  recvAddr = NULL;
  onMsgCb = NULL;
  onCloseCb = NULL;
  udpSend = NULL;
  state = KCPUV_STATE_CREATED;
  recvTs = now;
  sendTs = now;
  timeout = DEFAULT_TIMEOUT;
  cryptor = NULL;
  onBeforeFree = NULL;

  // TODO:
  // set output func for kcp
  // kcp->output = kcp_output;

  // create link and push to the queue
  kcpuv_link *link = kcpuv_link_create(this);
  kcpuv_link_add(sess_list->list, link);

  sess_list->len += 1;
}

int KcpuvSess::KcpuvIsFreed(KcpuvSess *sess) {
  return sess->state == KCPUV_STATE_FREED;
}

// Free a kcpuv session.
KcpuvSess::~KcpuvSess() {
  if (KcpuvIsFreed(this)) {
    fprintf(stderr, "%s\n", "sess have been freed");
    return;
  }

  state = KCPUV_STATE_FREED;

  if (onCloseCb != NULL) {
    // call callback to inform outside
    kcpuv_dgram_cb onCloseCb = onCloseCb;
    onCloseCb(this);
  }

  if (sess_list != NULL && sess_list->list != NULL) {
    kcpuv_link *ptr = kcpuv_link_get_pointer(sess_list->list, this);
    if (ptr != NULL) {
      delete ptr;
    }
    sess_list->len -= 1;
  }

  if (cryptor != NULL) {
    Cryptor::KcpuvCryptorClean(cryptor);
    delete cryptor;
  }

  if (sendAddr != NULL) {
    delete sendAddr;
  }
  if (recvAddr != NULL) {
    delete recvAddr;
  }

  if (!uv_is_closing((uv_handle_t *)handle)) {
    uv_close((uv_handle_t *)handle, free_handle_cb);
  }

  // // TODO: should stop listening
  // if (handle->data != NULL) {
  //   // freed
  // } else if (!uv_is_closing((uv_handle_t *)handle)) {
  // } else {
  //   // handle->data = NULL;
  //   // free(handle);
  // }

  delete recvBuf;
  ikcp_release(kcp);
}

// TODO: export salt
void KcpuvSess::KcpuvSessInitCryptor(KcpuvSess *sess, const char *key,
                                     int len) {
  unsigned int salt[] = {1, 2};
  sess->cryptor = new kcpuv_cryptor;
  Cryptor::KcpuvCryptorInit(sess->cryptor, key, len, salt);
}

// int kcpuv_get_last_packet_addr(KcpuvSess *sess, char *name, int *port) {
//   if (last_packet_addr == NULL) {
//     return 0;
//   }
//
//   if (last_packet_addr->sa_family == AF_INET) {
//     uv_ip4_name((const struct sockaddr_in *)last_packet_addr, name,
//                 IP4_ADDR_LENTH);
//     *port = ntohs(((struct sockaddr_in *)last_packet_addr)->sin_port);
//     return IP4_ADDR_LENTH;
//   }
//
//   uv_ip6_name((const struct sockaddr_in6 *)last_packet_addr, name,
//               IP6_ADDR_LENGTH);
//   *port = ntohs(((struct sockaddr_in6 *)last_packet_addr)->sin6_port);
//   return IP6_ADDR_LENGTH;
// }

// static void init_send(KcpuvSess *sess, const struct sockaddr *addr) {
//   if (sendAddr != NULL) {
//     free(sendAddr);
//   }
//
//   sendAddr = malloc(sizeof(struct sockaddr));
//   memcpy(sendAddr, addr, sizeof(struct sockaddr));
// }
//
// // Set sending info.
// void KcpuvInitSend(KcpuvSess *sess, char *addr, int port) {
//   struct sockaddr addr_info;
//
//   uv_ip4_addr(addr, port, (struct sockaddr_in *)&addr_info);
//   init_send(sess, &addr_info);
// }
//
// // TODO: remove this
// // Update kcp for content transmission.
// static int input_kcp(KcpuvSess *sess, const char *msg, int length) {
//   int rval = ikcp_input(kcp, msg, length);
//
//   if (KCPUV_DEBUG == 1 && rval < 0) {
//     // TODO:
//     fprintf(stderr, "ikcp_input() < 0: %d\n", rval);
//   }
//
//   return rval;
// }
//
// // Input dgram mannualy
// void KcpuvInput(KcpuvSess *sess, ssize_t nread, const uv_buf_t *buf,
//                 const struct sockaddr *addr) {
//   if (state == KCPUV_STATE_CREATED || KcpuvIsFreed(sess)) {
//     fprintf(stderr, "%s\n", "invalid msg input");
//     return;
//   }
//
//   if (nread < 0) {
//     // TODO:
//     fprintf(stderr, "uv error: %s\n", uv_strerror(nread));
//   }
//
//   if (nread > 0) {
//     if (state == KCPUV_STATE_WAIT_ACK) {
//       state = KCPUV_STATE_READY;
//       init_send(sess, addr);
//       // if (KCPUV_DEBUG) {
//       //   kcpuv__print_sockaddr(addr);
//       // }
//     }
//
//     int read_len = nread;
//     char *read_msg = (char *)kcpuv_cryptor_decrypt(
//         cryptor, (unsigned char *)buf->base, &read_len);
//
//     // if (KCPUV_DEBUG) {
//     //   print_as_hex(buf->base, nread);
//     //   print_as_hex(read_msg, read_len);
//     // }
//
//     // update active time
//     recvTs = iclock();
//
//     input_kcp(sess, (const char *)read_msg, read_len);
//
//     free(read_msg);
//   }
//
//   // release uv buf
//   free(buf->base);
// }
//
// // Bind a proxy function for sending udp.
// void KcpuvBindUdpSend(KcpuvSess *sess, kcpuv_udpSend udpSend) {
//   udpSend = udpSend;
// }
//
// void KcpuvBindBeforeFree(KcpuvSess *sess, kcpuv_dgram_cb onBeforeFree) {
//   onBeforeFree = onBeforeFree;
// }
//
// // Called when uv receives a msg and pass.
// static void on_recv(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf,
//                     const struct sockaddr *addr, unsigned flags) {
//   KcpuvSess *sess = (KcpuvSess *)handle->data;
//   KcpuvInput(sess, nread, buf, addr);
// }
//
// // Set receiving info.
// int KcpuvListen(KcpuvSess *sess, int port, KcpuvListen_cb cb) {
//   state = KCPUV_STATE_WAIT_ACK;
//   onMsgCb = cb;
//   // get local addr
//   recvAddr = malloc(sizeof(struct sockaddr));
//   uv_ip4_addr("0.0.0.0", port, (struct sockaddr_in *)recvAddr);
//
//   // bind local and start recv
//   int rval = uv_udp_bind(handle, (const struct sockaddr *)recvAddr, 0);
//
//   if (rval != 0) {
//     fprintf(stderr, "uv error: %s\n", uv_strerror(rval));
//     return rval;
//   }
//
//   uv_udp_recv_start(handle, alloc_cb, *on_recv);
//   // NOTE: it seems setting options before uv_udp_recv_start won't work
//   uv_send_buffer_size((uv_handle_t *)handle, &kcpuv_udp_buf_size);
//   uv_recvBuffer_size((uv_handle_t *)handle, &kcpuv_udp_buf_size);
//   return 0;
// }
//
// // Stop listening
// int KcpuvStopListen(KcpuvSess *sess) { return uv_udp_recv_stop(handle); }
//
// // Get address and port of a sess.
// int KcpuvGetAddress(KcpuvSess *sess, char *addr, int *namelen, int *port) {
//   // Assume their `sa_family`es are all ipv4.
//   struct sockaddr *name = malloc(sizeof(struct sockaddr));
//   *namelen = sizeof(struct sockaddr);
//   int rval = uv_udp_getsockname((const uv_udp_t *)handle,
//                                 (struct sockaddr *)name, namelen);
//
//   if (rval) {
//     free(name);
//     return rval;
//   }
//
//   if (name->sa_family == AF_INET) {
//     uv_ip4_name((const struct sockaddr_in *)name, addr, IP4_ADDR_LENTH);
//     *port = ntohs(((struct sockaddr_in *)name)->sin_port);
//   } else {
//     uv_ip6_name((const struct sockaddr_in6 *)name, addr, IP6_ADDR_LENGTH);
//     *port = ntohs(((struct sockaddr_in6 *)name)->sin6_port);
//   }
//
//   free(name);
//   return 0;
// }
//
// // Set close msg listener
// void KcpuvBindClose(KcpuvSess *sess, kcpuv_dgram_cb cb) { onCloseCb = cb; }
//
// void KcpuvBindListen(KcpuvSess *sess, KcpuvListen_cb cb) { onMsgCb = cb; }
//
// int KcpuvSetState(KcpuvSess *sess, int state) {
//   // do not allow to change state when it's KCPUV_STATE_WAIT_FREE
//   // if (state != KCPUV_STATE_WAIT_FREE && state == KCPUV_STATE_WAIT_FREE)
//   // {
//   //   return -1;
//   // }
//
//   state = state;
//   return 0;
// }
//
// // Close
// void KcpuvClose(KcpuvSess *sess) {
//   if (state >= KCPUV_STATE_FIN) {
//     return;
//   }
//
//   state = KCPUV_STATE_FIN;
//
//   // TODO: what if a sess is not established?
//   KcpuvSendCMD(sess, KCPUV_CMD_FIN);
//
//   // mark that this sess could be freed
//   // TODO: where to put KCPUV_STATE_CLOSED ?
//   // state = KCPUV_STATE_CLOSED;
//
//   // uv_close(handle, NULL);
// }
//
// static void trigger_before_free(KcpuvSess *sess) {
//   if (onBeforeFree != NULL) {
//     kcpuv_dgram_cb cb = onBeforeFree;
//     cb(sess);
//   }
// }

// Iterate the session_list and update kcp
void KcpuvSess::KcpuvUpdateKcpSess_(uv_timer_t *timer) {
  // TODO:
  // if (!sess_list || !sess_list->list) {
  //   return;
  // }
  //
  // kcpuv_link *ptr = sess_list->list;
  //
  // // TODO: maybe we could assume that ikcp_update won't
  // // cost too much time and get the time once
  // IUINT32 now = 0;
  //
  // while (ptr->next != NULL) {
  //   int size;
  //
  //   if (!ptr->next->node) {
  //     assert(0);
  //   }
  //
  //   KcpuvSess *sess = (KcpuvSess *)ptr->next->node;
  //
  //   now = iclock();
  //
  //   if (enable_timeout) {
  //     if (timeout && now - recvTs >= timeout) {
  //       trigger_before_free(sess);
  //       KcpuvFree(sess, "timeout");
  //       continue;
  //     }
  //   }
  //
  //   IUINT32 time_to_update = ikcp_check(kcp, now);
  //
  //   // NOTE: We have to call ikcp_update after the first calling
  //   // of the ikcp_input or we will get Segmentation fault.
  //   if (time_to_update <= now) {
  //     ikcp_update(kcp, now);
  //   }
  //
  //   size = ikcp_recv(kcp, buffer, BUFFER_LEN);
  //
  //   // TODO: consider the expected size
  //   while (size > 0) {
  //     // print_as_hex((const char *)(buffer), size);
  //     // parse cmd
  //     // check protocol
  //     int cmd = kcpuv_protocol_decode(buffer);
  //
  //     // NOTE: We don't need to make sure the KCPUV_CMD_NOO
  //     // to be received as messages sended by kcp will also
  //     // refresh the recvTs to avoid timeout.
  //     // NOTE: Some states need to be considered carefully.
  //     if (cmd == KCPUV_CMD_NOO) {
  //       // do nothing
  //     } else if (cmd == KCPUV_CMD_FIN) {
  //       if (state <= KCPUV_STATE_READY) {
  //         state = KCPUV_STATE_FIN_ACK;
  //         trigger_before_free(sess);
  //         KcpuvSendCMD(sess, KCPUV_CMD_FIN_ACK);
  //       }
  //     } else if (cmd == KCPUV_CMD_FIN_ACK) {
  //       state = KCPUV_STATE_FIN_ACK;
  //     } else if (cmd == KCPUV_CMD_PUSH) {
  //       unsigned int content_length = size - KCPUV_OVERHEAD;
  //       // fprintf(stderr, "%d %d\n", size, recvBufLength);
  //       // Push the packet into recvBuf.
  //       if (!kcpuv__should_output_buf(recvBufLength, content_length)) {
  //         kcpuv__put_buf(sess, (const char *)(buffer + KCPUV_OVERHEAD),
  //                        content_length);
  //       } else {
  //         if (onMsgCb != NULL) {
  //           // update receive data
  //           KcpuvListen_cb onMsgCb = onMsgCb;
  //           onMsgCb(sess, (const char *)recvBuf, recvBufLength);
  //         }
  //
  //         kcpuv__replace_buf(sess, (const char *)(buffer + KCPUV_OVERHEAD),
  //                            content_length);
  //       }
  //     } else {
  //       // invalid CMD
  //       fprintf(stderr, "receive invalid cmd: %d\n", cmd);
  //     }
  //
  //     size = ikcp_recv(kcp, buffer, BUFFER_LEN);
  //
  //     // TODO: remove
  //     // if (size > 0) {
  //     //   fprintf(stderr, "same_tick\n");
  //     // } else if (size == 0) {
  //     //   fprintf(stderr, "not_same_tick\n");
  //     // }
  //   }
  //
  //   // TODO: Is call next function in the same tick makes it more efficient?
  //   if (recvBufLength > 0) {
  //     if (onMsgCb != NULL) {
  //       // update receive data
  //       KcpuvListen_cb onMsgCb = onMsgCb;
  //       onMsgCb(sess, recvBuf, recvBufLength);
  //     }
  //
  //     kcpuv__replace_buf(sess, NULL, 0);
  //   }
  //
  //   if (size < 0) {
  //     // rval == -1 queue is empty
  //     // rval == -2 peeksize is zero(or invalid)
  //     if (size != -1 && size != -2) {
  //       // TODO:
  //       fprintf(stderr, "ikcp_recv() < 0: %d\n", size);
  //     }
  //   }
  //
  //   ptr = ptr->next;
  // }
  //
  // ptr = sess_list->list;
  //
  // while (ptr->next != NULL) {
  //   if (!ptr->next->node) {
  //     assert(0);
  //   }
  //
  //   KcpuvSess *sess = (KcpuvSess *)ptr->next->node;
  //
  //   if (state == KCPUV_STATE_FIN_ACK) {
  //     int packets = ikcp_waitsnd(kcp);
  //
  //     // Trigger close when all data acked
  //     if (packets == 0) {
  //       trigger_before_free(sess);
  //       // NOTE: kcpuv free will remove ptr
  //       KcpuvFree(sess, NULL);
  //     } else {
  //       ptr = ptr->next;
  //     }
  //   } else {
  //     if (KCPUV_SESS_HEARTBEAT_ACTIVE) {
  //       if (sendAddr != NULL && state == KCPUV_STATE_READY &&
  //           sendTs + KCPUV_SESS_HEARTBEAT_INTERVAL <= now &&
  //           ikcp_waitsnd(kcp) == 0) {
  //         KcpuvSendCMD(sess, KCPUV_CMD_NOO);
  //       }
  //     }
  //     ptr = ptr->next;
  //   }
  // }
}
} // namespace kcpuv
