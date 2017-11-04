import * as binding from '../socket'

function main() {
  binding.startKcpuv()

  const sess = binding.create()
  const sender = binding.create()

  binding.listen(sess, 0, (msg) => {
    console.log('msg', msg.toString('utf8'))
  })
  binding.listen(sender, 0, () => {})

  const senderPort = binding.getPort(sender)
  const sessPort = binding.getPort(sess)

  console.log('port', sessPort, senderPort)

  binding.setAddr(sender, '0.0.0.0', sessPort)
  binding.setAddr(sess, '0.0.0.0', senderPort)

  const msg = Buffer.from('hello')
  binding.send(sender, msg, msg.length)
}
if (module === require.main) {
  main()
}
