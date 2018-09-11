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
      'src/utils.c',
      'src/Loop.cc',
      'src/SessUDP.cc',
      'src/Cryptor.cc',
      'src/KcpuvSess.cc',
      'src/Mux.cc',
      'src/binding.cc',
    ],
    'conditions' : [
      ['OS=="win"', {
        'libraries' : ['ws2_32.lib']
      }]
    ],
    'cflags_cc!': ['-Wno-incompatible-pointer-types']
  }]
}
