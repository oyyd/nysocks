#include "kcpuv.h"

const int debug = 0;

const int KCPUV_STATE_CREATED = 0;
const int KCPUV_STATE_ACK = 10;
const int KCPUV_STATE_WAIT_ACK = 20;
const int KCPUV_STATE_READY = 30;
const int KCPUV_STATE_CLOSED = 40;
const int KCPUV_CMD_ACK = 10;
const int KCPUV_CMD_PUSH = 20;
const int KCPUV_CMD_NOO = 30;
const int KCPUV_CMD_CLS = 40;
const int KCPUV_NONCE_LENGTH = 8;
const int KCPUV_PROTOCOL_OVERHEAD = 1;
const int KCPUV_OVERHEAD = KCPUV_NONCE_LENGTH + KCPUV_PROTOCOL_OVERHEAD;
const int INIT_WND_SIZE = 2048;
const IUINT32 MTU_DEF = 1400 - KCPUV_PROTOCOL_OVERHEAD - KCPUV_NONCE_LENGTH;
// const IUINT32 IKCP_OVERHEAD = 24;
const size_t MAX_SENDING_LEN = 65536;
const size_t BUFFER_LEN = MAX_SENDING_LEN;
const unsigned int DEFAULT_TIMEOUT = 30000;
const unsigned short IP4_ADDR_LENTH = 17;
const unsigned short IP6_ADDR_LENGTH = 68;
const int KCPUV_MUX_CMD_PUSH = 10;
const int KCPUV_MUX_CMD_CLS = 40;
// one byte for cmd,
// four byte for id,
// two byte for length,
const int KCPUV_MUX_PROTOCOL_OVERHEAD = 7;
const size_t MAX_MUX_CONTENT_LEN =
    MAX_SENDING_LEN - KCPUV_MUX_PROTOCOL_OVERHEAD;
