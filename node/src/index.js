import ip from 'ip'
import { createServer as createSocksServer,
  createProxyConnection, parseDstInfo } from './socks_proxy_server'
import { createManager, createClient as createManagerClient,
  listen, sendBuf, createConnection } from './socket_manager'

export function createClient() {
  return createManagerClient(null).then((managerClient) => {
    const client = {}
    const socksServer = createSocksServer({}, (info, socket) => {
      const { chunk } = info

      // tunnel
      const conn = createConnection(managerClient)
      sendBuf(conn, chunk)

      // bind
      listen(conn, buf => {
        socket.write(buf)
      })
      socket.on('data', buf => {
        sendBuf(conn, buf)
      })
    })
    client.managerClient = managerClient
    client.socksServer = socksServer

    return client
  })
}

export function createServer() {
  const server = {}

  const managerServer = createManager({ socketAmount: 10 }, (conn) => {
    let firstBuf = true
    let socket = null

    listen(conn, buf => {
      if (firstBuf) {
        firstBuf = false
        const dstInfo = parseDstInfo(buf)

        if (!dstInfo) {
          throw new Error('invalid dstInfo')
        }

        const options = {
          port: dstInfo.dstPort.readUInt16BE(),
          host: (dstInfo.atyp === 3
            ? dstInfo.dstAddr.toString('ascii') : ip.toString(dstInfo.dstAddr)),
        }
        socket = createProxyConnection(options)
        socket.on('data', buffer => {
          sendBuf(conn, buffer)
        })
        return
      }

      socket.write(buf)
    })
  })

  server.managerServer = managerServer
  return server
}

if (module === require.main) {
  createClient()
  createServer()
}
