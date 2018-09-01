#include "SessUDP.h"
#include <cassert>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace kcpuv {
long kcpuvUDPBufSize = 4 * 1024 * 1024;

SessUDP::SessUDP(uv_loop_t *loop) {
  sendAddr = nullptr;
  recvAddr = nullptr;
  data = nullptr;
  handle.data = this;
  uv_udp_init(loop, &handle);
}

SessUDP::~SessUDP() {
  if (sendAddr != nullptr) {
    delete sendAddr;
  }

  if (recvAddr != nullptr) {
    delete recvAddr;
  }
}

void SessUDP::RecvCb(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf,
                     const struct sockaddr *addr, unsigned flags) {
  SessUDP *udp = reinterpret_cast<SessUDP *>(handle->data);
  DataCb dataCb = udp->dataCb;

  dataCb(udp, buf->base, nread);
}

void SessUDP::SetSendAddr(const char *addr, const int port) {
  if (!sendAddr) {
    sendAddr = new (struct sockaddr);
  }

  uv_ip4_addr(addr, port, reinterpret_cast<struct sockaddr_in *>(sendAddr));
}

int SessUDP::Bind(int port, DataCb cb) {
  assert(cb);
  dataCb = cb;

  if (!recvAddr) {
    recvAddr = new (struct sockaddr);
    uv_ip4_addr("0.0.0.0", port,
                reinterpret_cast<struct sockaddr_in *>(recvAddr));
  }

  int rval = uv_udp_bind(
      &handle, reinterpret_cast<const struct sockaddr *>(recvAddr), port);

  uv_udp_recv_start(&handle, alloc_cb, &RecvCb);
  // NOTE: it seems setting options before uv_udp_recv_start won't work
  uv_send_buffer_size(reinterpret_cast<uv_handle_t *>(&handle),
                      reinterpret_cast<int *>(&kcpuvUDPBufSize));
  uv_recv_buffer_size(reinterpret_cast<uv_handle_t *>(&handle),
                      reinterpret_cast<int *>(&kcpuvUDPBufSize));

  return rval;
}

int SessUDP::Send(const char *data, int dataLen) {
  assert(sendAddr);

  char *bufData = new char[dataLen];
  int rval;

  memcpy(bufData, data, dataLen);

  // Transfer `data` into buf.
  uv_buf_t buf = uv_buf_init(bufData, dataLen);

  rval = uv_udp_try_send(&handle, &buf, 1,
                         reinterpret_cast<const struct sockaddr *>(sendAddr));

  delete[] bufData;
  return rval;
}

int SessUDP::GetAddressPort(int *userNameLength, char *userAddr,
                            int *userPort) {
  struct sockaddr addr;
  int rval;

  *userNameLength = sizeof(struct sockaddr);
  rval = uv_udp_getsockname(reinterpret_cast<const uv_udp_t *>(&handle), &addr,
                            userNameLength);
  if (rval) {
    return rval;
  }

  if (addr.sa_family == AF_INET) {
    uv_ip4_name(reinterpret_cast<const struct sockaddr_in *>(&addr), userAddr,
                IP4_ADDR_LENTH);
    *userPort =
        ntohs((reinterpret_cast<const struct sockaddr_in *>(&addr))->sin_port);
  } else {
    // TODO: Support for ipv6 needs more consideration.
    uv_ip6_name(reinterpret_cast<const struct sockaddr_in6 *>(&addr), userAddr,
                IP6_ADDR_LENGTH);
    *userPort = ntohs(
        (reinterpret_cast<const struct sockaddr_in6 *>(&addr))->sin6_port);
  }

  return 0;
}

} // namespace kcpuv
