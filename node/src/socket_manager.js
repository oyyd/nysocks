import EventEmitter from 'events'
import {
  getPort, createWithOptions,
  // close as sessClose,
  listen as socketListen, setAddr,
  initCryptor, markFree, stopListen,
  // bindListener, send, startKcpuv,
} from './socket'
import {
  createMux, createMuxConn,
  muxBindConnection, connFree, connSend, connListen,
  connSendClose, connBindClose, connSetTimeout,
  muxFree,
} from './mux'
import { getIP, debug } from './utils'

export { startKcpuv, stopKcpuv } from './socket'

const TOTAL_TIMEOUT = (debug ? 10 : 60) * 1000
const BEATING_INTERVAL = TOTAL_TIMEOUT / 4
export const DEFAULT_SERVER_PORT = 20000

const DEFAULT_OPTIONS = {
  serverAddr: '0.0.0.0',
  serverPort: DEFAULT_SERVER_PORT,
  socketAmount: 100,
}

const BEATING_MSG = '\\\\beat'
const CONVERSATION_START_CHAR = '\\\\start'
const CONVERSATION_END_CHAR = '\\\\end'

function sendJson(conn, jsonMsg) {
  const content = Buffer.from(`${JSON.stringify(jsonMsg)}${CONVERSATION_END_CHAR}`)
  connSend(conn, content)
}

export function checkHandshakeMsg(buf) {
  return buf.slice(0, CONVERSATION_START_CHAR.length)
    .toString('utf8') === CONVERSATION_START_CHAR
}

export function checkJSONMsg(buf) {
  const { length } = buf
  const end = buf.slice(length - CONVERSATION_END_CHAR.length)
    .toString('utf8') === CONVERSATION_END_CHAR

  if (!end) {
    return null
  }

  let msg = null

  try {
    msg = JSON.parse(buf.slice(0, length - CONVERSATION_END_CHAR.length)
      .toString('utf8'))
  } catch (err) {
    throw new Error('invalid msg')
  }

  return msg
}

function initBeat(conn, next) {
  if (typeof next !== 'function') {
    throw new Error('expect next to be a function')
  }

  let timeoutTimerID = null
  let intervalID = null

  const resetTimeout = () => {
    clearTimeout(timeoutTimerID)
    timeoutTimerID = setTimeout(() => {
      // console.log('timeout')
      clearInterval(intervalID)
      next()
    }, TOTAL_TIMEOUT)
  }

  connListen(conn, (buf) => {
    // console.log(buf.toString('utf8'))
    if (buf.toString('utf8') === BEATING_MSG) {
      resetTimeout()
    }
  })

  resetTimeout()
  intervalID = setInterval(() => {
    connSend(conn, Buffer.from(BEATING_MSG))
  }, BEATING_INTERVAL)
}


export function freeManager(manager) {
  const { conns, masterMux, masterSocket } = manager

  if (Array.isArray(conns)) {
    conns.forEach((conn) => {
      const { mux, socket } = conn
      muxFree(mux)
      stopListen(socket)
      setTimeout(() => {
        markFree(socket)
      })
    })
  }

  muxFree(masterMux)
  stopListen(masterSocket)
  setTimeout(() => {
    markFree(masterSocket)
  })
}

export function initClientMasterSocket(mux) {
  return new Promise((resolve) => {
    const msg = Buffer.from(CONVERSATION_START_CHAR)
    let data = Buffer.allocUnsafe(0)

    const conn = createMuxConn(mux)
    connSetTimeout(conn, 0)
    connListen(conn, (buf) => {
      data = Buffer.concat([data, buf])

      const res = checkJSONMsg(data)

      if (res) {
        resolve([conn, res])
      }
    })

    connBindClose(conn, () => {
      connFree(conn)
    })

    // sess.event.on('close', (errorMsg) => {
    //   if (errorMsg) {
    //     throw new Error(errorMsg)
    //   }
    //
    //   // resolve(data)
    // })

    connSend(conn, msg)
  })
}

export const STATE = {
  NOT_CONNECT: 0,
  CONNECT: 1,
}

export function initClientConns(options, client, ipAddr) {
  const { serverAddr } = options
  const { ports } = client
  const conns = []

  ports.forEach((port) => {
    const info = {}
    const socket = createWithOptions(options.kcp)

    initCryptor(socket, options.password)
    socketListen(socket, 0)
    setAddr(socket, ipAddr, port)

    const mux = createMux({
      sess: socket,
    })

    // NOTE: This will happen when the previous conn has been closed
    // but the data comes from the other side.
    // muxBindConnection(mux, (conn) => {
    //   connSendClose(conn)
    //   connFree(conn)
    // })

    info.mux = mux
    info.socket = socket
    info.targetSocketPort = port
    info.targetSocketAddr = serverAddr

    conns.push(info)
  })

  return conns
}

// TODO: throw when failed to connect
export function createClient(_options) {
  const options = Object.assign({}, DEFAULT_OPTIONS, _options)
  const { serverAddr, serverPort } = options
  const client = new EventEmitter()

  client.state = 0
  client.ports = []
  client._roundCur = 0

  let ipAddr = null
  const masterSocket = createWithOptions(options.kcp)
  initCryptor(masterSocket, options.password)
  socketListen(masterSocket, 0)

  const masterMux = createMux({
    sess: masterSocket,
  })

  client.masterSocket = masterSocket
  client.masterMux = masterMux

  muxBindConnection(client.masterMux, (conn) => {
    connSendClose(conn)
    connFree(conn)
  })

  return getIP(serverAddr)
    .then((_ipAddr) => {
      ipAddr = _ipAddr
      setAddr(masterSocket, ipAddr, serverPort)
      return initClientMasterSocket(masterMux)
    })
    .then(([conn, ports]) => {
      // TODO:
      initBeat(conn, () => {
        client.emit('close')
      })

      client.ports = ports
      client.state = 1
      return client
    })
    .then(c => {
      const conns = initClientConns(options, c, ipAddr)
      client.conns = conns
      return client
    })
}

// export function closeClient(client) {
//   const { masterSocket, conns } = client
//
//   conns.forEach(conn => sessClose(conn.socket, true))
//   sessClose(masterSocket)
// }

export function getConnectionPorts(sockets) {
  return sockets.map(i => i.port)
}

function createPassiveSockets(manager, options) {
  const { socketAmount } = options
  const sockets = []

  const handleConn = (conn) => {
    if (typeof manager.onConnection === 'function') {
      manager.onConnection(conn)
    }
  }

  // create sockets for tunneling
  for (let i = 0; i < socketAmount; i += 1) {
    const info = {}
    const socket = createWithOptions(options.kcp)
    initCryptor(socket, options.password)
    socketListen(socket, 0)
    const port = getPort(socket)

    const mux = createMux({
      sess: socket,
    })

    muxBindConnection(mux, handleConn)

    // // TODO:
    // muxBindClose(() => {
    //   console.log('mux closed')
    // })
    info.mux = mux
    info.socket = socket
    info.port = port
    sockets.push(info)
  }

  return sockets
}

export function createManager(_options, onConnection, onClose) {
  const options = Object.assign({}, DEFAULT_OPTIONS, _options)
  const { serverPort } = options
  const manager = {
    conns: null,
    onConnection,
  }

  // create master socket
  const masterSocket = createWithOptions(options.kcp)
  initCryptor(masterSocket, options.password)
  socketListen(masterSocket, serverPort)
  const masterMux = createMux({
    sess: masterSocket,
  })
  manager.masterSocket = masterSocket
  manager.masterMux = masterMux

  // TODO:
  muxBindConnection(masterMux, (conn) => {
    connListen(conn, (buf) => {
      connSetTimeout(conn, 0)
      const shouldReply = checkHandshakeMsg(buf)

      if (!shouldReply) {
        return
      }

      const conns = createPassiveSockets(manager, options)
      manager.conns = conns

      sendJson(conn, getConnectionPorts(conns))

      initBeat(conn, () => {
        if (typeof onClose === 'function') {
          onClose()
        }
      })
    })

    connBindClose(conn, () => {
      connFree(conn)
    })
  })

  return manager
}

export const sendBuf = connSend

export const listen = connListen

export const close = connFree

export const bindClose = connBindClose

export const sendClose = connSendClose

export function bindConnection(manager, next) {
  manager.onConnection = next
}

export function createConnection(client) {
  let i = client._roundCur + 1

  if (i >= client.conns.length) {
    i = 0
  }

  client._roundCur = i

  const info = client.conns[i]
  const conn = createMuxConn(info.mux)

  return conn
}

// if (module === require.main) {
//   startKcpuv()
//
//   const serverAddr = '0.0.0.0'
//   const password = 'hello'
//   const serverPort = 20020
//   const socketAmount = 1
//
//   const manager = createManager({
//     password,
//     serverAddr,
//     serverPort,
//     socketAmount,
//   }, () => {})
//
//   createClient({
//     password,
//     serverAddr,
//     serverPort,
//     socketAmount,
//   }).then(client => {
//     // console.log('client', client)
//     // console.log('client', client)
//     freeManager(client)
//     freeManager(manager)
//   })
// }
