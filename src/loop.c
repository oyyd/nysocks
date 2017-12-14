#include "loop.h"
#include "kcpuv.h"
#include "utils.h"

static uv_loop_t *kcpuv_loop = NULL;
static int use_default_loop = 0;
// static uv_idle_t *idle = NULL;
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

void kcpuv__add_timer(uv_timer_t *timer) {
  uv_timer_init(kcpuv_get_loop(), timer);
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

// static void close_cb(uv_handle_t *handle) {
//   //
//   fprintf(stderr, "%s\n", "close_cb");
// }

// Force closing all handles
static void closing_walk(uv_handle_t *handle, void *arg) {
  if (!uv_is_closing(handle)) {
    uv_close(handle, NULL);
  }
}

void kcpuv__loop_close_handles() {
  uv_walk(kcpuv_get_loop(), closing_walk, NULL);
}

int kcpuv_stop_loop() {
  if (timer != NULL) {
    int rval = uv_timer_stop(timer);

    if (rval) {
      fprintf(stderr, "%s\n", uv_strerror(rval));
    }

    if (!uv_is_closing((uv_handle_t *)timer)) {
      uv_close((uv_handle_t *)timer, free_handle_cb);
    }

    timer = NULL;
  }

  if (!use_default_loop && kcpuv_get_loop() != NULL) {
    kcpuv__loop_close_handles();
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

void kcpuv__destroy_loop() {
  if (!use_default_loop && kcpuv_loop != NULL) {
    int rval = uv_loop_close(kcpuv_get_loop());

    if (rval) {
      fprintf(stderr, "%s\n", uv_strerror(rval));
    }

    free(kcpuv_loop);
    kcpuv_loop = NULL;
  }
}
