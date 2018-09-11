#include "KcpuvSess.h"
#include "utils.h"
#include <cassert>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace kcpuv {
// kpcuv_sess
static kcpuv_sess_list *sess_list = NULL;
static char *buffer = NULL;
static short enable_timeout = KCPUV_SESS_TIMEOUT;

// KcpuvSess extern
// TODO: it only accepts int in uv.h
long kcpuv_udp_buf_size = 4 * 1024 * 1024;

static int kcpuv__should_output_buf(unsigned int recvBufLength,
                                    unsigned int content_length) {
  return ((long)(recvBufLength + content_length)) >= KCPUV_SESS_BUFFER_SIZE;
}

static void kcpuv__replace_buf(KcpuvSess *sess, const char *content,
                               unsigned int content_length) {
  sess->recvBufLength = content_length;
  memcpy(sess->recvBuf, content, content_length);
}

static void kcpuv__put_buf(KcpuvSess *sess, const char *content,
                           unsigned int content_length) {
  memcpy(sess->recvBuf + sess->recvBufLength, content, content_length);
  sess->recvBufLength += content_length;
}

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
    // TODO: Inconsistency.
    delete sess;
    ptr_next = ptr->next;
  }

  delete sess_list->list;
  delete sess_list;
  sess_list = NULL;

  return 0;
}

bool KcpuvSess::AllowSend() { return state < KCPUV_STATE_FIN_ACK; }

bool KcpuvSess::AllowInput() { return state < KCPUV_STATE_WAIT_FREE; }

// TODO: Allow outside to know if the operation is successful.
// Send raw data through kcp.
void KcpuvSess::RawSend(const int cmd, const char *msg, unsigned long len) {
  KcpuvSess *sess = this;

  // TODO:
  // if (!sess->AllowSend()) {
  //   return;
  // }

  sess->sendTs = iclock();

  // encode protocol
  // int write_len = len + KCPUV_OVERHEAD;
  //
  // // ikcp assume we copy and send the msg
  // char *plaintext = malloc(sizeof(char) * write_len);
  //
  // Cryptor::KcpuvProtocolEncode(cmd, plaintext);
  //
  // if (len != 0 && msg != NULL) {
  //   memcpy(plaintext + KCPUV_OVERHEAD, msg, len);
  // }

  // split content and send
  unsigned long s = 0;

  while (s == 0 || s < len) {
    // The position that we have send the data before it.
    unsigned long e = s + MAX_SENDING_LEN - KCPUV_OVERHEAD;

    if (e > len) {
      e = len;
    }

    unsigned long part_len = e - s;

    char *plaintext = new char[part_len + KCPUV_OVERHEAD];
    Cryptor::KcpuvProtocolEncode(cmd, plaintext);

    if (part_len != 0 && msg != NULL) {
      memcpy(plaintext + KCPUV_OVERHEAD, msg + s, part_len);
    }
    int rval = ikcp_send(sess->kcp, plaintext, part_len + KCPUV_OVERHEAD);

    if (KCPUV_DEBUG == 1 && rval < 0) {
      // TODO:
      printf("ikcp_send() < 0: %d", rval);
    }

    s = e;
    delete[] plaintext;

    if (s == 0) {
      break;
    }
  }
}

// Send app data through kcp.
void KcpuvSess::Send(const char *msg, unsigned long len) {
  if (!this->AllowSend()) {
    fprintf(stderr, "%s\n", "output with invalid state");
  }

  RawSend(KCPUV_CMD_PUSH, msg, len);
}

// Func to output data for kcp through udp.
// NOTE: Should call `InitSend` with the session before KcpOutput
// TODO: do not allocate twice
static int KcpOutput(const char *msg, int len, ikcpcb *kcp, void *user) {
  KcpuvSess *sess = (KcpuvSess *)user;

  if (KCPUV_DEBUG) {
    printf("output: %d %lld\n", len, iclock64());
    printf("content: ");
    print_as_hex(msg, len);
    printf("\n");
  }

  // encrypt
  char *data = reinterpret_cast<char *>(
      Cryptor::KcpuvCryptorEncrypt(sess->cryptor, (unsigned char *)msg, &len));

  sess->sessUDP->Send(data, len);

  delete[] data;
  return 0;
}

// Send cmd.
void KcpuvSess::SendCMD(const int cmd) { RawSend(cmd, NULL, 0); }

// Create a kcpuv session. This is a common structure for
// both of the sending and receiving.
// A session could only have one recvAddr and sendAddr.
KcpuvSess::KcpuvSess(bool passive_) {
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

  sessUDP = new SessUDP(Loop::kcpuv_get_loop());
  sessUDP->data = this;

  recvBufLength = 0;
  recvBuf = new char[KCPUV_SESS_BUFFER_SIZE];
  mux = NULL;
  // TODO:
  // handle->data = this;
  recvAddr = NULL;
  onMsgCb = NULL;
  onCloseCb = NULL;
  state = KCPUV_STATE_CREATED;
  recvTs = now;
  sendTs = now;
  timeout = DEFAULT_TIMEOUT;
  cryptor = NULL;
  onBeforeFree = NULL;
  passive = 0;
  // NOTE: Take next dgram source as send addr.
  SetPassive(passive_);

  // set output func for kcp
  kcp->output = KcpOutput;

  // create link and push to the queue
  kcpuv_link *link = kcpuv_link_create(this);
  kcpuv_link_add(sess_list->list, link);

  sess_list->len += 1;
}

bool KcpuvSess::ExitUpdateQueue() {
  if (state == KCPUV_STATE_WAIT_FREE) {
    return 0;
  }

  state = KCPUV_STATE_WAIT_FREE;

  // Possible created without sess_list.
  if (sess_list == NULL || sess_list->list == NULL) {
    return 0;
  }

  kcpuv_link *ptr = kcpuv_link_get_pointer(sess_list->list, this);
  assert(ptr != NULL);
  delete ptr;
  sess_list->len -= 1;

  return 1;
}

void KcpuvSess::Close_() {
  this->ExitUpdateQueue();

  if (onBeforeFree != NULL) {
    CloseCb cb = this->onBeforeFree;
    cb(this);
  }

  assert(onCloseCb);
  // call callback to inform outside
  CloseCb cb = onCloseCb;
  cb(this);
}

static void RemoveSessInNextTick(KcpuvCallbackInfo *info) {
  KcpuvSess *sess = reinterpret_cast<KcpuvSess *>(info->data);
  delete info;

  // TODO: `Unbind`ing is a bit late.
  sess->sessUDP->Unbind();
  sess->Close_();
}

// NOTE: Outside should not delete other instances in the same tick.
// NOTE: Outside is expected to delete instances manually.
void KcpuvSess::TriggerClose() {
  KcpuvCallbackInfo *info = new KcpuvCallbackInfo;
  info->data = this;
  info->cb = RemoveSessInNextTick;

  Loop::NextTick(info);
}

// Free a kcpuv session.
KcpuvSess::~KcpuvSess() {
  this->ExitUpdateQueue();

  if (cryptor != NULL) {
    Cryptor::KcpuvCryptorClean(cryptor);
    delete cryptor;
  }

  if (recvAddr != NULL) {
    delete recvAddr;
  }

  // TODO: Make sure sessUdp will be closed correctly.
  // if (!uv_is_closing((uv_handle_t *)handle)) {
  //   uv_close((uv_handle_t *)handle, free_handle_cb);
  // }
  delete sessUDP;

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
void KcpuvSess::InitCryptor(const char *key, int len) {
  KcpuvSess *sess = this;
  unsigned int salt[] = {1, 2};
  sess->cryptor = new kcpuv_cryptor;
  Cryptor::KcpuvCryptorInit(sess->cryptor, key, len, salt);
}

// Set sending info.
void KcpuvSess::InitSend(char *addr, int port) {
  sessUDP->SetSendAddr(addr, port);
}

// TODO: remove this
// Update kcp for content transmission.
static int input_kcp(KcpuvSess *sess, const char *msg, int length) {
  int rval = ikcp_input(sess->kcp, msg, length);

  if (KCPUV_DEBUG == 1 && rval < 0) {
    // TODO:
    fprintf(stderr, "ikcp_input() < 0: %d\n", rval);
  }

  return rval;
}

// Input dgram mannualy.
// `len` is also `nread`.
void KcpuvSess::KcpInput(const struct sockaddr *addr, const char *data,
                         int len) {
  KcpuvSess *sess = this;
  if (!sess->AllowInput()) {
    fprintf(stderr, "%s %d\n", "invalid sess state", sess->state);
    return;
  }

  if (len < 0) {
    // TODO:
    fprintf(stderr, "uv error: %s\n", uv_strerror(len));
  }

  if (len > 0) {
    // Mainly for the side that starts the conversation.
    if (sess->state == KCPUV_STATE_CREATED) {
      sess->state = KCPUV_STATE_READY;
    }

    // Mainlyy for the side waiting for a conversation.
    if (!sess->sessUDP->HasSendAddr() && sess->passive) {
      sess->state = KCPUV_STATE_READY;
      sess->sessUDP->SetSendAddrBySockaddr(addr);
    }

    int read_len = len;
    char *read_msg = reinterpret_cast<char *>(Cryptor::KcpuvCryptorDecrypt(
        sess->cryptor, (unsigned char *)data, &read_len));

    // if (KCPUV_DEBUG) {
    //   print_as_hex(buf->base, nread);
    //   print_as_hex(read_msg, read_len);
    // }

    // update active time
    sess->recvTs = iclock();

    input_kcp(sess, read_msg, read_len);

    delete[] read_msg;
  }
}

void KcpuvSess::BindBeforeClose(CloseCb cb) { onBeforeFree = cb; }

// Called when uv receives a msg and pass.
static void OnDgramCb(SessUDP *sessUDP, const struct sockaddr *addr,
                      const char *data, int len) {
  KcpuvSess *sess = reinterpret_cast<KcpuvSess *>(sessUDP->data);
  sess->KcpInput(addr, data, len);
}

// Set receiving info.
int KcpuvSess::Listen(int port, DataCb cb) {
  KcpuvSess *sess = this;
  assert(sess->state == KCPUV_STATE_CREATED);

  sess->onMsgCb = cb;

  return sess->sessUDP->Bind(port, OnDgramCb);
}

// // Stop listening
// int KcpuvStopListen(KcpuvSess *sess) { return uv_udp_recv_stop(handle); }

// Get address and port of a sess.
int KcpuvSess::GetAddressPort(char *addr, int *namelen, int *port) {
  return sessUDP->GetAddressPort(namelen, addr, port);
}

// Set close msg listener
void KcpuvSess::BindClose(CloseCb cb) {
  assert(cb);
  onCloseCb = cb;
}

void KcpuvSess::BindListen(DataCb cb) { onMsgCb = cb; }

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

// Close
void KcpuvSess::Close() {
  KcpuvSess *sess = this;

  // Don't need to handle a sess that is closing.
  if (sess->state >= KCPUV_STATE_FIN) {
    return;
  }

  if (sess->state == KCPUV_STATE_READY) {
    this->SendCMD(KCPUV_CMD_FIN);
    sess->state = KCPUV_STATE_FIN;
    return;
  }

  // The session is not connected and will nerver receive a fin ack.
  // Let's close it directly.
  // TODO: Tell outside that it exits because of timeout.
  sess->TriggerClose();

  // mark that this sess could be freed
  // TODO: where to put KCPUV_STATE_CLOSED ?
  // state = KCPUV_STATE_CLOSED;

  // uv_close(handle, NULL);
}

// Iterate the session_list and update kcp
void KcpuvSess::KcpuvUpdateKcpSess_(uv_timer_t *timer) {
  if (!sess_list || !sess_list->list) {
    return;
  }

  kcpuv_link *ptr = sess_list->list;

  // TODO: maybe we could assume that ikcp_update won't
  // cost too much time and get the time once
  IUINT32 now = 0;

  while (ptr->next != NULL) {
    int size;

    if (!ptr->next->node) {
      assert(0);
    }

    KcpuvSess *sess = (KcpuvSess *)ptr->next->node;
    unsigned int timeout = sess->timeout;
    ikcpcb *kcp = sess->kcp;
    int state = sess->state;

    now = iclock();

    if (enable_timeout && timeout && now - sess->recvTs >= timeout &&
        sess->state < KCPUV_STATE_WAIT_FREE) {
      // TODO: Tell outside that it exits because of timeout.
      sess->TriggerClose();
      ptr = ptr->next;
      continue;
    }

    IUINT32 time_to_update = ikcp_check(kcp, now);

    // NOTE: We have to call ikcp_update after the first calling
    // of the ikcp_input or we will get Segmentation fault.
    if (time_to_update <= now) {
      ikcp_update(kcp, now);
    }

    size = ikcp_recv(kcp, buffer, BUFFER_LEN);

    // TODO: consider the expected size
    while (size > 0) {
      // print_as_hex((const char *)(buffer), size);
      // parse cmd
      // check protocol
      int cmd = Cryptor::KcpuvProtocolDecode(buffer);
      // NOTE: We don't need to make sure the KCPUV_CMD_NOO
      // to be received as messages sended by kcp will also
      // refresh the recvTs to avoid timeout.
      // NOTE: Some states need to be considered carefully.
      if (cmd == KCPUV_CMD_NOO) {
        // do nothing
      } else if (cmd == KCPUV_CMD_FIN) {
        // If we also want and send fin, simply fin ack.
        if (state <= KCPUV_STATE_FIN) {
          sess->SendCMD(KCPUV_CMD_FIN_ACK);
          sess->state = KCPUV_STATE_FIN_ACK;
        }
      } else if (cmd == KCPUV_CMD_FIN_ACK) {
        sess->state = KCPUV_STATE_FIN_ACK;
      } else if (cmd == KCPUV_CMD_PUSH) {
        unsigned int content_length = size - KCPUV_OVERHEAD;
        // Push the packet into recvBuf.
        if (!kcpuv__should_output_buf(sess->recvBufLength, content_length)) {
          kcpuv__put_buf(sess, (const char *)(buffer + KCPUV_OVERHEAD),
                         content_length);
        } else {
          if (sess->onMsgCb != NULL) {
            // TODO: More tests.
            // update receive data
            DataCb onMsgCb = sess->onMsgCb;
            onMsgCb(sess, sess->recvBuf, sess->recvBufLength);
          }

          kcpuv__replace_buf(sess, buffer + KCPUV_OVERHEAD, content_length);
        }
      } else {
        // invalid CMD
        fprintf(stderr, "receive invalid cmd: %d\n", cmd);
      }

      size = ikcp_recv(kcp, buffer, BUFFER_LEN);
    }

    // TODO: Is call next function in the same tick makes it more efficient?
    if (sess->recvBufLength > 0) {
      if (sess->onMsgCb != NULL) {
        // update receive data
        DataCb onMsgCb = sess->onMsgCb;
        onMsgCb(sess, sess->recvBuf, sess->recvBufLength);
      }

      kcpuv__replace_buf(sess, NULL, 0);
    }

    if (size < 0) {
      // rval == -1 queue is empty
      // rval == -2 peeksize is zero(or invalid)
      if (size != -1 && size != -2) {
        // TODO:
        fprintf(stderr, "ikcp_recv() < 0: %d\n", size);
      }
    }

    if (sess->state == KCPUV_STATE_FIN_ACK) {
      int packets = ikcp_waitsnd(sess->kcp);

      // Trigger close when all data acked
      if (packets == 0) {
        // NOTE: kcpuv free will remove ptr
        sess->TriggerClose();
      }
    } else {
      if (KCPUV_SESS_HEARTBEAT_ACTIVE && sess->sessUDP->HasSendAddr() &&
          sess->state == KCPUV_STATE_READY &&
          sess->sendTs + KCPUV_SESS_HEARTBEAT_INTERVAL <= now &&
          ikcp_waitsnd(sess->kcp) == 0) {
        sess->SendCMD(KCPUV_CMD_NOO);
      }
    }

    ptr = ptr->next;
  }

  ptr = sess_list->list;
}

void KcpuvSess::SetTimeout(unsigned int t) { timeout = t; }
} // namespace kcpuv
