import { sendSocket, lockOne, releaseOne,
  createClient, createManager, listen, sendBuf } from './socket_manager'

export function client() {
  const clientInfo = {}

  return clientInfo
}

export function server() {

}

if (module === require.main) {
  // eslint-disable-next-line
  const net = require('net')

  const port = 20011

  const managerClient = createClient({ socketAmount: 3 }, (info, buf) => {
    // clientTCPSocket.
  })

  const manager = createManager(null, (info, buf) => {

  })

  const tcpServer = net.createServer((connection) => {
    const conn = lockOne(managerClient)

    connection.on('data', (buf) => {
      console.log('tcp_server_buf', buf)
      sendSocket(conn, buf)
    })

    connection.on('close', () => {
      releaseOne(managerClient, conn)
    })
  })

  tcpServer.listen(port)

  const clientTCPSocket = net.createConnection({
    host: '0.0.0.0',
    port,
  }, () => {
    clientTCPSocket.write(Buffer.from('Hello'))
  })

  // clientTCPSocket.writable.on('data', (buf) => {
  //   console.log('data', buf)
  // })
  //
  // // clientTCPSocket.write(Buffer.from('Hello'))
  // clientTCPSocket.on('data', (data) => {
  //   sendSocket(conn, data)
  // })
}
