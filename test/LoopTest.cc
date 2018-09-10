#include "Loop.h"
#include "KcpuvTest.h"
#include <iostream>

namespace kcpuv_test {
using namespace kcpuv;
using namespace std;

class LoopTest : public testing::Test {
protected:
  LoopTest(){};
  ~LoopTest(){};
};

TEST_F(LoopTest, NextTick) {
  //
}
} // namespace kcpuv_test
