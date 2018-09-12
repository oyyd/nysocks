#include "Loop.h"
#include "SessUDP.h"
#include "kcpuv.h"
#include "utils.h"

namespace kcpuv {

static uv_loop_t *kcpuv_loop = NULL;
static int use_default_loop = 0;
// static uv_idle_t *idle = NULL;
static uv_timer_t *timer = NULL;

// Force closing all handles
// static void closing_walk(uv_handle_t *handle, void *arg) {
//   if (uv_handle_get_type(handle) == UV_UDP && handle->data != nullptr) {
//     SessUDP *sessUDP = reinterpret_cast<SessUDP *>(handle->data);
//     sessUDP->CloseHandle();
//   } else {
//     fprintf(stderr, "%s\n", "LOOP_CLOSE_WALKING");
//     kcpuv__try_close_handle(handle);
//   }
// }

static void init_loop() {
  if (!use_default_loop && kcpuv_loop == NULL) {
    kcpuv_loop = new uv_loop_t;
    uv_loop_init(kcpuv_loop);
  }
}

// NOTE: Don't change this while looping.
void Loop::KcpuvUseDefaultLoop(int value) { use_default_loop = value; }

// Get or create a loop.
uv_loop_t *Loop::kcpuv_get_loop() {
  // init loop
  if (use_default_loop) {
    return uv_default_loop();
  }

  init_loop();

  return kcpuv_loop;
}

static void TriggerCallback(uv_timer_t *timer) {
  KcpuvCallbackInfo *info = reinterpret_cast<KcpuvCallbackInfo *>(timer->data);
  NextTickCb cb = info->cb;
  cb(info);
  kcpuv__try_close_handle(reinterpret_cast<uv_handle_t *>(timer));
}

void Loop::NextTick(KcpuvCallbackInfo *info) {
  uv_timer_t *timer = new uv_timer_t;

  timer->data = info;

  Loop::KcpuvNextTick_(timer, TriggerCallback);
}

uv_timer_t *Loop::AddTimer(unsigned int timeout, KcpuvCallbackInfo *info) {
  uv_timer_t *timer = new uv_timer_t;

  timer->data = info;

  uv_timer_init(kcpuv_get_loop(), timer);
  uv_timer_start(timer, TriggerCallback, timeout, 0);

  return timer;
}

void Loop::StopTimer(uv_timer_t *timer) {
  uv_timer_stop(timer);
  kcpuv__try_close_handle(reinterpret_cast<uv_handle_t *>(timer));
}

void Loop::NextTick(uv_loop_t *loop, KcpuvCallbackInfo *info) {
  uv_timer_t *timer = new uv_timer_t;

  timer->data = info;

  Loop::KcpuvNextTick_(loop, timer, TriggerCallback);
}

void Loop::KcpuvNextTick_(uv_timer_t *timer, uv_timer_cb cb) {
  uv_timer_init(kcpuv_get_loop(), timer);

  uv_timer_start(timer, cb, 0, 0);
}

void Loop::KcpuvNextTick_(uv_loop_t *loop, uv_timer_t *timer, uv_timer_cb cb) {
  uv_timer_init(loop, timer);

  uv_timer_start(timer, cb, 0, 0);
}

void Loop::KcpuvAddIdle_(uv_idle_t *idle) {
  uv_idle_init(kcpuv_get_loop(), idle);
}

void Loop::KcpuvAddTimer_(uv_timer_t *timer) {
  uv_timer_init(kcpuv_get_loop(), timer);
}

// Start uv kcpuv_loop and updating kcp.
void Loop::KcpuvStartLoop_(uv_timer_cb cb) {
  init_loop();
  // inited before

  timer = new uv_timer_t;
  timer->data = NULL;

  uv_timer_init(kcpuv_get_loop(), timer);
  uv_timer_start(timer, cb, 0, KCPUV_TIMER_INTERVAL);

  if (!use_default_loop) {
    uv_run(kcpuv_get_loop(), UV_RUN_DEFAULT);
  }
}

// void Loop::CloseLoopHandles_(uv_loop_t *loop) {
//   uv_walk(loop, closing_walk, NULL);
// }

// void Loop::KcpuvLoopCloseHandles_() {
//   Loop::CloseLoopHandles_(kcpuv_get_loop());
// }

static void check_handles(uv_handle_t *handle, void *arg) {
  fprintf(stderr, "handle_type: %s, isclosing: %d\n",
          uv_handle_type_name(handle->type), uv_is_closing(handle));
}

void Loop::KcpuvCheckHandles_() {
  uv_walk(kcpuv_get_loop(), check_handles, NULL);
}

int Loop::KcpuvStopUpdaterTimer() {
  if (timer != NULL) {
    int rval = uv_timer_stop(timer);

    if (rval) {
      fprintf(stderr, "close timer error: %s\n", uv_strerror(rval));
    }

    kcpuv__try_close_handle((uv_handle_t *)timer);

    timer = NULL;
  }

  return 0;
}

void Loop::KcpuvDestroyLoop_() {
  if (!use_default_loop && kcpuv_loop != NULL) {
    int rval = uv_loop_close(kcpuv_get_loop());

    if (rval) {
      fprintf(stderr, "close loop error: %s\n", uv_strerror(rval));
    }

    free(kcpuv_loop);
    kcpuv_loop = NULL;
  }
}
} // namespace kcpuv
