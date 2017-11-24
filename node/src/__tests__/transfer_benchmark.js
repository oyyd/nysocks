import {
  createMux, createMuxConn,
  muxBindConnection, connListen, connSend,
} from '../mux'
import {
  startKcpuv,
} from '../socket'

function main() {
  startKcpuv()

  const password = 'hello'
  const serverPort = 20001
  const bufSize = 1 * 1024 * 1024
  const clientOptions = {
    password,
    port: 0,
    targetPort: serverPort,
    targetAddr: '0.0.0.0',
  }
  const serverOptions = {
    password,
    port: serverPort,
  }

  const client = createMux(clientOptions)
  const server = createMux(serverOptions)
  const startTime = Date.now()
  let total = 0

  muxBindConnection(server, (conn) => {
    connListen(conn, buf => {
      total += buf.length
      console.log('buf', buf.length)
      if (total === bufSize) {
        console.log('total_time:', Date.now() - startTime, 'ms')
      }
    })
  })

  const clientConn = createMuxConn(client)
  const buf = Buffer.allocUnsafe(bufSize)
  connSend(clientConn, buf)
}

if (module === require.main) {
  main()
}
