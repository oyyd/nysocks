#ifndef KCPUV_LOOP_H
#define KCPUV_LOOP_H

#include "uv.h"

namespace kcpuv {

class Loop {
public:
  Loop(){};
  ~Loop(){};

  static uv_loop_t *kcpuv_get_loop();

  static void KcpuvNextTick_(uv_timer_t *timer, uv_timer_cb cb);

  static void KcpuvUseDefaultLoop(int value);

  static void KcpuvAddIdle_(uv_idle_t *idle);

  static void KcpuvAddTimer_(uv_timer_t *timer);

  static void KcpuvStartLoop_(uv_timer_cb cb);

  static void KcpuvLoopCloseHandles_();

  static int KcpuvStopLoop();

  static void KcpuvDestroyLoop_();

  static void KcpuvCheckHandles_();
};

} // namespace kcpuv
#endif // KCPUV_LOOP_H
