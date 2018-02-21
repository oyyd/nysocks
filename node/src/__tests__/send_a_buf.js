import { create, listen, stopListen, destroy,
  setAddr, send, close, startKcpuv, stopKcpuv, initCryptor } from '../socket'

function main() {
  startKcpuv()

  const addr = '127.0.0.1'
  const senderPort = 18888
  const receiverPort = 18889
  let msg = 'hellohellohellohello'

  for (let i = 0; i < 5; i += 1) {
    msg += msg
  }

  const key = 'keykeykeykey'

  const sender = create()
  const receiver = create()

  listen(sender, senderPort, (buf) => {
    console.log('sender: ', buf)
  })

  listen(receiver, receiverPort, (buf) => {
    const content = buf.toString('utf8')
    // console.log('receive: ', content)

    if (content === msg) {
      // close(sender)
      // close(receiver)
      stopListen(sender)
      stopListen(receiver)
      destroy(sender)
      destroy(receiver)

      setTimeout(() => {
        stopKcpuv()
      }, 1000)
    }
  })

  initCryptor(sender, key)
  initCryptor(receiver, key)

  setAddr(sender, addr, receiverPort)
  setAddr(receiver, addr, senderPort)

  const buf = Buffer.from(msg)

  send(sender, buf, buf.length)
}

main()
