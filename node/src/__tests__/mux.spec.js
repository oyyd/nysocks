import { markFree, startKcpuv, stopKcpuv, stopListen, create, setAddr, getPort, bindUdpSend, destroy, _stopLoop } from '../socket'
import { connListen, connBindClose, muxBindConnection, connSendClose,
  connSend, createMux, createMuxConn, muxFree, connFree } from '../mux'

describe('mux', () => {
  const ADDR = '0.0.0.0'

  describe('createMux', () => {
    it('should create a wrapped mux object', (done) => {
      startKcpuv()

      const mux = createMux({
        password: 'hello',
        port: 0,
        targetAddr: '0.0.0.0',
        targetPort: 20000,
      })
      expect(mux).toBeTruthy()

      _stopLoop()

      setTimeout(() => {
        muxFree(mux)
        stopListen(mux.sess)
        markFree(mux.sess)
        stopKcpuv()
        done()
      }, 100)
    })
  })

  // describe('connSendClose', () => {
  //   it('should send a close msg to the other side', (done) => {
  //     startKcpuv()
  //
  //     const mux1 = createMux({
  //       password: 'hello',
  //       port: 0,
  //     })
  //     const mux2 = createMux({
  //       password: 'hello',
  //       port: 0,
  //     })
  //     const port1 = getPort(mux1.sess)
  //     const port2 = getPort(mux2.sess)
  //     setAddr(mux1.sess, ADDR, port2)
  //     setAddr(mux2.sess, ADDR, port1)
  //
  //     const conn1 = createMuxConn(mux1)
  //
  //     muxBindConnection(mux2, (conn2) => {
  //       connListen(conn2, (msg) => {
  //         expect(msg.toString('utf8')).toBe('hello')
  //       })
  //
  //       connBindClose(conn2, () => {
  //         _stopLoop()
  //
  //         setTimeout(() => {
  //           // connFree(conn1)
  //           // connFree(conn2)
  //           // muxFree(mux1)
  //           // muxFree(mux2)
  //           // stopListen(mux1.sess)
  //           // stopListen(mux2.sess)
  //           // markFree(mux1.sess)
  //           // markFree(mux2.sess)
  //         }, 10)
  //
  //         // TODO: refactor
  //         setTimeout(() => {
  //           // stopKcpuv()
  //           done()
  //         }, 100)
  //       })
  //     })
  //
  //     connSend(conn1, Buffer.from('hello'))
  //     connSendClose(conn1)
  //   })
  // })
  //
  // TODO: refactor
  // it('should work with bindUdpSend', (done) => {
  //   startKcpuv()
  //
  //   const mux = createMux({
  //     password: 'hello',
  //     port: 0,
  //     targetPort: 20005,
  //     targetAddr: '0.0.0.0',
  //   })
  //   const conn = createMuxConn(mux)
  //
  //   bindUdpSend(mux.sess, (msg) => {
  //     expect(msg).toBeTruthy()
  //     _stopLoop()
  //
  //     muxFree(mux)
  //     stopListen(mux.sess)
  //     markFree(mux.sess)
  //
  //     setTimeout(() => {
  //       stopKcpuv()
  //       done()
  //     }, 100)
  //   })
  //
  //   connSend(conn, Buffer.from('h'))
  // })
})
