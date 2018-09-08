#include "Loop.h"
#include "kcpuv.h"
#include "utils.h"

static uv_loop_t *kcpuv_loop = NULL;
static int use_default_loop = 0;
// static uv_idle_t *idle = NULL;
static uv_timer_t *timer = NULL;

// Force closing all handles
static void closing_walk(uv_handle_t *handle, void *arg) {
  kcpuv__try_close_handle(handle);
}

static void check_handles(uv_handle_t *handle, void *arg) {
  fprintf(stderr, "handle_type: %d\n", handle->type);
}

static void init_loop() {
  if (!use_default_loop && kcpuv_loop == NULL) {
    kcpuv_loop = new uv_loop_t;
    uv_loop_init(kcpuv_loop);
  }
}

namespace kcpuv {

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

void Loop::KcpuvNextTick_(uv_timer_t *timer, uv_timer_cb cb) {
  uv_timer_init(kcpuv_get_loop(), timer);

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

void Loop::CloseLoopHandles_(uv_loop_t *loop) {
  uv_walk(loop, closing_walk, NULL);
}

void Loop::KcpuvLoopCloseHandles_() {
  Loop::CloseLoopHandles_(kcpuv_get_loop());
}

void Loop::KcpuvCheckHandles_() {
  uv_walk(kcpuv_get_loop(), check_handles, NULL);
}

int Loop::KcpuvStopLoop() {
  if (timer != NULL) {
    int rval = uv_timer_stop(timer);

    if (rval) {
      fprintf(stderr, "%s\n", uv_strerror(rval));
    }

    kcpuv__try_close_handle((uv_handle_t *)timer);

    timer = NULL;
  }

  if (!use_default_loop && kcpuv_get_loop() != NULL) {
    KcpuvLoopCloseHandles_();
    // uv_stop(kcpuv_get_loop());

    // uv_walk(kcpuv_get_loop(), closing_walk, NULL);
    //
    // uv_run(kcpuv_get_loop(), UV_RUN_DEFAULT);

    // int rval = uv_loop_close(kcpuv_get_loop());
    //
    // if (rval) {
    //   fprintf(stderr, "%s\n", uv_strerror(rval));
    // }
    //
    // return rval;
  }

  return 0;
}

void Loop::KcpuvDestroyLoop_() {
  if (!use_default_loop && kcpuv_loop != NULL) {
    int rval = uv_loop_close(kcpuv_get_loop());

    if (rval) {
      fprintf(stderr, "%s\n", uv_strerror(rval));
    }

    free(kcpuv_loop);
    kcpuv_loop = NULL;
  }
}
} // namespace kcpuv
