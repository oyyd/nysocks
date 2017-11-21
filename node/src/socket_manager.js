import dns from 'dns'
import ip from 'ip'
import {
  getPort, create, send, close as sessClose,
  startKcpuv, listen as socketListen, setAddr,
  initCryptor,
  // bindListener,
} from './socket'
import { createMux, createMuxConn, wrapMuxConn,
  muxBindConnection, connFree, connSend, connListen,
  // connSendClose, muxFree, connBindClose, muxBindClose,
} from './mux'

const DEFAULT_OPTIONS = {
  serverAddr: '0.0.0.0',
  serverPort: 20000,
  socketAmount: 100,
}

const CONVERSATION_START_CHAR = '\\\\start'
const CONVERSATION_END_CHAR = '\\\\end'

let kcpuvStarted = false

function getIP(address) {
  if (ip.isV4Format(address) || ip.isV6Format(address)) {
    return Promise.resolve(address)
  }

  return new Promise((resolve, reject) => {
    dns.lookup(address, (err, ipAddr) => {
      if (err) {
        reject(err)
        return
      }

      resolve(ipAddr)
    })
  })
}

function start() {
  if (!kcpuvStarted) {
    kcpuvStarted = true
    startKcpuv()
  }
}

function sendJson(sess, jsonMsg) {
  const content = Buffer.from(`${JSON.stringify(jsonMsg)}${CONVERSATION_END_CHAR}`)
  send(sess, content, content.length)
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

export function initClientSocket(sess, serverAddr, serverPort) {
  return new Promise((resolve) => {
    const msg = Buffer.from(CONVERSATION_START_CHAR)
    let data = Buffer.allocUnsafe(0)

    setAddr(sess, serverAddr, serverPort)
    socketListen(sess, 0, (buf) => {
      data = Buffer.concat([data, buf])

      const res = checkJSONMsg(data)

      if (res) {
        resolve(res)
      }
    })

    sess.event.on('close', (errorMsg) => {
      if (errorMsg) {
        throw new Error(errorMsg)
      }

      // resolve(data)
    })

    send(sess, msg, msg.length)
  })
}

export const CLIENT_STATE = {
  0: 'NOT_CONNECT',
  1: 'CONNECT',
}

export function initClientConns(options, client) {
  const { serverAddr } = options
  const { ports } = client
  const conns = []

  ports.forEach((port) => {
    const info = {}
    const socket = create()

    initCryptor(socket, options.password)
    socketListen(socket, 0)
    setAddr(socket, serverAddr, port)

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
  const masterSocket = create()
  initCryptor(masterSocket, options.password)

  client.masterSocket = masterSocket

  return getIP(serverAddr)
    .then(ipAddr => initClientSocket(masterSocket, ipAddr, serverPort))
    .then((ports) => {
      client.ports = ports
      client.state = 1
      return client
    })
    .then(c => initClientConns(options, c))
}

export function closeClient(client) {
  const { masterSocket, conns } = client

  conns.forEach(conn => sessClose(conn.socket, true))
  sessClose(masterSocket)
}

export function getConnectionPorts(manager) {
  return manager.conns.map(i => i.port)
}

export function createManager(_options, onConnection) {
  start()

  const options = Object.assign({}, DEFAULT_OPTIONS, _options)
  const { socketAmount, serverPort } = options
  const conns = []
  const manager = {
    conns,
    onConnection,
  }

  const handleConn = (conn) => {
    wrapMuxConn(conn)
    if (typeof manager.onConnection === 'function') {
      manager.onConnection(conn)
    }
  }

  // create master socket
  const masterSocket = create()
  initCryptor(masterSocket, options.password)
  manager.masterSocket = masterSocket

  // create sockets for tunneling
  for (let i = 0; i < socketAmount; i += 1) {
    const info = {}
    const socket = create()
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
    conns.push(info)
  }

  socketListen(masterSocket, serverPort, (buf) => {
    const shouldReply = checkHandshakeMsg(buf)

    if (shouldReply) {
      // TODO:
      sendJson(masterSocket, getConnectionPorts(manager))
    }
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
