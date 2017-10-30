import { create, listen, stopListen,
  setAddr, send, close, startKcpuv, stopKcpuv } from '../socket'

function main() {
  startKcpuv()

  const addr = '127.0.0.1'
  const senderPort = 8888
  const receiverPort = 8889
  const msg = 'hello'

  const sender = create()
  const receiver = create()

  listen(sender, senderPort, (buf) => {
    console.log('sender: ', buf)
  })
  listen(receiver, receiverPort, (buf) => {
    const content = buf.toString('utf8')
    console.log('receive: ', content)

    if (content === msg) {
      stopListen(sender)
      stopListen(receiver)
      close(sender)
      close(receiver)
      stopKcpuv()
    }
  })
  setAddr(sender, addr, receiverPort)
  setAddr(receiver, addr, senderPort)

  const buf = Buffer.from(msg)

  send(sender, buf, buf.length)

  console.log('binding finished')
}

main()
