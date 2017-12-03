import ip from 'ip'
import {
  createServer as createSocksServer,
  createProxyConnection,
  parseDstInfo,
} from './socks_proxy_server'
import {
  createManager,
  createClient as createManagerClient,
  listen,
  sendBuf,
  createConnection,
  close,
  bindClose,
  sendClose,
  startKcpuv,
  stopKcpuv,
} from './socket_manager'
import { createRouter } from './router'
import { logger } from './logger'

let kcpuvStarted = false

function start() {
  if (kcpuvStarted) {
    return
  }

  kcpuvStarted = true
  startKcpuv()
}

export function createClient(config, _onClose) {
  start()

  const onClose = () => {
    stopKcpuv()
    _onClose()
  }

  return createManagerClient(config, onClose).then(managerClient => {
    const client = {}
    const socksServer = createSocksServer(config.SOCKS, (info, socket) => {
      const { chunk } = info

      // tunnel
      const conn = createConnection(managerClient)

      bindClose(conn, () => {
        // TODO: error msg
        socket.destroy()
        close(conn)
      })

      sendBuf(conn, chunk)

      // bind
      listen(conn, buf => {
        socket.write(buf)
      })
      socket.on('data', buf => {
        sendBuf(conn, buf)
      })
      socket.on('error', err => {
        logger.error(err.message)
      })
      socket.on('close', () => {
        sendClose(conn)
        close(conn)
      })
    })
    client.managerClient = managerClient
    client.socksServer = socksServer

    return client
  })
}

export function createServer(config, onClose) {
  start()

  const server = {}

  const managerServer = createManager(config, conn => {
    let firstBuf = true
    let socket = null

    listen(conn, buf => {
      if (firstBuf) {
        firstBuf = false
        const dstInfo = parseDstInfo(buf)

        // drop invalid msg
        if (!dstInfo) {
          // throw new Error('invalid dstInfo')
          close(conn)
          return
        }

        const options = {
          port: dstInfo.dstPort.readUInt16BE(),
          host: dstInfo.atyp === 3
            ? dstInfo.dstAddr.toString('ascii')
            : ip.toString(dstInfo.dstAddr),
        }
        socket = createProxyConnection(options)
        socket.on('data', buffer => {
          sendBuf(conn, buffer)
        })
        socket.on('error', err => {
          logger.error(err.message)
        })
        socket.on('close', () => {
          sendClose(conn)
          close(conn)
        })
        bindClose(conn, () => {
          // TODO: error msg
          socket.destroy()
          close(conn)
        })
        return
      }

      socket.write(buf)
    })
  }, onClose)

  server.managerServer = managerServer
  return server
}

export function createServerRouter(config) {
  const router = createRouter(
    {
      listenPort: config.serverPort,
    },
    (options) => {
      const serverConfig = Object.assign({}, config, {
        serverPort: 0,
      })
      const server = createServer(serverConfig, () => {
        // on close
        router.onServerClose(options)
      })

      return server.managerServer.masterSocket
    },
  )

  return router
}

// if (module === require.main) {
//   const config = {
//     serverAddr: '0.0.0.0',
//     serverPort: 20000,
//     socketAmount: 100,
//     password: 'HELLO',
//     kcp: {
//       sndwnd: 2048,
//       rcvwnd: 2048,
//       nodelay: 1,
//       interval: 10,
//       resend: 2,
//       nc: 1,
//     },
//     pac: {
//       pacServerPort: 8091,
//     },
//     SOCKS: {},
//   }
//
//   createServerRouter(config)
//   createClient(config)
// }
