#ifndef KCPUV_H
#define KCPUV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ikcp.h"

// global
extern const int debug;

extern const int KCPUV_MUX_CONN_TIMEOUT;
extern const int KCPUV_STATE_CREATED;
extern const int KCPUV_STATE_ACK;
extern const int KCPUV_STATE_WAIT_ACK;
extern const int KCPUV_STATE_READY;
extern const int KCPUV_STATE_CLOSED;
extern const int KCPUV_CMD_ACK;
extern const int KCPUV_CMD_PUSH;
extern const int KCPUV_CMD_NOO;
extern const int KCPUV_CMD_CLS;
extern const int KCPUV_NONCE_LENGTH;
extern const int KCPUV_PROTOCOL_OVERHEAD;
extern const int KCPUV_OVERHEAD;
extern const int INIT_WND_SIZE;
extern const IUINT32 MTU_DEF;
// extern IUINT32 IKCP_OVERHEAD;
extern const size_t MAX_SENDING_LEN;
extern const size_t BUFFER_LEN;
extern const unsigned int DEFAULT_TIMEOUT;
extern const unsigned short IP4_ADDR_LENTH;
extern const unsigned short IP6_ADDR_LENGTH;
extern const int KCPUV_MUX_CMD_PUSH;
extern const int KCPUV_MUX_CMD_CLS;
// one byte for cmd, two byte for length
extern const int KCPUV_MUX_PROTOCOL_OVERHEAD;
extern const size_t MAX_MUX_CONTENT_LEN;

#ifdef __cplusplus
}
#endif
#endif
