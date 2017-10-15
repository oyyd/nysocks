{
  'targets': [{
    'target_name': 'kcpuv',
    'type': 'executable',
    'dependencies': [
      'deps/libuv/uv.gyp:libuv'
    ],
    'include_dirs': [
      'deps/kcp',
      'deps/libuv/include',
      'src',
    ],
    'sources': [
      'deps/kcp/ikcp.c',
      'src/kcpuv_sess.c',
      'src/main.c',
      'src/utils.c'
    ],
  }]
}
