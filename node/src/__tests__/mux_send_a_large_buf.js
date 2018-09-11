import {
  startUpdaterTimer,
  stopUpdaterTimer,
  stopListen,
  create,
  setAddr,
  getPort,
  bindUdpSend,
  destroy,
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
} from '../mux'

startUpdaterTimer()

const ADDR = '0.0.0.0'
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
const BUFFER_LENGTH = 64 * 1024

muxBindConnection(mux2, conn2 => {
  let len = 0
  connListen(conn2, msg => {
    len += msg.length

    console.log(len)

    if (len === BUFFER_LENGTH) {
      console.log('DONE')
    }
  })

  // conn2.event.on('close', () => {
  //   stopUpdaterTimer()
  //   stopListen(mux1.sess)
  //   stopListen(mux2.sess)
  //   muxFree(mux1)
  //   muxFree(mux2)
  //
  //   // TODO: refactor
  //   setTimeout(() => {
  //     stopUpdaterTimer()
  //     done()
  //   }, 100)
  // })
})

connSend(conn1, Buffer.alloc(BUFFER_LENGTH, 0))
connSendClose(conn1)
