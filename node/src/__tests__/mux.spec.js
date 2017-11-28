import { startKcpuv, stopKcpuv, setAddr, getPort, bindUdpSend } from '../socket'
import { connListen, connBindClose, muxBindConnection, connSendClose,
  connSend, createMux, createMuxConn, muxFree, connFree } from '../mux'

describe('mux', () => {
  const ADDR = '0.0.0.0'

  beforeEach(() => {
    startKcpuv()
  })

  afterEach(() => {
    stopKcpuv()
  })

  describe('createMux', () => {
    it('should create a wrapped mux object', () => {
      const obj = createMux({
        password: 'hello',
        port: 0,
      })

      expect(obj).toBeTruthy()

      muxFree(obj)
    })
  })

  describe('connSendClose', () => {
    it('should send a close msg to the other side', (done) => {
      const mux1 = createMux({
        password: 'hello',
        port: 0,
      })
      const mux2 = createMux({
        password: 'hello',
        port: 0,
      })
      const port1 = getPort(mux1.sess)
      const port2 = getPort(mux2.sess)
      setAddr(mux1.sess, ADDR, port2)
      setAddr(mux2.sess, ADDR, port1)

      const conn1 = createMuxConn(mux1)

      muxBindConnection(mux2, (conn2) => {
        connListen(conn2, (msg) => {
          expect(msg.toString('utf8')).toBe('hello')
        })
        connBindClose(conn2, () => {
          connFree(conn2)
          done()
        })
      })

      connSend(conn1, Buffer.from('hello'))
      connSendClose(conn1)
      connFree(conn1)
    })
  })

  // TODO: refactor
  it('should work with bindUdpSend', () => {
    const mux = createMux({
      password: 'hello',
      port: 0,
      targetPort: 20005,
      targetAddr: '0.0.0.0',
    })
    const conn = createMuxConn(mux)

    bindUdpSend(mux.sess, (msg) => {
      // console.log('msg', msg)
      // muxFree(mux)
      // done()
    })

    // connSend(conn, Buffer.from('h'))
  })
})
