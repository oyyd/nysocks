import {
  startUpdaterTimer,
  stopListen,
  create,
  setAddr,
  getPort,
  bindUdpSend,
  destroy,
  stopUpdaterTimer,
  close,
} from '../socket'
import {
  connListen,
  connBindClose,
  muxBindConnection,
  connSendClose,
  connSend,
  createMux,
  createMuxConn,
  muxFree,
  isConnFreed,
  muxClose,
  connClose,
} from '../mux'

describe('mux', () => {
  const ADDR = '0.0.0.0'

  describe('createMux', () => {
    it('should create a wrapped mux object', done => {
      startUpdaterTimer()

      const mux = createMux({
        password: 'hello',
        port: 0,
        targetAddr: '0.0.0.0',
        targetPort: 20000,
        passive: false,
      })

      expect(mux).toBeTruthy()

      mux.event.on('close', () => {
        stopUpdaterTimer()
        done()
      })

      muxClose(mux)
    })
  })

  describe('connSendClose', () => {
    it('should send a close msg to the other side', done => {
      startUpdaterTimer()

      const mux1 = createMux({
        password: 'hello',
        port: 0,
        passive: false,
      })
      const mux2 = createMux({
        password: 'hello',
        port: 0,
        passive: true,
      })

      let muxClosed = 0

      mux1.event.on('close', () => {
        muxClosed += 1

        if (muxClosed === 2) {
          stopUpdaterTimer()
          done()
        }
      })

      mux2.event.on('close', () => {
        muxClosed += 1

        if (muxClosed === 2) {
          stopUpdaterTimer()
          done()
        }
      })

      const port1 = getPort(mux1.sess)
      const port2 = getPort(mux2.sess)

      setAddr(mux1.sess, ADDR, port2)
      setAddr(mux2.sess, ADDR, port1)

      const conn1 = createMuxConn(mux1)

      muxBindConnection(mux2, conn2 => {
        // connListen(conn2, msg => {
        //   expect(msg.toString('utf8')).toBe('hello')
        // })
        //
        // conn2.event.on('close', () => {
        //   muxClose(mux1)
        //   muxClose(mux2)
        // })
      })

      connSend(conn1, Buffer.from('hello'))
      // connSendClose(conn1)
    })
  })

  // // TODO: refactor
  // it('should work with bindUdpSend', done => {
  //   startUpdaterTimer()
  //
  //   const mux = createMux({
  //     password: 'hello',
  //     port: 0,
  //     targetPort: 20005,
  //     targetAddr: '0.0.0.0',
  //   })
  //   const conn = createMuxConn(mux)
  //
  //   bindUdpSend(mux.sess, msg => {
  //     expect(msg).toBeTruthy()
  //     stopUpdaterTimer()
  //     muxFree(mux)
  //     stopListen(mux.sess)
  //
  //     setTimeout(() => {
  //       stopUpdaterTimer()
  //       done()
  //     }, 100)
  //   })
  //
  //   connSend(conn, Buffer.from('h'))
  // })
  //
  // it('should trigger onClose when the mux has been closed', (done) => {
  //   startUpdaterTimer()
  //
  //   const mux = createMux({
  //     password: 'hello',
  //     port: 0,
  //     targetAddr: '0.0.0.0',
  //     targetPort: 20000,
  //   })
  //
  //   mux.event.on('close', () => {
  //     setTimeout(() => {
  //       stopUpdaterTimer()
  //       done()
  //     })
  //   })
  //
  //   // muxFree(mux)
  //   stopListen(mux.sess)
  //   destroy(mux.sess)
  // })
  //
  // it('should also free all relative conns of muxes when freed', done => {
  //   startUpdaterTimer()
  //
  //   const mux = createMux({
  //     password: 'hello',
  //     port: 0,
  //     targetAddr: '0.0.0.0',
  //     targetPort: 20000,
  //   })
  //
  //   const conn = createMuxConn(mux, null)
  //   let called = false
  //
  //   conn.event.on('close', () => {
  //     called = true
  //   })
  //
  //   mux.event.on('close', () => {
  //     expect(isConnFreed(conn)).toBeTruthy()
  //     expect(called).toBeTruthy()
  //
  //     setTimeout(() => {
  //       stopUpdaterTimer()
  //       done()
  //     })
  //   })
  //
  //   muxFree(mux)
  //   stopListen(mux.sess)
  //   destroy(mux.sess)
  // })
  //
  // describe('connSend', () => {
  //   it('should send a large buffer', done => {
  //     startUpdaterTimer()
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
  //     const BUFFER_LENGTH = 64 * 1024
  //
  //     muxBindConnection(mux2, conn2 => {
  //       let len = 0
  //       connListen(conn2, msg => {
  //         len += msg.length
  //
  //         if (len === BUFFER_LENGTH) {
  //           setTimeout(() => {
  //             stopUpdaterTimer()
  //             stopListen(mux1.sess)
  //             stopListen(mux2.sess)
  //             muxFree(mux1)
  //             muxFree(mux2)
  //
  //             // TODO: refactor
  //             setTimeout(() => {
  //               stopUpdaterTimer()
  //               done()
  //             }, 100)
  //           })
  //         }
  //       })
  //     })
  //
  //     connSend(conn1, Buffer.alloc(BUFFER_LENGTH, 0))
  //     connSendClose(conn1)
  //   })
  // })
})
