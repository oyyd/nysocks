import { create, listen, stopListen, destroy,
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
      close(sender)
      close(receiver)
      stopListen(sender)
      stopListen(receiver)
      setTimeout(() => {
        stopKcpuv()
      }, 100)
    }
  })

  setAddr(sender, addr, receiverPort)
  setAddr(receiver, addr, senderPort)

  const buf = Buffer.from(msg)

  send(sender, buf, buf.length)
}

main()
