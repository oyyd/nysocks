#include "KcpuvTest.h"

// void kcpuv_try_close_cb(uv_timer_t *timer) {
//   int rval = kcpuv_stop_loop();
//
//   if (rval) {
//     return;
//   }
//
//   kcpuv_stop_loop();
//   fprintf(stderr, "%s\n", "released");
//
//   uv_timer_stop(timer);
//
//   free(timer);
//   timer = NULL;
// }
