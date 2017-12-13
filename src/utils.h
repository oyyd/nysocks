#ifndef KCPUV_UTILS_H
#define KCPUV_UTILS_H
#ifdef __cplusplus
extern "C" {
#endif

#include "ikcp.h"
#include "uv.h"
#include <time.h>

#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
#include <windows.h>
#elif !defined(__unix)
#define __unix
#endif

#ifdef __unix
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

IINT64 iclock64(void);

IUINT32 iclock();

typedef struct KCPUV_LINK kcpuv_link;

void print_as_hex(const char *msg, int len);

struct KCPUV_LINK {
  void *node;
  kcpuv_link *next;
};

kcpuv_link *kcpuv_link_create(void *);

void kcpuv__print_sockaddr(const struct sockaddr *name);

void kcpuv_link_add(kcpuv_link *current, kcpuv_link *next);

kcpuv_link *kcpuv_link_get_pointer(kcpuv_link *head, void *node);

void kcpuv_log_error(char *msg);

void kcpuv_log(char *msg);

void alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);

void free_handle_cb(uv_handle_t *handle);

void kcpuv_print_protocol(const char *content, int len);

#ifdef __cplusplus
}
#endif
#endif
