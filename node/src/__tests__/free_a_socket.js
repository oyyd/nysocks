import { send, setAddr, listen, initCryptor, startUpdaterTimer, create, bindClose, destroy, close } from '../socket'

function main() {
  startUpdaterTimer()

  const sender = create()
  // const receiver = create()

  listen(sender, 8991, msg => {
    console.log('sender receive', msg)
  })
  // listen(receiver, 8992, msg => {
  //   console.log('receiver receive', msg)
  // })

  setAddr(sender, '0.0.0.0', 8992)
  // setAddr(receiver, '0.0.0.0', 8991)

  initCryptor(sender, 'key')

  // initCryptor(receiver, 'key')

  sender.event.on('close', () => {
    console.log('sender closed')
  })

  // receiver.event.on('close', () => {
  //   console.log('receiver closed')
  // })

  close(sender)
  // close(receiver)

  send(sender, Buffer.from('hello'))

  setInterval(() => {
    console.log('run')
  }, 1000)
}

main()
