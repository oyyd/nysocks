#include "kcpuv.h"

const int debug = 0;
const int KCPUV_NONCE_LENGTH = 8;
const int KCPUV_PROTOCOL_OVERHEAD = 1;
const int INIT_WND_SIZE = 2048;
const IUINT32 MTU_DEF = 1400 - KCPUV_PROTOCOL_OVERHEAD - KCPUV_NONCE_LENGTH;
// const IUINT32 IKCP_OVERHEAD = 24;
const size_t MAX_SENDING_LEN = 65536;
const size_t BUFFER_LEN = MAX_SENDING_LEN;
const unsigned int DEFAULT_TIMEOUT = 30000;
const unsigned short IP4_ADDR_LENTH = 17;
const unsigned short IP6_ADDR_LENGTH = 68;
