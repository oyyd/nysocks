import {
  getPort, createWithOptions, close as sessClose,
  startKcpuv, listen as socketListen, setAddr,
  initCryptor,
  // bindListener, send,
} from './socket'
import {
  createMux, createMuxConn,
  muxBindConnection, connFree, connSend, connListen,
  // connSendClose, muxFree, connBindClose, muxBindClose,
} from './mux'
import { getIP } from './utils'

export const DEFAULT_SERVER_PORT = 20000

const DEFAULT_OPTIONS = {
  serverAddr: '0.0.0.0',
  serverPort: DEFAULT_SERVER_PORT,
  socketAmount: 100,
}

const CONVERSATION_START_CHAR = '\\\\start'
const CONVERSATION_END_CHAR = '\\\\end'
// const MSG_END_SIGNAL = '\\\\end'

let kcpuvStarted = false

function start() {
  if (!kcpuvStarted) {
    kcpuvStarted = true
    startKcpuv()
  }
}

// function checkMsgEnd(pre, cur) {
//   const next = Buffer.concat([pre, cur])
//   const isEnd = next.slice(0, MSG_END_SIGNAL.length)
//     .toString('utf8') === MSG_END_SIGNAL
//
//   return [next, isEnd]
// }

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

export function initClientSocket(mux) {
  return new Promise((resolve) => {
    const msg = Buffer.from(CONVERSATION_START_CHAR)
    let data = Buffer.allocUnsafe(0)

    const conn = createMuxConn(mux)
    connListen(conn, (buf) => {
      data = Buffer.concat([data, buf])

      const res = checkJSONMsg(data)

      if (res) {
        resolve(res)
      }
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

    info.mux = mux
    info.socket = socket
    info.targetSocketPort = port
    info.targetSocketAddr = serverAddr

    conns.push(info)
  })

  return Object.assign({}, client, {
    conns,
  })
}

// TODO: throw when failed to connect
export function createClient(_options) {
  start()

  const options = Object.assign({}, DEFAULT_OPTIONS, _options)
  const { serverAddr, serverPort } = options
  const client = {
    state: 0,
    ports: [],
    _roundCur: 0,
  }
  let ipAddr = null
  const masterSocket = createWithOptions(options.kcp)
  initCryptor(masterSocket, options.password)
  socketListen(masterSocket, 0)
  const masterMux = createMux({
    sess: masterSocket,
  })

  client.masterSocket = masterSocket
  client.masterMux = masterMux

  return getIP(serverAddr)
    .then((_ipAddr) => {
      ipAddr = _ipAddr
      setAddr(masterSocket, ipAddr, serverPort)
      return initClientSocket(masterMux)
    })
    .then((ports) => {
      client.ports = ports
      client.state = 1
      return client
    })
    .then(c => initClientConns(options, c, ipAddr))
}

export function closeClient(client) {
  const { masterSocket, conns } = client

  conns.forEach(conn => sessClose(conn.socket, true))
  sessClose(masterSocket)
}

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

export function createManager(_options, onConnection) {
  start()

  const options = Object.assign({}, DEFAULT_OPTIONS, _options)
  const { serverPort } = options
  const conns = []
  const manager = {
    conns,
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
      const shouldReply = checkHandshakeMsg(buf)

      if (!shouldReply) {
        return
      }

      const sockets = createPassiveSockets(manager, options)
      const connInfo = {
        sockets,
      }

      conns.push(connInfo)
      sendJson(conn, getConnectionPorts(sockets))
    })
  })

  return manager
}

export const sendBuf = connSend

export const listen = connListen

export const close = connFree

export function bindConnection(manager, next) {
  manager.onConnection = next
}

export function createConnection(client) {
  let i = client._roundCur
  client._roundCur += 1

  if (i >= client.conns.length) {
    i = 0
  }

  const info = client.conns[i]
  const conn = createMuxConn(info.mux)

  return conn
}

// if (module === require.main) {
//   const message = Buffer.alloc(4 * 1024 * 1024)
//
//   const manager = createManager({ socketAmount: 2 }, (conn) => {
//     listen(conn, (buf) => {
//       console.log('manager received:', buf.length)
//       sendBuf(conn, Buffer.from('I have received'))
//     })
//   })
//
//   createClient(null).then(client => {
//     const conn = createConnection(client)
//
//     listen(conn, (buf) => {
//       console.log('client:', buf.toString('utf8'))
//     })
//
//     sendBuf(conn, message)
//   }).catch(err => {
//     console.error(err)
//   })
// }
