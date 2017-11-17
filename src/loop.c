#include "loop.h"

static uv_loop_t *kcpuv_loop = NULL;
static int use_default_loop = 0;
static uv_idle_t *idler = NULL;

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

// Start uv kcpuv_loop and updating kcp.
void kcpuv_start_loop(uv_idle_cb cb) {
  init_loop();
  // inited before

  // bind a idle watcher to uv kcpuv_loop for updating kcp
  // TODO: do we need to delete idler?
  idler = malloc(sizeof(uv_idle_t));
  uv_idle_init(kcpuv_get_loop(), idler);
  uv_idle_start(idler, cb);

  if (!use_default_loop) {
    uv_run(kcpuv_get_loop(), UV_RUN_DEFAULT);
  }
}

void kcpuv_stop_loop() {
  if (idler != NULL) {
    uv_idle_stop(idler);
    free(idler);
  }

  if (!use_default_loop && kcpuv_get_loop() != NULL) {
    uv_stop(kcpuv_get_loop());

    // NOTE: The closing may failed simply because
    // we don't call the `uv_run` on this loop and
    // then causes a memory leaking.
    int rval = uv_loop_close(kcpuv_get_loop());
    // if (rval) {
    //   fprintf(stderr, "failed to close the loop: %s\n", uv_strerror(rval));
    //   return -1;
    // }
  }
}

void kcpuv__destroy_loop() {
  if (!use_default_loop && kcpuv_loop != NULL) {
    free(kcpuv_loop);
    kcpuv_loop = NULL;
  }
}
