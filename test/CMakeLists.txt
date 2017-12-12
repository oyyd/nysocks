cmake_minimum_required(VERSION 3.5)

project(kcpuv_test)

# cmake deps
include_directories(../deps/googletest/googletest/include)
include_directories(../deps/googletest/googlemock/include)
# openssl
include_directories(/usr/local/opt/openssl/include)

add_subdirectory(../deps/googletest/googlemock
  "${CMAKE_CURRENT_BINARY_DIR}/deps/googletest/googlemock")

# kcpuv
add_subdirectory(../
  "${CMAKE_CURRENT_BINARY_DIR}/kcpuv")

# libuv
include_directories(../deps/libuv/include)
# ikcp
include_directories(../deps/kcp)
include_directories(../src/)

add_executable(unit_test
  KcpuvTest.cpp
  KcpuvSessTest.cpp
  UtilsTest.cpp
  ProtocolTest.cpp
  MuxTest.cpp
)
target_link_libraries(unit_test
  kcpuv
  gmock_main
)
add_test(unit_test unit_test)

# known warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-incompatible-pointer-types")
