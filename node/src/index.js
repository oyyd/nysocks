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
  freeManager,
} from './socket_manager'
import { createMonitor } from './network_monitor'
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

function createClientInstance(config) {
  return createManagerClient(config).then(managerClient => {
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

export function createClient(config) {
  start()

  let networkMonitor = null
  const currentClients = {
    // 0 disconenct
    // 1 connecting
    // 2 connected
    connectState: 0,
    managerClient: null,
    socksServer: null,
  }

  const free = () => {
    currentClients.connectState = 0
    // close
    if (currentClients.managerClient) {
      freeManager(currentClients.managerClient)
    }
    if (currentClients.socksServer) {
      currentClients.socksServer.close()
    }
    currentClients.socksServer = null
    currentClients.managerClient = null
  }

  // Create new instance.
  const recreate = () => {
    logger.info('connecting...')
    currentClients.connectState = 1
    createClientInstance(config).then(({ managerClient, socksServer }) => {
      logger.info(`SOCKS5 service is listening on ${config.SOCKS.port}`)

      currentClients.connectState = 2
      currentClients.managerClient = managerClient
      currentClients.socksServer = socksServer
      // eslint-disable-next-line
      managerClient.on('close', closeAndTryRecreate)
    }).catch(err => {
      // Create client failed
      setTimeout(() => {
        throw err
      })
    })
  }

  const closeAndTryRecreate = () => {
    logger.info('client disconnect')

    free()

    const { ip: localIP } = networkMonitor

    // do not restart if there is no non-internal network
    if (localIP) {
      recreate()
    }
  }

  // ip changed
  networkMonitor = createMonitor(() => {
    const { ip: localIP } = networkMonitor

    if (localIP && currentClients.connectState !== 1) {
      closeAndTryRecreate()
    }
  })

  // Create first time.
  recreate()
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
        // TODO:
        // on close
        freeManager(server.managerServer)
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
