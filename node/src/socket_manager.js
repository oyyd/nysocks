import { getPort, create, send, close,
  startKcpuv, listen, setAddr } from './socket'

const DEFAULT_OPTIONS = {
  targetAddress: '0.0.0.0',
  targetPort: 20000,
  // socketAmount: 100,
  socketAmount: 1,
}

const CONVERSATION_START_CHAR = '\\\\start'
const CONVERSATION_END_CHAR = '\\\\end'

let kcpuvStarted = false

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

export function initClientSocket(sess, targetAddress, targetPort) {
  return new Promise((resolve) => {
    const msg = Buffer.from(CONVERSATION_START_CHAR)
    let data = Buffer.allocUnsafe(0)

    setAddr(sess, targetAddress, targetPort)
    listen(sess, 0, (buf) => {
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

export function initClientConns(options, client, next) {
  const { targetAddress } = options
  const { ports } = client
  const conns = []

  ports.forEach((port) => {
    const info = {}
    const socket = create()

    listen(socket, 0, (buf) => next(info, buf))
    setAddr(socket, targetAddress, port)

    info.socket = socket
    info.targetSocketPort = port
    info.targetSocketAddr = targetAddress
    info.locked = false

    conns.push(info)
  })

  return Object.assign({}, client, {
    conns,
  })
}

export function createClient(_options, next) {
  start()

  const options = Object.assign({}, DEFAULT_OPTIONS, _options)
  const { targetAddress, targetPort } = options
  const client = {
    state: 0,
    ports: [],
  }

  const masterSocket = create()

  client.masterSocket = masterSocket

  return initClientSocket(masterSocket, targetAddress, targetPort)
    .then((ports) => {
      client.ports = ports
      client.state = 1
      return client
    })
    .then(c => initClientConns(options, c, next))
}

export function closeClient(client) {
  const { masterSocket, conns } = client

  conns.forEach(conn => close(conn.socket, true))
  close(masterSocket)
}

export function getConnectionPorts(manager) {
  return manager.conns.map(i => i.port)
}

export function createManager(_options, next) {
  start()

  const options = Object.assign({}, DEFAULT_OPTIONS, _options)
  const { socketAmount, targetPort } = options
  const conns = []
  const manager = {
    conns,
  }

  // create master socket
  const masterSocket = create()
  manager.masterSocket = masterSocket

  // create sockets for tunneling
  for (let i = 0; i < socketAmount; i += 1) {
    const info = {}
    const socket = create()
    listen(socket, 0, (buf) => next(info, buf))

    const port = getPort(socket)
    info.socket = socket
    info.port = port
    conns.push(info)
  }

  listen(masterSocket, targetPort, (buf) => {
    const shouldReply = checkHandshakeMsg(buf)

    if (shouldReply) {
      // TODO:
      sendJson(masterSocket, getConnectionPorts(manager))
    }
  })

  return manager
}

export function sendBuf(socketInfo, buf) {
  send(socketInfo.socket, buf, buf.length)
}

export function lockOne(client) {
  const { conns } = client

  for (let i = 0; i < conns.length; i += 1) {
    const conn = conns[i]

    if (!conn.locked) {
      conn.locked = true
      return conn
    }
  }

  throw new Error('no availiable socket')
}

export function releaseOne(client, info) {
  const { conns } = client

  for (let i = 0; i < conns.length; i += 1) {
    if (conns[i] === info) {
      if (!conns[i].locked) {
        throw new Error('release an unlocked socket')
      } else {
        conns[i].locked = false
        return
      }
    }
  }

  throw new Error('release an unknown socket')
}

if (module === require.main) {
  const manager = createManager(null, (info, buf) => {
    console.log('manager received:', buf.toString('utf8'))
    sendBuf(info, Buffer.from('I have received'))
  })

  createClient(null, (info, buf) => {
    console.log('client:', buf.toString('utf8'))
  }).then(client => {
    sendBuf(client.conns[0], Buffer.from('Hello'))
  }).catch(err => {
    console.error(err)
  })
}
