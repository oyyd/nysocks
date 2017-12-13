#include "utils.h"
#include "kcpuv.h"
#include "uv.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* get system time */
static inline void itimeofday(long *sec, long *usec) {
#if defined(__unix)
  struct timeval time;
  gettimeofday(&time, NULL);
  if (sec)
    *sec = time.tv_sec;
  if (usec)
    *usec = time.tv_usec;
#else
  static long mode = 0, addsec = 0;
  BOOL retval;
  static IINT64 freq = 1;
  IINT64 qpc;
  if (mode == 0) {
    retval = QueryPerformanceFrequency((LARGE_INTEGER *)&freq);
    freq = (freq == 0) ? 1 : freq;
    retval = QueryPerformanceCounter((LARGE_INTEGER *)&qpc);
    addsec = (long)time(NULL);
    addsec = addsec - (long)((qpc / freq) & 0x7fffffff);
    mode = 1;
  }
  retval = QueryPerformanceCounter((LARGE_INTEGER *)&qpc);
  retval = retval * 2;
  if (sec)
    *sec = (long)(qpc / freq) + addsec;
  if (usec)
    *usec = (long)((qpc % freq) * 1000000 / freq);
#endif
}

/* get clock in millisecond 64 */
IINT64 iclock64(void) {
  long s, u;
  IINT64 value;
  itimeofday(&s, &u);
  value = ((IINT64)s) * 1000 + (u / 1000);
  return value;
}

IUINT32 iclock() { return (IUINT32)(iclock64() & 0xfffffffful); }

// +------+-----+-----+-----+----+----+-----+-----+
// | CONV | CMD | FRG | WND | TS | SN | UNA | LEN |
// +------+-----+-----+-----+----+----+-----+-----+
// |  4   |  1  |  1  |  2  | 4  | 4  |  4  |  4  |
// +------+-----+-----+-----+----+----+-----+-----+
static int protocol_len[8] = {4, 1, 1, 2, 4, 4, 4, 4};
// static char *protocol_name[8] = {"conv", "cmd", "frg", "wnd",
//                                  "ts",   "sn",  "una", "len"};
// static int protocol_length = 24;

void print_as_hex(const char *msg, int len) {
  int i;
  int j;
  int cur = 0;

  for (i = 0; i < 8; i += 1) {
    // char *name = protocol_name[i];
    int len = protocol_len[i];

    for (j = 0; j < len; j += 1) {
      printf("%x ", (unsigned)(unsigned char)msg[cur + j]);
    }
    printf(" ");
    cur += len;
  }

  // for (i = 8; i < len && i < 18; i += 1) {
  //   for (int j = 0; j < len; j += 1) {
  //     printf("%x ", (unsigned)(unsigned char)msg[cur + j]);
  //   }
  //   printf(" ");
  //   cur += len;
  // }

  printf("\n");
}

void kcpuv__print_sockaddr(const struct sockaddr *name) {
  int port;
  char *addr = malloc(sizeof(char) * 16);

  uv_ip4_name((const struct sockaddr_in *)name, addr, IP4_ADDR_LENTH);
  port = ntohs(((struct sockaddr_in *)name)->sin_port);

  fprintf(stderr, "addr: %s:%d\n", addr, port);
  free(addr);
}

void kcpuv_log_error(char *msg) { fprintf(stderr, "error: %s", msg); }

void kcpuv_log(char *msg) { fprintf(stdout, "log: %s", msg); };

// NOTE: Should keep the head empty.
kcpuv_link *kcpuv_link_create(void *node) {
  kcpuv_link *link = malloc(sizeof(kcpuv_link));
  link->node = node;
  link->next = NULL;

  return link;
}

void kcpuv_link_add(kcpuv_link *head, kcpuv_link *next) {
  kcpuv_link *current = head;

  // TODO: we don't have to add a new node to the end of the queue
  while (current->next != NULL) {
    current = current->next;
  }

  current->next = next;
  next->next = NULL;
}

// Return `NULL` if the pointer is not in the queue.
kcpuv_link *kcpuv_link_get_pointer(kcpuv_link *head, void *node) {
  kcpuv_link *current = head;
  kcpuv_link *ptr = NULL;

  while (current->next != NULL && current->next->node != node) {
    current = current->next;
  }

  if (current->next != NULL) {
    ptr = current->next;
    current->next = ptr->next;
    ptr->next = NULL;

    return ptr;
  }

  return NULL;
}

void alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
  buf->base = malloc(suggested_size);
  buf->len = suggested_size;
}

void free_handle_cb(uv_handle_t *handle) { free(handle); }
