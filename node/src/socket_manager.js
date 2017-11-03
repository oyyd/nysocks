import { createConnection, getPort, create, send, startKcpuv, listen, setAddr } from './socket'

const DEFAULT_OPTIONS = {
  targetAddress: '0.0.0.0',
  targetPort: 20000,
  socketAmount: 100,
}

startKcpuv()

export function tunnel(manager, socket) {
  // const socket = manager.conns[0]
  //
  // return socket
}

export function getPorts(targetAddress, targetPort) {
  return new Promise((resolve) => {
    let data = Buffer.allocUnsafe(0)

    const sess = createConnection(targetAddress, targetPort, (buf) => {
      data = Buffer.concat([data, buf])
    })

    sess.on('close', () => {
      resolve(data)
    })
  })
}

export const CLIENT_STATE = {
  0: 'NOT_CONNECT',
  1: 'CONNECT',
}

export function createClient(_options) {
  const options = Object.assign({}, DEFAULT_OPTIONS, _options)
  const { targetAddress, targetPort } = options
  const client = {
    state: 0,
    ports: [],
  }

  return getPorts(targetAddress, targetPort).then((ports) => {
    client.ports = ports
    client.state = 1
    return client
  })
}

export function createManager(_options) {
  const options = Object.assign({}, DEFAULT_OPTIONS, _options)
  const { socketAmount } = options
  const conns = []

  for (let i = 0; i < socketAmount; i += 1) {
    const socket = create()
    // TODO:
    listen(socket, 0, (buf) => { console.log(buf) })

    const port = getPort(socket)

    const info = {
      socket,
      port,
    }
    conns.push(info)
  }

  return {
    conns,
  }
}

export function getConnectionPorts(manager) {
  return manager.conns.map(i => i.port)
}
