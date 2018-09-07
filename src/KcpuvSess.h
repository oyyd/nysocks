#ifndef KCPUV_SESS_H
#define KCPUV_SESS_H

#include "Loop.h"
#include "ikcp.h"
#include "kcpuv.h"
// TODO:
#include "Cryptor.h"
#include "utils.h"
#include "uv.h"

namespace kcpuv {

class KcpuvSess;

typedef void (*kcpuv_listen_cb)(KcpuvSess *sess, const char *data,
                                unsigned int len);
typedef void (*kcpuv_dgram_cb)(KcpuvSess *sess);
typedef void (*kcpuv_udp_send)(KcpuvSess *sess, uv_buf_t *buf, int buf_count,
                               const struct sockaddr *);

// struct KCPUV_SESS {
// };

typedef struct KCPUV_SESS_LIST {
  kcpuv_link *list;
  int len;
} kcpuv_sess_list;

class KcpuvSess {
public:
  KcpuvSess();
  ~KcpuvSess();

  // // Get Internal session list references for updating or other operations.
  static kcpuv_sess_list *KcpuvGetSessList();
  //
  // // For debug.
  // void KcpuvPrintSessList_();

  // Static global func to enable timeout setting.
  static void KcpuvSessEnableTimeout(short);

  // Init the cryptor function inside a sess which is a required operation
  // before any actual communication.
  static void KcpuvSessInitCryptor(KcpuvSess *sess, const char *key, int len);

  // Get if a sess is freed.
  // TODO: This seems unreliable and need refactor.
  static int KcpuvIsFreed(KcpuvSess *sess);

  // // TODO: This seems to be not used.
  // int KcpuvSetState(KcpuvSess *sess, int state);

  // Tell kcp that new message has come so that it could update its reliable
  // message.
  static void KcpuvInput(KcpuvSess *sess, ssize_t nread, const uv_buf_t *buf,
                         const struct sockaddr *addr);

  // Tell sess where all coming messages should be sent to.
  // A required operation before actual communication.
  static void KcpuvInitSend(KcpuvSess *sess, char *addr, int port);

  // Set port to listen.
  static int KcpuvListen(KcpuvSess *sess, int port, kcpuv_listen_cb cb);

  // // Get listened port info.
  // int KcpuvGetAddress(KcpuvSess *sess, char *addr, int *namelen, int *port);
  //
  // // Stop listen on the port.
  // // This seems to be not used.
  // int KcpuvStopListen(KcpuvSess *sess);

  // Actual send operation.
  static void kcpuv_raw_send(KcpuvSess *sess, const int cmd, const char *msg,
                             unsigned long len);
  // Send data message.
  static void KcpuvSend(KcpuvSess *sess, const char *msg, unsigned long len);

  // Send cmd instead of message.
  static void KcpuvSendCMD(KcpuvSess *sess, const int cmd);

  // // Only send close cmd. Different from KcpuvFree.
  // void KcpuvClose(KcpuvSess *sess);
  //
  // // Bind listener on close.
  // void KcpuvBindClose(KcpuvSess *, kcpuv_dgram_cb);
  //
  // // Bind listener on listen.
  // void KcpuvBindListen(KcpuvSess *, kcpuv_listen_cb);
  //
  // // Bind listener on udpSend.
  // // Seems not useful.
  // void KcpuvBindUdpSend(KcpuvSess *sess, kcpuv_udp_send udpSend);
  //
  // // Bind listener on before_free.
  // void KcpuvBindBeforeFree(KcpuvSess *sess, kcpuv_dgram_cb onBeforeFree);
  //
  // Init global variables like sess_list.
  static void KcpuvInitialize();

  // Destruct global varibales.
  static int KcpuvDestruct();

  // An internal operations to make sess/kcp digest data.
  // Mainly used for mux.
  static void KcpuvUpdateKcpSess_(uv_timer_t *timer);

  // TODO: Make these private.
  ikcpcb *kcp;
  uv_udp_t *handle;
  int state;
  kcpuv_udp_send udpSend;
  kcpuv_cryptor *cryptor;
  struct sockaddr *sendAddr;
  struct sockaddr *recvAddr;
  // User defined data.
  void *data;
  // TODO: make mux store sess sf a clearer solution
  void *mux;
  IUINT32 recvTs;
  IUINT32 sendTs;
  unsigned int timeout;
  kcpuv_listen_cb onMsgCb;
  kcpuv_dgram_cb onCloseCb;
  kcpuv_dgram_cb onBeforeFree;
  char *recvBuf;
  unsigned int recvBufLength;

private:
};

} // namespace kcpuv

#endif // KCPUV_KCP_SESS
