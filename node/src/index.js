import ip from 'ip'
import Event from 'events'
import {
  createServer as createSocksServer,
  createProxyConnection,
  parseDstInfo,
} from './socks_proxy_server'
import {
  createServer as createSSServer,
} from './ss_proxy_server'
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
import { emptyFunc } from './utils'

let kcpuvStarted = false

const DEFAULT_SS_CONFIG = {
  password: 'YOUR_PASSWORD',
  method: 'aes-128-cfb',
  serverPort: 8083,
  udpActive: false,
  timeout: 600,
}

function start() {
  if (kcpuvStarted) {
    return
  }

  kcpuvStarted = true
  startKcpuv()
}

function ssProtocol(config, managerClient) {
  const ssConfig = Object.assign({}, DEFAULT_SS_CONFIG, config.SS)

  const { server } = createSSServer(ssConfig, (info, onConnect, chunk) => {
    // const { port, host } = info
    const mockSocket = new Event()
    // tunnel conn
    const conn = createConnection(managerClient)
    mockSocket.conn = conn

    // bind
    bindClose(conn, () => {
      mockSocket.emit('close')
      // close(conn)
    })
    listen(conn, (buf) => {
      mockSocket.emit('data', buf)
    })

    sendBuf(conn, chunk)

    mockSocket.write = (buf) => {
      sendBuf(conn, buf)
      return true
    }
    mockSocket.destroy = () => {
      sendClose(conn)
      close(conn)
      mockSocket.emit('close')
    }
    // TODO: support pausing connections
    mockSocket.resume = emptyFunc
    mockSocket.pause = emptyFunc
    // TODO: we don't have half open state
    mockSocket.end = emptyFunc

    onConnect()

    return mockSocket
  })

  logger.info(`SS server is listening on ${ssConfig.serverPort}`)

  return server
}

function socksProtocol(config, managerClient) {
  return createSocksServer(config.SOCKS, (info, socket) => {
    const { chunk } = info

    // tunnel
    const conn = createConnection(managerClient)

    // bind
    bindClose(conn, () => {
      // TODO: error msg
      socket.destroy()
      // console.log('conn_exit')
      // close(conn)
    })

    listen(conn, buf => {
      socket.write(buf)
    })

    socket.on('data', buf => {
      // console.log('socket_send')
      sendBuf(conn, buf)
    })

    socket.on('error', err => {
      logger.error(err.message)
    })

    socket.on('close', () => {
      sendClose(conn)
      // console.log('close_conn')
      close(conn)
    })

    sendBuf(conn, chunk)
  })
}

export function createClientInstance(config) {
  return createManagerClient(config).then(managerClient => {
    const { clientProtocol } = config
    const client = {
      managerClient,
    }

    client.proxyClient = null

    if (clientProtocol === 'SOCKS') {
      client.proxyClient = socksProtocol(config, managerClient)
      logger.info(`SOCKS5 service is listening on ${config.SOCKS.port}`)
    } else if (clientProtocol === 'SS') {
      client.proxyClient = ssProtocol(config, managerClient)
    } else {
      throw new Error(`unexpected "clientProtocol" option: ${clientProtocol}`)
    }

    return client
  })
}

export function createClient(config, onReconnect) {
  start()

  let networkMonitor = null
  const currentClients = {
    // 0 disconenct
    // 1 connecting
    // 2 connected
    connectState: 0,
    managerClient: null,
    proxyClient: null,
  }

  const free = () => new Promise((resolve) => {
    if (currentClients.connectState === 0) {
      resolve()
      return
    }

    currentClients.connectState = 0
    logger.info('client disconnect')

    // close
    if (currentClients.managerClient) {
      freeManager(currentClients.managerClient)
    }

    if (currentClients.proxyClient) {
      currentClients.proxyClient.close(() => {
        resolve()
      })
    }
    currentClients.proxyClient = null
    currentClients.managerClient = null
  })

  // Create new instance.
  const recreate = () => {
    logger.info('connecting...')
    currentClients.connectState = 1
    // TODO: refactor this
    // NOTE: wait for fd released to avoid errors
    setTimeout(() => {
      createClientInstance(config).then(({ managerClient, proxyClient }) => {
        currentClients.connectState = 2
        currentClients.managerClient = managerClient
        currentClients.proxyClient = proxyClient
        managerClient.on('close', () => {
          // eslint-disable-next-line
          closeAndTryRecreate()
        })

        // TODO:
        if (typeof onReconnect === 'function') {
          onReconnect()
        }
      }).catch(err => {
        // Create client failed
        setTimeout(() => {
          throw err
        })
      })
    })
  }

  const closeAndTryRecreate = () => {
    free().then(() => {
      const { ip: localIP } = networkMonitor

      // do not restart if there is no non-internal network
      if (localIP) {
        recreate()
      }
    }).catch(err => setTimeout(() => {
      throw err
    }))
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

  // setTimeout(() => {
  //   free().catch(err => setTimeout(() => {
  //     throw err
  //   }))
  // }, 3000)
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
          // close(conn)
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
//   console.log('encryptsocks', require('encryptsocks'))
// }
