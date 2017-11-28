{
  'targets': [{
    'target_name': 'addon',
    'include_dirs': [
      "<!(node -e \"require('nan')\")",
      'lib/kcp',
      'src',
    ],
    'sources': [
      'lib/kcp/ikcp.c',
      'src/kcpuv.c',
      'src/kcpuv_sess.c',
      'src/utils.c',
      'src/protocol.c',
      'src/mux.c',
      'src/loop.c',
      'node/binding.cc',
    ],
  }]
}
