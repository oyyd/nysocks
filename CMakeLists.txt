# NOTE: Cmake build is only for tests.
cmake_minimum_required(VERSION 3.5)

project(kcpuv)

# known warnings
# TODO: CMAKE_CXX_FLAGS doesn't work
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-incompatible-pointer-types -g")
add_compile_options(-g)
add_compile_options(-Wno-incompatible-pointer-types)

# libuv
include_directories(./deps/libuv/include)

# ikcp
include_directories(./lib/kcp)
add_library(ikcp ./lib/kcp/ikcp.h ./lib/kcp/ikcp.c)

# openssl
include_directories(/usr/local/opt/openssl/include)

# local sources
set(
  KCPUV_HEADERS
  src/utils.h
  src/kcpuv.h
  src/Loop.h
  src/SessUDP.h
  src/Cryptor.h
  src/KcpuvSess.h
)

set(
  KCPUV_SOURCES
  src/utils.c
  src/Loop.cc
  src/SessUDP.cc
  src/Cryptor.cc
  src/KcpuvSess.cc
)

add_library(kcpuv ${KCPUV_SOURCES} ${KCPUV_HEADERS})

# NOTE: Make sure both the libs of libuv.a and libcrypto.a exist.
# See ../tools/build.js for how to build libuv indepently.
target_link_libraries(kcpuv
  ikcp
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/libuv/out/Debug/libuv.a
  /usr/local/opt/openssl/lib/libcrypto.a
)

# add_executable(benchmark_udp
#   src/benchmark_udp.c
# )
#
# add_executable(benchmark_openssl
#   src/benchmark_openssl.c
# )

# benchmark
# target_link_libraries(benchmark_udp
#   kcpuv
# )

# target_link_libraries(benchmark_openssl
#   kcpuv
# )
