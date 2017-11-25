import dgram from 'dgram'
import { DEFAULT_SERVER_PORT } from './socket_manager'
import { input } from './socket'

const DEFAULT_OPTIONS = {
  listenPort: DEFAULT_SERVER_PORT,
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

  routerSocket.on('message', (msg, { address, port }) => {
    const key = `${address}:${port}`

    // TODO: drop invalid msg
    if (!managerMaps[key]) {
      const sess = createASess({ address, port })
      managerMaps[key] = sess
    }

    const sess = managerMaps[key]
    input(sess, msg, address, port)
  })

  routerSocket.bind(listenPort)

  return {
    managerMaps,
  }
}
