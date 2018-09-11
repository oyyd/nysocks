#ifndef SESS_UDP_H
#define SESS_UDP_H

#include "kcpuv.h"
#include "utils.h"
#include "uv.h"

namespace kcpuv {

class SessUDP;

typedef void (*DgramCb)(SessUDP *udp, const struct sockaddr *addr,
                        const char *data, int len);
typedef void (*UDPProxySend)(SessUDP *udp, const struct sockaddr *addr,
                             const char *data, int len);

// TODO: All uv methods used:
// uv_udp_try_send
// uv_udp_init
// uv_udp_bind
// uv_udp_recv_start
// uv_udp_recv_stop
// uv_udp_getsockname
class SessUDP {
public:
  SessUDP(uv_loop_t *loop);
  ~SessUDP();

  static void RecvCb(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf,
                     const struct sockaddr *addr, unsigned flags);
  void CloseHandle();
  void SetSendAddrBySockaddr(const struct sockaddr *addr);
  void SetSendAddr(const char *addr, const int port);
  // NOTE: Send may failed.
  int Send(const char *data, int len);
  int Bind(int port, DgramCb cb);
  int Unbind();
  int GetAddressPort(int *namelength, char *addr, int *port);
  bool HasSendAddr();
  void BindUdpSend(UDPProxySend send);

  // user data
  void *data;

private:
  // Operations in uv are mostly asynchronous so that we should not try to
  // delete it synchchronously.
  uv_udp_t *handle;
  UDPProxySend udpSend;

  // TODO: Change the name to `dgramCb`.
  DgramCb dataCb;

  struct sockaddr *sendAddr;
  struct sockaddr *recvAddr;
};
} // namespace kcpuv

#endif // SESS_UDP_H
