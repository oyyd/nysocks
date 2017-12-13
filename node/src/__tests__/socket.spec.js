import dgram from 'dgram'
import {
  startKcpuv, stopKcpuv, create, send, bindListener,
  input, getPort, listen, initCryptor, setAddr,
  destroy, bindUdpSend, markFree, _stopLoop,
  stopListen,
} from '../socket'

describe('socket', () => {
  it('should send some data from a to b', (done) => {
    startKcpuv()

    const addr = '0.0.0.0'
    const password = 'hello'
    const message = 'hello'
    const a = create()
    const b = create()
    initCryptor(a, password)
    initCryptor(b, password)
    listen(a, 0)
    listen(b, 0)

    const portA = getPort(a)
    const portB = getPort(b)

    expect(typeof portA === 'number').toBe(true)
    expect(typeof portB === 'number').toBe(true)

    setAddr(a, addr, portB)
    setAddr(b, addr, portA)

    bindListener(b, (msg) => {
      expect(msg.toString('utf8')).toBe(message)
      stopListen(a)
      stopListen(b)
      markFree(a)
      markFree(b)

      setTimeout(() => {
        _stopLoop()
        stopKcpuv()
        done()
      }, 100)
    })

    send(a, Buffer.from(message))
  })
  //
  // // describe('input', () => {
  // //   // TODO: refactor
  // //   it('should put some data to a sess by calling input', (done) => {
  // //     startKcpuv()
  // //
  // //     const addr = '0.0.0.0'
  // //     const password = 'hello'
  // //     const a = create()
  // //     initCryptor(a, password)
  // //     listen(a, 0)
  // //
  // //     const buffer = Buffer.from('Hello')
  // //
  // //     input(a, buffer, addr, 20000)
  // //
  // //     stopListen(a)
  // //     markFree(a)
  // //
  // //     setTimeout(() => {
  // //       _stopLoop()
  // //       stopKcpuv()
  // //       done()
  // //     }, 20)
  // //   })
  // // })
  //
  // describe('bindUdpSend', () => {
  //   // TODO: refactor
  //   it('should send msg by proxy', (done) => {
  //     startKcpuv()
  //
  //     const udpSocket = dgram.createSocket('udp4')
  //     const addr = '0.0.0.0'
  //     const password = 'hello'
  //     const message = 'hello'
  //     const a = create()
  //     const b = create()
  //     initCryptor(a, password)
  //     initCryptor(b, password)
  //     listen(a, 0)
  //     listen(b, 0)
  //
  //     const portA = getPort(a)
  //     const portB = getPort(b)
  //
  //     expect(typeof portA === 'number').toBe(true)
  //     expect(typeof portB === 'number').toBe(true)
  //
  //     setAddr(a, addr, portB)
  //     setAddr(b, addr, portA)
  //
  //     bindListener(b, (msg) => {
  //       expect(msg.toString('utf8')).toBe(message)
  //       _stopLoop()
  //
  //       stopListen(a)
  //       stopListen(b)
  //       markFree(a)
  //       markFree(b)
  //       udpSocket.close()
  //
  //       // TODO: refactor this
  //       setTimeout(() => {
  //         stopKcpuv()
  //         done()
  //       }, 100)
  //     })
  //
  //     bindUdpSend(b, (msg, address, port) => {
  //       udpSocket.send(msg, port, address)
  //     })
  //
  //     send(a, Buffer.from(message))
  //   })
  // })
})
