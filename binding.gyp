{
  'targets': [{
    'target_name': 'addon',
    'include_dirs': [
      "<!(node -e \"require('nan')\")",
      'deps/kcp',
      'src',
    ],
    'sources': [
      'deps/kcp/ikcp.c',
      'src/kcpuv_sess.c',
      'src/utils.c',
      'node/binding.cc',
    ],
  }]
}
