#include "gmock/gmock.h"
#include "gtest/gtest.h"

#ifndef KCPUV_TEST_H
#define KCPUV_TEST_H

extern "C" {

#include "KcpuvSess.h"
#include "Loop.h"
#include "utils.h"
#include <uv.h>

static uv_timer_t *timer = NULL;

// void kcpuv_try_close_cb(uv_timer_t *);

#ifndef KCPUV_TRY_STOPPING_LOOP
#define KCPUV_TRY_STOPPING_LOOP()                                              \
  Loop::KcpuvDestroyLoop_();                                                   \
  KcpuvSess::KcpuvDestruct();
#endif

// timer = new uv_timer_t;
// kcpuv__add_timer(timer);
// uv_timer_start(timer, kcpuv_try_close_cb, 1000, 10);

#ifndef KCPUV_INIT_ENCRYPTOR
#define KCPUV_INIT_ENCRYPTOR(sess)                                             \
  KcpuvSess::KcpuvSessInitCryptor(sess, "hello", 5);
#endif

} // extern "C"

#endif // KCPUV_TEST_H

namespace kcpuv_test {
using namespace testing;

// using TestCallback = testing::MockFunction<void(const char *)>;

} // namespace kcpuv_test
