#include "SessUDP.h"
#include <cassert>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace kcpuv {
long kcpuvUDPBufSize = 4 * 1024 * 1024;

SessUDP::SessUDP(uv_loop_t *loop) {
  udpSend = nullptr;
  sendAddr = nullptr;
  recvAddr = nullptr;
  data = nullptr;
  handle = new uv_udp_t;
  handle->data = this;
  uv_udp_init(loop, handle);
}

// The `handle` might be closed in two situations:
// 1. SessUDPs freed.
// 2. UV loop stopped and all handle closed.
void SessUDP::CloseHandle() {
  if (handle != nullptr) {
    uv_udp_t *h = handle;
    handle = nullptr;
    h->data = nullptr;
    // TODO: We may still need to free it manualy.
    kcpuv__try_close_handle((uv_handle_t *)h);
  }
}

SessUDP::~SessUDP() {
  CloseHandle();

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
  DgramCb dataCb = udp->dataCb;

  dataCb(udp, addr, buf->base, nread);

  delete buf->base;
}

void SessUDP::SetSendAddrBySockaddr(const struct sockaddr *addr) {
  assert(!sendAddr);
  sendAddr = new (struct sockaddr);
  memcpy(sendAddr, addr, sizeof(struct sockaddr));
}

void SessUDP::SetSendAddr(const char *addr, const int port) {
  // NOTE: Do not allow SetSendAddr twice.
  assert(!sendAddr);
  sendAddr = new (struct sockaddr);

  uv_ip4_addr(addr, port, reinterpret_cast<struct sockaddr_in *>(sendAddr));
}

void SessUDP::BindUdpSend(UDPProxySend send) { udpSend = send; }

int SessUDP::Bind(int port, DgramCb cb) {
  assert(cb);
  // NOTE: Do not allow bind twice.
  assert(!recvAddr);

  int rval = 0;

  dataCb = cb;

  if (!recvAddr) {
    recvAddr = new struct sockaddr;
    uv_ip4_addr("127.0.0.1", port,
                reinterpret_cast<struct sockaddr_in *>(recvAddr));
  }

  // No binding before `uv_udp_recv_start` will make uv bind at a random port.
  if (port != 0) {
    rval = uv_udp_bind(handle,
                       reinterpret_cast<const struct sockaddr *>(recvAddr), 0);

    if (rval < 0) {
      return rval;
    }
  }

  uv_udp_recv_start(handle, alloc_cb, &RecvCb);
  // NOTE: it seems setting options before uv_udp_recv_start won't work
  uv_send_buffer_size(reinterpret_cast<uv_handle_t *>(handle),
                      reinterpret_cast<int *>(&kcpuvUDPBufSize));
  uv_recv_buffer_size(reinterpret_cast<uv_handle_t *>(handle),
                      reinterpret_cast<int *>(&kcpuvUDPBufSize));

  return rval;
}

int SessUDP::Unbind() { return uv_udp_recv_stop(handle); }

int SessUDP::Send(const char *data, int dataLen) {
  assert(sendAddr);

  char *bufData = new char[dataLen];
  int rval = 0;

  memcpy(bufData, data, dataLen);

  if (udpSend != nullptr) {
    // TODO: Test this in cpp.
    udpSend(this, reinterpret_cast<const struct sockaddr *>(sendAddr), bufData,
            dataLen);
  } else {
    // Transfer `data` into buf.
    uv_buf_t buf = uv_buf_init(bufData, dataLen);
    // TODO: Maybe handle error here.
    rval = uv_udp_try_send(handle, &buf, 1,
                           reinterpret_cast<const struct sockaddr *>(sendAddr));
  }

  delete[] bufData;
  return rval;
}

int SessUDP::GetAddressPort(int *userNameLength, char *userAddr,
                            int *userPort) {
  struct sockaddr addr;
  int rval;

  *userNameLength = sizeof(struct sockaddr);
  rval = uv_udp_getsockname(reinterpret_cast<const uv_udp_t *>(handle), &addr,
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

bool SessUDP::HasSendAddr() { return sendAddr != nullptr; }

} // namespace kcpuv
