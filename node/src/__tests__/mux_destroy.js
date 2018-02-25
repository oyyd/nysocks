import {
  startKcpuv,
  stopKcpuv,
  stopListen,
  create,
  setAddr,
  getPort,
  bindUdpSend,
  destroy,
  _stopLoop,
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
  startKcpuv()

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
      stopKcpuv()
    })
  })

  // muxFree(mux)
  stopListen(mux.sess)
  destroy(mux.sess)
}

main()
