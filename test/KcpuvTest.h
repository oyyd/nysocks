#include "gmock/gmock.h"
#include "gtest/gtest.h"

#ifndef KCPUV_INIT_ENCRYPTOR
#define KCPUV_INIT_ENCRYPTOR(sess) kcpuv_sess_init_cryptor(sess, "hello", 5);

#endif

namespace kcpuv_test {
using namespace testing;

// using TestCallback = testing::MockFunction<void(const char *)>;

} // namespace kcpuv_test
