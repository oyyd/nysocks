import dgram from 'dgram'
import { DEFAULT_SERVER_PORT } from './socket_manager'
// TODO: Hide socket info
import { input, bindUdpSend } from './socket'
import { logger } from './logger'

const DEFAULT_OPTIONS = {
  listenPort: DEFAULT_SERVER_PORT,
}

function createKey(address, port) {
  return `${address}:${port}`
}

export function createRouter(_options, createASess) {
  if (typeof createASess !== 'function') {
    throw new Error('router expect a "createASess" function')
  }

  const options = Object.assign({}, DEFAULT_OPTIONS, _options)
  const { listenPort } = options

  const managerMaps = {}

  const routerSocket = dgram.createSocket({
    type: 'udp4',
    reuseAddr: false,
  })

  // NOTE: For some network environments, thouse udp sockets
  // with different sending and receiving address/port won't
  // work properly. For these environments we make their sending
  // and receiving address/port same by proxy.
  routerSocket.on('message', (msg, { address, port }) => {
    const key = createKey(address, port)

    // TODO: drop invalid msg
    if (!managerMaps[key]) {
      const sess = createASess({ address, port })
      managerMaps[key] = sess

      bindUdpSend(sess, (msgToBeSend, remoteAddr, remotePort) => {
        routerSocket.send(msgToBeSend, remotePort, remoteAddr)
      })
    }

    const sess = managerMaps[key]
    input(sess, msg, address, port)
  })

  routerSocket.bind(listenPort)

  const onServerClose = ({ address, port }) => {
    const key = createKey(address, port)
    delete managerMaps[key]
    logger.info(`close ${key}`)
  }

  return {
    managerMaps,
    onServerClose,
  }
}

// if (module === require.main) {
//   import dgram from 'dgram'
//
//   const socket = dgram.createSocket({
//     type: 'udp4',
//     reuseAddr: false,
//   })
//
//   socket.on('message', (m, rinfo) => {
//     console.log('rinfo', rinfo)
//   })
//
//   socket.listen(20001)
// }
