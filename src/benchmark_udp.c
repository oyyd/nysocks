#include "kcpuv_sess.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

static long start_time_sec;
static long start_time_usec;
static long end_time_sec;
static long end_time_usec;
static int received = 0;
static int times = 1;
static int content_size = 1 * 1024 * 1024;
// static int times = 4 * 1024;
// static int content_size = 1024;
static long recv_send_buf_size = 4 * 1024 * 1024;

static void get_time(long *sec, long *usec) {
  struct timeval time;

  gettimeofday(&time, NULL);

  if (sec) {
    *sec = time.tv_sec;
  }

  if (usec) {
    *usec = time.tv_usec;
  }
}

void recv_cb(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf,
             const struct sockaddr *addr, unsigned flags) {
  if (nread) {
    received += 1;
    // fprintf(stdout, "recv nread: %d \n", received);
    if (received == times) {
      get_time(&end_time_sec, &end_time_usec);

      fprintf(stdout, "time elapsed: %ld \n",
              (end_time_sec - start_time_sec) * 1000 +
                  (end_time_usec - start_time_usec) / 1000);
      uv_stop(uv_default_loop());
    }
  }

  free(buf->base);
}

void send_cb(uv_udp_send_t *req, int status) {
  if (status) {
    fprintf(stderr, "error: %s\n", uv_strerror(status));
  }
}

int benchmark_udp() {
  struct sockaddr listen_addr;
  uv_loop_t *loop = uv_default_loop();
  uv_udp_t *recv_handle = malloc(sizeof(uv_udp_t));
  uv_udp_t *send_handle = malloc(sizeof(uv_udp_t));
  // int content_size = 10;
  // int times = 10;

  // 1. udp listen
  uv_udp_init(loop, recv_handle);

  uv_ip4_addr("0.0.0.0", 12354, (struct sockaddr_in *)&listen_addr);

  uv_udp_bind(recv_handle, &listen_addr, UV_UDP_REUSEADDR);

  // 2. start receive and count result
  uv_udp_recv_start(recv_handle, &alloc_cb, &recv_cb);

  // 3. init send_handle and send msg
  uv_udp_init(loop, send_handle);

  // 4. set options after init handles
  uv_send_buffer_size((uv_udp_t *)recv_handle, &recv_send_buf_size);
  uv_send_buffer_size((uv_udp_t *)send_handle, &recv_send_buf_size);
  uv_recv_buffer_size((uv_udp_t *)recv_handle, &recv_send_buf_size);
  uv_recv_buffer_size((uv_udp_t *)send_handle, &recv_send_buf_size);

  int value = 0;

  // NOTE: I can't get the correct buf content_size from uv.
  // int bufsize = uv_send_buffer_size((uv_udp_t *)recv_handle, &value);
  //
  // if (bufsize) {
  //   printf("error: %s\n", uv_strerror(bufsize));
  //   return bufsize;
  // }
  //
  // printf("buf content_size: %d\n", bufsize);

  for (int i = 0; i < times; i += 1) {
    char *msg = malloc(content_size);
    uv_udp_send_t *req = malloc(sizeof(uv_udp_send_t));
    uv_buf_t *buf = malloc(sizeof(uv_buf_t));

    memset(msg, (int)'a', content_size);

    buf->base = msg;
    buf->len = content_size;

    uv_udp_send(req, send_handle, buf, 1, &listen_addr, &send_cb);
  }

  get_time(&start_time_sec, &start_time_usec);

  // 4. start uv loop
  return uv_run(loop, UV_RUN_DEFAULT);
}

static long total_len = 0;

void recver_cb(kcpuv_sess *sess, char *data, int len) {
  // printf("recv: %d\n", len);
  // printf("content: %s\n", data);
  // kcpuv_stop_loop();
  get_time(&end_time_sec, &end_time_usec);

  IUINT32 elapsed = (end_time_sec - start_time_sec) * 1000 +
                    (end_time_usec - start_time_usec) / 1000;
  total_len += len;

  // printf("total len: %ld %ld\n", total_len, times * content_size);
  // printf("wnd: %d\n", sess->kcp->rmt_wnd);

  if (total_len == times * content_size) {
    fprintf(stdout, "time elapsed: %ld\n", elapsed, total_len);
    printf("total len: %ld\n", total_len);
    kcpuv_stop_loop();
  }
}

int benchmark_kcpuv_sess() {
  kcpuv_initialize();

  kcpuv_sess *sender = kcpuv_create();
  kcpuv_sess *recver = kcpuv_create();
  int send_port = 12305;
  int receive_port = 12306;

  // bind local
  kcpuv_listen(recver, receive_port, &recver_cb);
  kcpuv_init_send(recver, "0.0.0.0", send_port);
  kcpuv_listen(sender, send_port, NULL);
  kcpuv_init_send(sender, "0.0.0.0", receive_port);

  for (int i = 0; i < times; i += 1) {
    char *msg = malloc(sizeof(char) * content_size);
    memset(msg, (int)'a', content_size);
    kcpuv_send(sender, msg, content_size);
    free(msg);
  }

  get_time(&start_time_sec, &start_time_usec);

  kcpuv_start_loop(kcpuv__update_kcp_sess);

  kcpuv_destruct();
}

int main() {
  // return benchmark_udp();
  return benchmark_kcpuv_sess();
}
