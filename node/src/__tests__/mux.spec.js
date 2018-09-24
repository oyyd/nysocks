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
  getSessState,
  setSessWaitFinTimeout,
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
  connSendStop,
  bindOthersideEnd,
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

      setSessWaitFinTimeout(mux1.sess, 1000)
      setSessWaitFinTimeout(mux2.sess, 1000)

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
        connListen(conn2, msg => {
          expect(msg.toString('utf8')).toBe('hello')
          connClose(conn2)
        })

        conn2.event.on('close', () => {
          muxClose(mux1)
          muxClose(mux2)
        })
      })

      connSend(conn1, Buffer.from('hello'))
    })
  })

  // TODO: refactor
  it('should work with bindUdpSend', done => {
    startUpdaterTimer()

    const mux = createMux({
      password: 'hello',
      port: 0,
      targetPort: 20005,
      targetAddr: '0.0.0.0',
      passive: false,
    })
    const conn = createMuxConn(mux)

    mux.event.on('close', () => {
      stopUpdaterTimer()
      done()
    })

    bindUdpSend(mux.sess, msg => {
      expect(msg).toBeTruthy()
      muxClose(mux)
    })

    connSend(conn, Buffer.from('h'))
  })

  it('should also free all relative conns of muxes when freed', done => {
    startUpdaterTimer()

    const mux = createMux({
      password: 'hello',
      port: 0,
      targetAddr: '0.0.0.0',
      targetPort: 20000,
      passive: false,
    })

    const conn = createMuxConn(mux, null)
    let called = false

    conn.event.on('close', () => {
      called = true
    })

    mux.event.on('close', () => {
      expect(isConnFreed(conn)).toBeTruthy()
      expect(called).toBeTruthy()

      stopUpdaterTimer()
      done()
    })

    muxClose(mux)
  })

  describe('connSend', () => {
    it('should send a large buffer', done => {
      startUpdaterTimer()

      const TIMES = 10
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
      // TODO: refactor
      setSessWaitFinTimeout(mux1.sess, 500)
      setSessWaitFinTimeout(mux2.sess, 500)

      const conn1 = createMuxConn(mux1)
      const BUFFER_LENGTH = 64 * 1024

      let t = 0

      const tryClose = () => {
        t += 1
        if (t === 2) {
          muxClose(mux1)
          muxClose(mux2)
        }
      }

      const sendDataManyTimes = conn => {
        for (let i = 0; i < TIMES; i += 1) {
          connSend(conn, Buffer.alloc(BUFFER_LENGTH, 0))
        }
      }

      muxBindConnection(mux2, conn2 => {
        let receive = 0
        connListen(conn2, () => {
          receive += 1
          if (receive === TIMES) {
            tryClose()
          }
        })
        sendDataManyTimes(conn2)
      })

      let receive = 0
      connListen(conn1, () => {
        receive += 1
        if (receive === TIMES) {
          tryClose()
        }
      })

      sendDataManyTimes(conn1)
    })
  })

  describe('connSendStop', () => {
    it('should stop sending but still receive msg after stopping sending', done => {
      startUpdaterTimer()

      const TIMES = 10
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
      // TODO: refactor
      setSessWaitFinTimeout(mux1.sess, 500)
      setSessWaitFinTimeout(mux2.sess, 500)

      const conn1 = createMuxConn(mux1)
      const BUFFER_LENGTH = 64 * 1024

      let t = 0

      const tryClose = () => {
        t += 1
        if (t === 2) {
          muxClose(mux1)
          muxClose(mux2)
        }
      }

      const sendDataManyTimes = conn => {
        for (let i = 0; i < TIMES; i += 1) {
          connSend(conn, Buffer.alloc(BUFFER_LENGTH, 0))
        }
      }

      muxBindConnection(mux2, conn2 => {
        let receive = 0
        connListen(conn2, () => {
          receive += 1
          if (receive === TIMES) {
            tryClose()
          }
        })

        bindOthersideEnd(conn2, () => {
          // Called.
        })
        sendDataManyTimes(conn2)
      })

      let receive = 0

      connListen(conn1, () => {
        receive += 1
        if (receive === TIMES) {
          tryClose()
        }
      })

      sendDataManyTimes(conn1)
      connSendStop(conn1)
    })
  })
})
