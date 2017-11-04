#ifndef KCPUV_H
#define KCPUV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ikcp.h"

// global
extern const int debug;
extern const int KCPUV_NONCE_LENGTH;
extern const int KCPUV_PROTOCOL_OVERHEAD;
extern const int INIT_WND_SIZE;
extern const IUINT32 MTU_DEF;
// extern IUINT32 IKCP_OVERHEAD;
extern const size_t MAX_SENDING_LEN;
extern const size_t BUFFER_LEN;
extern const unsigned int DEFAULT_TIMEOUT;
extern const unsigned short IP4_ADDR_LENTH;
extern const unsigned short IP6_ADDR_LENGTH;

#ifdef __cplusplus
}
#endif
#endif
