import {
  startKcpuv, create, send, bindListener,
  input, getPort, listen, initCryptor, setAddr,
  destroy,
} from '../socket'

startKcpuv()

describe('socket', () => {
  it('should send some data from a to b', (done) => {
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

      // TODO: refactor this
      setTimeout(() => {
        destroy(a)
        destroy(b)
        done()
      })
    })

    send(a, Buffer.from(message))
  })

  describe('input', () => {
    // TODO: refactor
    it('should put some data to a sess by calling input', () => {
      const addr = '0.0.0.0'
      const password = 'hello'
      const a = create()
      initCryptor(a, password)
      listen(a, 0)

      const buffer = Buffer.from('Hello')

      input(a, buffer, addr, 30000)
    })
  })
})
