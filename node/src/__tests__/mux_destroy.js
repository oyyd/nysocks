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

function main() {
  startUpdaterTimer()

  const mux = createMux({
    password: 'hello',
    port: 0,
    targetAddr: '0.0.0.0',
    targetPort: 20000,
  })

  const conn = createMuxConn(mux, null)

  conn.event.on('close', () => {
    // console.log('conn', conn.isClosed)
  })

  // connBindClose(conn, () => {
  //   console.log('ENTER')
  // })


  mux.event.on('close', () => {
    // console.log('mux', conn.isClosed)
    // expect(isConnFreed(conn)).toBeTruthy()

    setTimeout(() => {
      stopUpdaterTimer()
    })
  })

  // muxFree(mux)
  stopListen(mux.sess)
  destroy(mux.sess)
}

main()
