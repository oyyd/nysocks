import EventEmitter from 'events'
import { getPort } from './socket'
import {
  createMux,
  createMuxConn,
  muxBindConnection,
  connClose,
  connSend,
  connListen,
  connSendClose,
  connSetTimeout,
  muxClose,
  bindOthersideEnd,
  connSendStop,
  setWaitFinTimeout as _setWaitFinTimeout,
} from './mux'
import { getIP, debug } from './utils'

export { startUpdaterTimer, stopUpdaterTimer } from './socket'

const TOTAL_TIMEOUT = (debug ? 10 : 60) * 1000
const BEATING_INTERVAL = TOTAL_TIMEOUT / 6
const CONN_DEFAULT_TIMEOUT = 2 * 1000 * 60
export const DEFAULT_SERVER_PORT = 20000

const DEFAULT_OPTIONS = {
  serverAddr: '0.0.0.0',
  serverPort: DEFAULT_SERVER_PORT,
  socketAmount: 100,
}

const BEATING_MSG = '\\\\beat'
const CONVERSATION_START_CHAR = '\\\\start'
const CONVERSATION_END_CHAR = '\\\\end'

// class SocketRequester {}
//
// class SocketDistributor {}

// TODO: Move these detail into conn
function initConn(conn, onMsg) {
  connSetTimeout(conn, CONN_DEFAULT_TIMEOUT)
  connListen(conn, onMsg)

  conn.event.on('close', () => {})
}

function sendJson(conn, jsonMsg) {
  const content = Buffer.from(
    `${JSON.stringify(jsonMsg)}${CONVERSATION_END_CHAR}`,
  )
  connSend(conn, content)
}

export function checkHandshakeMsg(buf) {
  return (
    buf.slice(0, CONVERSATION_START_CHAR.length).toString('utf8') ===
    CONVERSATION_START_CHAR
  )
}

export function checkJSONMsg(buf) {
  const { length } = buf
  const end =
    buf.slice(length - CONVERSATION_END_CHAR.length).toString('utf8') ===
    CONVERSATION_END_CHAR

  if (!end) {
    return null
  }

  let msg = null

  try {
    msg = JSON.parse(
      buf.slice(0, length - CONVERSATION_END_CHAR.length).toString('utf8'),
    )
  } catch (err) {
    throw new Error('invalid msg')
  }

  return msg
}

function initBeat(conn, next) {
  if (typeof next !== 'function') {
    throw new Error('expect next to be a function')
  }

  const beatInfo = {
    timeoutTimerID: null,
    intervalID: null,
  }

  const resetTimeout = () => {
    clearTimeout(beatInfo.timeoutTimerID)

    beatInfo.timeoutTimerID = setTimeout(() => {
      clearInterval(beatInfo.intervalID)
      next()
    }, TOTAL_TIMEOUT)
  }

  connListen(conn, buf => {
    if (buf.toString('utf8') === BEATING_MSG) {
      resetTimeout()
    }
  })

  resetTimeout()

  beatInfo.intervalID = setInterval(() => {
    connSend(conn, Buffer.from(BEATING_MSG))
  }, BEATING_INTERVAL)

  return beatInfo
}

function clearBeat(beatInfo) {
  const { timeoutTimerID, intervalID } = beatInfo

  if (timeoutTimerID) {
    clearTimeout(timeoutTimerID)
  }

  if (intervalID) {
    clearInterval(intervalID)
  }
}

// Manager may not be freed after this call.
export function freeManager(manager) {
  const {
    beatInfo,
    conns,
    masterMux,
    onClose,
  } = manager
  let remainedSockets = conns.length + 1

  const tryTriggerClose = () => {
    remainedSockets -= 1
    if (remainedSockets === 0 && typeof onClose === 'function') {
      onClose()
    }
  }

  clearBeat(beatInfo)

  if (Array.isArray(conns)) {
    conns.forEach(conn => {
      const { mux } = conn
      mux.event.on('close', tryTriggerClose)
      muxClose(mux)
    })
  }

  masterMux.event.on('close', tryTriggerClose)
  muxClose(masterMux)
}

export function initClientMasterSocket(mux) {
  return new Promise(resolve => {
    const msg = Buffer.from(CONVERSATION_START_CHAR)
    let data = Buffer.allocUnsafe(0)

    const conn = createMuxConn(mux)
    initConn(conn, buf => {
      data = Buffer.concat([data, buf])

      const res = checkJSONMsg(data)

      if (res) {
        resolve([conn, res])
      }
    })

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

  ports.forEach(port => {
    const info = {}

    const mux = createMux(
      Object.assign({}, options, {
        port: 0,
        // TODO: check
        passive: false,
        targetPort: port,
        targetAddr: ipAddr,
      }),
    )

    info.mux = mux
    info.socket = mux.sess
    info.targetSocketPort = port
    info.targetSocketAddr = serverAddr

    conns.push(info)
  })

  return conns
}

// TODO: throw when failed to connect
export async function createClient(_options) {
  const options = Object.assign({}, DEFAULT_OPTIONS, _options)
  const { serverAddr, serverPort } = options
  const client = new EventEmitter()

  client.state = 0
  client.ports = []
  client._roundCur = 0

  // Requests the ip from address.
  const ipAddr = await getIP(serverAddr)

  const masterMux = createMux(
    Object.assign({}, _options, {
      // TODO: check
      passive: false,
      targetAddr: ipAddr,
      targetPort: serverPort,
      port: 0,
    }),
  )
  client.masterSocket = masterMux.sess
  client.masterMux = masterMux

  // Client should never get any passive connections.
  // TODO: Remove?
  muxBindConnection(client.masterMux, conn => {
    connSendClose(conn)
  })

  return initClientMasterSocket(masterMux)
    .then(([conn, ports]) => {
      // TODO:
      client.beatInfo = initBeat(conn, () => {
        // console.log('emit_close', i)
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

export function getConnectionPorts(sockets) {
  return sockets.map(i => i.port)
}

function createPassiveSockets(manager, options) {
  const { socketAmount } = options
  const sockets = []

  const handleConn = conn => {
    if (typeof manager.onConnection === 'function') {
      manager.onConnection(conn)
    }
  }

  // create sockets for tunneling
  for (let i = 0; i < socketAmount; i += 1) {
    const info = {}

    const mux = createMux(
      Object.assign({}, options, {
        passive: true,
        port: 0,
      }),
    )
    const port = getPort(mux.sess)

    muxBindConnection(mux, handleConn)

    info.mux = mux
    info.socket = mux.sess
    info.port = port
    sockets.push(info)
  }

  return sockets
}

// Create a "server" that waits for connection and create sockets
// for them to connect.
export async function createManager(_options, onConnection, onClose) {
  const options = Object.assign({}, DEFAULT_OPTIONS, _options)
  const { serverPort } = options
  const manager = {
    conns: null,
    onConnection,
  }

  // 1. Create mux sockets.
  // create master socket
  const masterMux = createMux(
    Object.assign({}, options, {
      // TODO: check
      passive: true,
      port: serverPort,
    }),
  )
  manager.masterSocket = masterMux.sess
  manager.masterMux = masterMux
  manager.onClose = onClose

  // When new connections comes, create some passive sockets
  // that waits for connections.
  muxBindConnection(masterMux, conn => {
    initConn(conn, buf => {
      const shouldReply = checkHandshakeMsg(buf)

      if (!shouldReply) {
        return
      }

      const conns = createPassiveSockets(manager, options)
      manager.conns = conns

      sendJson(conn, getConnectionPorts(conns))

      manager.beatInfo = initBeat(conn, () => {
        freeManager(manager)
      })
    })
  })

  return manager
}

export const sendBuf = connSend

export const listen = connListen

export const close = conn => connClose(conn)

export const bindEnd = bindOthersideEnd

export const sendStop = connSendStop

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

// private
export function setWaitFinTimeout(manager, timeout) {
  const { masterMux, conns } = manager

  _setWaitFinTimeout(masterMux, timeout)
  conns.forEach(muxInfo => {
    _setWaitFinTimeout(muxInfo.mux, timeout)
  })
}

// if (module === require.main) {
//   startUpdaterTimer()
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
