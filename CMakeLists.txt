cmake_minimum_required(VERSION 3.5)

project(kcpuv)

# libuv
include_directories(./deps/libuv/include)

# ikcp
include_directories(./deps/kcp)
add_library(ikcp ./deps/kcp/ikcp.h ./deps/kcp/ikcp.c)

# openssl
include_directories(/usr/local/opt/openssl/include)

# local sources
set(
  KCPUV_HEADERS
  src/kcpuv_sess.h
  src/protocol.h
  src/utils.h
)

set(
  KCPUV_SOURCES
  src/kcpuv_sess.c
  src/protocol.c
  src/utils.c
)

add_library(kcpuv ${KCPUV_SOURCES} ${KCPUV_HEADERS})

target_link_libraries(kcpuv
  ikcp
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/libuv/build/Default/libuv.a
  /usr/local/opt/openssl/lib/libcrypto.a
)

add_executable(benchmark_udp
  src/benchmark_udp.c
)

add_executable(benchmark_openssl
  src/benchmark_openssl.c
)

# benchmark
target_link_libraries(benchmark_udp
  kcpuv
)

target_link_libraries(benchmark_openssl
  kcpuv
)

# known warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-incompatible-pointer-types")
