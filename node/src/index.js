import ip from 'ip'
import Event from 'events'
import {
  createServer as createSocksServer,
  createProxyConnection,
  parseDstInfo,
} from './socks_proxy_server'
import { createServer as createSSServer } from './ss_proxy_server'
import {
  createManager,
  createClient as createManagerClient,
  listen,
  sendBuf,
  createConnection,
  close,
  sendClose,
  startUpdaterTimer,
  freeManager,
  bindEnd,
  sendStop,
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
  startUpdaterTimer()
}

function createException(msg) {
  if (!msg) {
    return undefined
  }

  let error
  if (msg) {
    error = new Error()
    error.code = msg
    error.errno = msg
  }
  return error
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
    conn.event.on('close', (msg) => {
      mockSocket.emit('close', createException(msg))
    })

    listen(conn, buf => {
      mockSocket.emit('data', buf)
    })

    sendBuf(conn, chunk)

    mockSocket.write = buf => {
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
    // TODO: We don't need to use 'end' event.
    // tunnel
    const conn = createConnection(managerClient)
    // TODO: Pass error code.
    // let errorMsg

    bindEnd(conn, () => {
      socket.end()
    })
    // bind
    conn.event.on('close', (msg) => {
      // TODO: error msg
      // socket.destroy(createException(msg))
      socket.destroy()
    })

    listen(conn, buf => {
      socket.write(buf)
    })

    socket.on('data', buf => {
      sendBuf(conn, buf)
    })

    socket.on('error', err => {
      // errorMsg = err.code
      logger.error(err)
    })

    socket.on('end', () => {
      sendStop(conn)
    })

    socket.on('close', () => {
      sendClose(conn)
      close(conn)
    })

    sendBuf(conn, chunk)
  })
}

export async function createClientInstance(config) {
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

  const free = () =>
    new Promise(resolve => {
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
      createClientInstance(config)
        .then(({ managerClient, proxyClient }) => {
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
        })
        .catch(err => {
          // Create client failed
          setTimeout(() => {
            throw err
          })
        })
    })
  }

  const closeAndTryRecreate = () => {
    free()
      .then(() => {
        const { ip: localIP } = networkMonitor

        // do not restart if there is no non-internal network
        if (localIP) {
          recreate()
        }
      })
      .catch(err =>
        setTimeout(() => {
          throw err
        }),
      )
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

export async function createServer(config, onClose) {
  start()

  const server = {}

  const managerServer = await createManager(
    config,
    // on connection
    conn => {
      let firstBuf = true
      let socket = null
      // let errorMsg

      listen(conn, buf => {
        if (firstBuf) {
          firstBuf = false
          const dstInfo = parseDstInfo(buf)

          // drop invalid msg
          if (!dstInfo) {
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
            // errorMsg = err.code
            logger.error(err)
          })
          socket.on('close', () => {
            sendClose(conn)
            close(conn)
          })
          socket.on('end', () => {
            sendStop(conn)
          })
          bindEnd(conn, () => {
            socket.end()
          })
          conn.event.on('close', (msg) => {
            // socket.destroy(createException(msg))
            socket.destroy()
          })
          return
        }
        socket.write(buf)
      })
    },
    // on close
    onClose,
  )

  server.managerServer = managerServer
  return server
}

export function createServerRouter(config) {
  const router = createRouter(
    {
      listenPort: config.serverPort,
    },
    options => {
      const serverConfig = Object.assign({}, config, {
        serverPort: 0,
      })
      let server = null

      return createServer(serverConfig, () => {
        freeManager(server.managerServer)
        router.onServerClose(options)
      }).then(s => {
        server = s
        return server.managerServer.masterSocket
      })
    },
  )

  return router
}
