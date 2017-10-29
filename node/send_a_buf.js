const { initSend, listen, close, send, startLoop,
  stopListen, create, initialize } = require('../build/Release/addon.node')

function main() {
  initialize()

  const addr = '127.0.0.1'
  const sender_port = 8888
  const receiver_port = 8889

  const sender = create()
  const receiver = create()

  listen(sender, sender_port, (buf) => {
    console.log('sender: ', buf)
  })
  listen(receiver, receiver_port, (buf) => {
    console.log('receiver: ', buf.toString('utf8'))
  })
  initSend(sender, addr, receiver_port)
  initSend(receiver, addr, sender_port)

  const buf = Buffer.from('hello')

  send(sender, buf, buf.length)

  startLoop()

  console.log('binding finished')
}

main()
