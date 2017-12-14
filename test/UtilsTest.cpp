#include "KcpuvTest.h"
#include <iostream>

namespace kcpuv_test {

using namespace ::std;
using namespace ::testing;

class UtilsTest : public Test {
protected:
  UtilsTest(){};
};

TEST_F(UtilsTest, kcpuv_link) {
  kcpuv_link *head = kcpuv_link_create(NULL);
  kcpuv_link *current = head;
  kcpuv_link *ptr;

  for (int i = 0; i < 3; i += 1) {
    int *node = new int;
    *node = i;

    kcpuv_link *link = kcpuv_link_create(node);
    kcpuv_link_add(head, link);
  }

  for (int i = 0; i < 3; i += 1) {
    current = current->next;
    EXPECT_EQ(*(static_cast<int *>(current->node)), i);
  }

  current = head->next->next;

  ptr = kcpuv_link_get_pointer(head, current->node);
  free(ptr);
  free(current->node);

  current = head->next;
  EXPECT_EQ(*(static_cast<int *>(current->node)), 0);

  current = current->next;
  EXPECT_EQ(*(static_cast<int *>(current->node)), 2);

  current = NULL;

  ptr = kcpuv_link_get_pointer(head, current);
  free(ptr);

  kcpuv_link *next = NULL;
  current = head;

  while (current != NULL) {
    next = current->next;
    free(current->node);
    free(current);
    current = next;
  }
}

TEST_F(UtilsTest, iclock) {
  IUINT32 time = iclock();
  EXPECT_TRUE(time);
}

} // namespace kcpuv_test
