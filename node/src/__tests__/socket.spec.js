import dgram from 'dgram'
import {
  getSessState,
  close,
  startUpdaterTimer,
  stopUpdaterTimer,
  create,
  send,
  bindListener,
  input,
  getPort,
  listen,
  initCryptor,
  setAddr,
  destroy,
  bindUdpSend,
  stopListen,
  createWithOptions,
  setSessWaitFinTimeout,
} from '../socket'

describe('socket', () => {
  it('should exit automatically when all handles are freed', done => {
    startUpdaterTimer()

    const sess = create(false)

    sess.event.on('close', () => {
      stopUpdaterTimer()
      done()
    })

    close(sess)
  })

  it('should send some data from a to b', done => {
    startUpdaterTimer()

    const addr = '0.0.0.0'
    const password = 'hello'
    const message = 'hello'
    const a = create(false)
    const b = create(true)
    let closed = 0

    const closeLoop = () => {
      closed += 1

      if (closed !== 2) {
        return
      }

      stopUpdaterTimer()
      done()
    }

    a.event.on('close', () => {
      expect(getSessState(a)).toBe('KCPUV_STATE_WAIT_FREE')
      closeLoop()
    })
    b.event.on('close', () => {
      expect(getSessState(a)).toBe('KCPUV_STATE_WAIT_FREE')
      closeLoop()
    })

    initCryptor(a, password)
    initCryptor(b, password)

    listen(a, 0)
    listen(b, 0)

    const portA = getPort(a)
    const portB = getPort(b)

    expect(getSessState(a)).toBe('KCPUV_STATE_CREATED')
    expect(getSessState(b)).toBe('KCPUV_STATE_CREATED')

    expect(typeof portA === 'number' && portA !== 0).toBe(true)
    expect(typeof portB === 'number' && portB !== 0).toBe(true)

    setAddr(a, addr, portB)
    setAddr(b, addr, portA)

    bindListener(a, msg => {
      expect(msg.toString('utf8')).toBe(message)
      close(a)
      close(b)

      expect(getSessState(a)).toBe('KCPUV_STATE_FIN')
      expect(getSessState(b)).toBe('KCPUV_STATE_FIN')
    })

    bindListener(b, msg => {
      send(b, msg)
    })

    send(a, Buffer.from(message))
  })

  // describe('input', () => {
  //   // TODO: refactor
  //   it('should put some data to a sess by calling input', (done) => {
  //     startUpdaterTimer()
  //
  //     const addr = '0.0.0.0'
  //     const password = 'hello'
  //     const a = create()
  //     initCryptor(a, password)
  //     listen(a, 0)
  //
  //     const buffer = Buffer.from('Hello')
  //
  //     input(a, buffer, addr, 20000)
  //
  //     stopListen(a)
  //
  //     setTimeout(() => {
  //       stopUpdaterTimer()
  //       stopUpdaterTimer()
  //       done()
  //     }, 20)
  //   })
  // })

  describe('bindUdpSend', () => {
    it('should send msg by proxy', (done) => {
      startUpdaterTimer()

      const udpSocket = dgram.createSocket('udp4')
      const addr = '0.0.0.0'
      const password = 'hello'
      const message = 'hello'
      let closed = 0
      const a = create(false)
      const b = create(true)
      initCryptor(a, password)
      initCryptor(b, password)

      listen(a, 0)
      listen(b, 0)


      const closeLoop = () => {
        closed += 1

        if (closed !== 2) {
          return
        }

        udpSocket.close()
        stopUpdaterTimer()
        done()
      }

      a.event.on('close', () => {
        closeLoop()
      })
      b.event.on('close', () => {
        closeLoop()
      })

      const portA = getPort(a)
      const portB = getPort(b)

      setSessWaitFinTimeout(a, 500)
      setSessWaitFinTimeout(b, 500)

      expect(typeof portA === 'number').toBe(true)
      expect(typeof portB === 'number').toBe(true)

      setAddr(a, addr, portB)
      setAddr(b, addr, portA)

      bindListener(b, (msg) => {
        expect(msg.toString('utf8')).toBe(message)

        close(a)
        close(b)
      })

      bindUdpSend(b, (msg, address, port) => {
        udpSocket.send(msg, port, address)
      })

      send(a, Buffer.from(message))
    })
  })
})
