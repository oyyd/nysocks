#include "gmock/gmock.h"
#include "gtest/gtest.h"

#ifndef KCPUV_TEST_H
#define KCPUV_TEST_H

#include "KcpuvSess.h"
#include "Loop.h"
#include "utils.h"
#include <uv.h>

namespace kcpuv_test {
using namespace testing;

static uv_timer_t *timer = NULL;

static void emptyTimerCb(uv_timer_t *timer) {}

static void EmptyLoopCallback(uv_timer_t *timer) {
  kcpuv::Loop::KcpuvStopUpdaterTimer();
}

#ifndef RUN_EMPTY_LOOP
#define RUN_EMPTY_LOOP() Loop::KcpuvStartLoop_(EmptyLoopCallback)
#endif

#ifndef ENABLE_100MS_TIMER
#define ENABLE_100MS_TIMER()                                                   \
  assert(timer == NULL);                                                       \
  uv_timer_t *timer = new uv_timer_t;                                          \
  uv_timer_init(Loop::kcpuv_get_loop(), timer);                                \
  uv_timer_start(timer, EmptyLoopCallback, 100, 0)
#endif

// NOTE: Libuv may failed to trigger some callbacks if we don't actually use it.
#ifndef ENABLE_EMPTY_TIMER
#define ENABLE_EMPTY_TIMER()                                                   \
  assert(timer == NULL);                                                       \
  uv_timer_t *timer = new uv_timer_t;                                          \
  uv_timer_init(Loop::kcpuv_get_loop(), timer);                                \
  uv_timer_start(timer, emptyTimerCb, 10, 0)
#endif

#ifndef CLOSE_TEST_TIMER
#define CLOSE_TEST_TIMER()                                                     \
  do {                                                                         \
    if (timer) {                                                               \
      kcpuv__try_close_handle(reinterpret_cast<uv_handle_t *>(timer));         \
      timer = NULL;                                                            \
    }                                                                          \
  } while (0)
#endif

// void kcpuv_try_close_cb(uv_timer_t *);

#ifndef KCPUV_TRY_STOPPING_LOOP_AND_DESTRUCT
#define KCPUV_TRY_STOPPING_LOOP_AND_DESTRUCT()                                 \
  Loop::KcpuvDestroyLoop_();                                                   \
  KcpuvSess::KcpuvDestruct();
#endif

// timer = new uv_timer_t;
// kcpuv__add_timer(timer);
// uv_timer_start(timer, kcpuv_try_close_cb, 1000, 10);

#ifndef KCPUV_INIT_ENCRYPTOR
#define KCPUV_INIT_ENCRYPTOR(sess) sess->InitCryptor("hello", 5);
#endif

} // namespace kcpuv_test

#endif // KCPUV_TEST_H
