#ifndef KCPUV_LOOP_H
#define KCPUV_LOOP_H

#include "uv.h"

namespace kcpuv {

typedef struct KcpuvCallbackInfo KcpuvCallbackInfo;

typedef void (*NextTickCb)(KcpuvCallbackInfo *);

typedef struct KcpuvCallbackInfo {
  NextTickCb cb;
  void *data;
} KcpuvCallbackInfo;

class Loop {
public:
  Loop(){};
  ~Loop(){};

  static uv_loop_t *kcpuv_get_loop();

  static void KcpuvNextTick_(uv_timer_t *timer, uv_timer_cb cb);

  static void KcpuvNextTick_(uv_loop_t *loop, uv_timer_t *timer,
                             uv_timer_cb cb);

  static void NextTick(KcpuvCallbackInfo *);

  static void NextTick(uv_loop_t *loop, KcpuvCallbackInfo *);

  static void KcpuvUseDefaultLoop(int value);

  static void KcpuvAddIdle_(uv_idle_t *idle);

  static void KcpuvAddTimer_(uv_timer_t *timer);

  static void KcpuvStartLoop_(uv_timer_cb cb);

  static void KcpuvLoopCloseHandles_();

  static int KcpuvStopUpdaterTimer();

  static void KcpuvDestroyLoop_();

  static void KcpuvCheckHandles_();
};

} // namespace kcpuv
#endif // KCPUV_LOOP_H
