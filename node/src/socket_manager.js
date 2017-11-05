import { getPort, create, send, close,
  startKcpuv, listen, setAddr } from './socket'

const DEFAULT_OPTIONS = {
  targetAddress: '0.0.0.0',
  targetPort: 20000,
  // socketAmount: 100,
  socketAmount: 2,
}

const CONVERSATION_START_CHAR = '\\\\start'
const CONVERSATION_END_CHAR = '\\\\end'

let kcpuvStarted = false

function sendJson(sess, jsonMsg) {
  const content = Buffer.from(`${JSON.stringify(jsonMsg)}${CONVERSATION_END_CHAR}`)
  send(sess, content, content.length)
}

export function tunnel(manager, socket) {
  // const socket = manager.conns[0]
  //
  // return socket
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

export function initClientConns(options, client) {
  const { targetAddress } = options
  const { ports } = client
  const conns = []

  ports.forEach((port) => {
    const socket = create()
    setAddr(socket, targetAddress, port)

    conns.push({
      socket,
      targetSocketPort: port,
      targetSocketAddr: targetAddress,
    })
  })

  return Object.assign({}, client, {
    conns,
  })
}

export function createClient(_options) {
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
    .then(c => initClientConns(options, c))
}

export function closeClient(client) {
  const { masterSocket, conns } = client

  conns.forEach(conn => close(conn))
  close(masterSocket)
}

export function createManager(_options) {
  if (!kcpuvStarted) {
    kcpuvStarted = true
    startKcpuv()
  }

  const options = Object.assign({}, DEFAULT_OPTIONS, _options)
  const { socketAmount, targetPort } = options
  const conns = []

  // create master socket
  const masterSocket = create()

  // create sockets for tunneling
  for (let i = 0; i < socketAmount; i += 1) {
    const socket = create()
    listen(socket, 0, (buf) => {
      // ...
    })

    const port = getPort(socket)
    const info = {
      socket,
      port,
    }
    conns.push(info)
  }

  listen(masterSocket, targetPort, (buf) => {
    const shouldReply = checkHandshakeMsg(buf)

    if (shouldReply) {
      // TODO:
      sendJson(masterSocket, conns.map(i => i.port))
    }
  })

  return {
    masterSocket,
    conns,
  }
}

export function getConnectionPorts(manager) {
  return manager.conns.map(i => i.port)
}

if (module === require.main) {
  startKcpuv()
  const socket = create()
  close(socket)

  // const manager = createManager()
  //
  // createClient().then(client => {
  //   console.log('client', client)
  //
  //   setTimeout(() => {
  //     closeClient(client)
  //   }, 1000)
  // })
}
