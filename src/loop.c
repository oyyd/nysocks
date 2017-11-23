#include "loop.h"
#include "kcpuv.h"

static uv_loop_t *kcpuv_loop = NULL;
static int use_default_loop = 0;
static uv_idle_t *idle = NULL;
static uv_timer_t *timer = NULL;

// NOTE: Don't change this while looping.
void kcpuv_use_default_loop(int value) { use_default_loop = value; }

static void init_loop() {
  if (!use_default_loop && kcpuv_loop == NULL) {
    kcpuv_loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(kcpuv_loop);
  }
}

// Get or create a loop.
uv_loop_t *kcpuv_get_loop() {
  // init loop
  if (use_default_loop) {
    return uv_default_loop();
  }

  init_loop();

  return kcpuv_loop;
}

void kcpuv__add_idle(uv_idle_t *idle) {
  //
  uv_idle_init(kcpuv_get_loop(), idle);
}

// Start uv kcpuv_loop and updating kcp.
void kcpuv_start_loop(uv_timer_cb cb) {
  init_loop();
  // inited before

  timer = malloc(sizeof(uv_timer_t));
  uv_timer_init(kcpuv_get_loop(), timer);
  uv_timer_start(timer, cb, 0, KCPUV_TIMER_INTERVAL);

  if (!use_default_loop) {
    uv_run(kcpuv_get_loop(), UV_RUN_DEFAULT);
  }
}

int kcpuv_stop_loop() {
  if (timer != NULL) {
    uv_timer_stop(timer);
    free(timer);
  }

  if (!use_default_loop && kcpuv_get_loop() != NULL) {
    uv_stop(kcpuv_get_loop());

    // NOTE: The closing may failed simply because
    // we don't call the `uv_run` on this loop and
    // then causes a memory leaking.
    return uv_loop_close(kcpuv_get_loop());
  }

  return 0;
}

void kcpuv__destroy_loop() {
  if (!use_default_loop && kcpuv_loop != NULL) {
    free(kcpuv_loop);
    kcpuv_loop = NULL;
  }
}
