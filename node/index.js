var listen_port = 8888
var binding = require('../build/Release/addon.node')

binding.initialize()

var instance = binding.create()

binding.initSend(instance, '127.0.0.1', listen_port)

binding.listen(instance, listen_port, (buf) => {
  return 'js_called'
})

binding.bindClose(instance, () => {
  console.log('bindClose called')
})

binding.close(instance, 0)

binding.stopListen(instance)

// binding.free(instance)

console.log('finished')
