/**
 * 1. listen
 * 2. redirect to another local port
 */
import net from 'net'
import socks from 'socksv5'
import { logger } from './logger'
import { createManager } from './socket_manager'

const DEFAULT_OPTIONS = {
  address: '127.0.0.1',
  port: 1081,
  targetAddress: '127.0.0.1',
  targetPort: 8022,
}

export function createProxyConnection(host, port, buffer) {
  const socket = net.createConnection({
    port,
    host,
  })

  if (buffer) {
    socket.write(buffer)
  }

  return socket
}

export function createServer(_options, _next) {
  const { port, address } = Object.assign({}, DEFAULT_OPTIONS, _options)

  // create kcpuv socket manager
  const manager = createManager()

  // init socks server
  const server = socks.createServer((info, accept, deny) => {
    // info structure
    // { cmd: 'connect',
    // srcAddr: '127.0.0.1',
    // srcPort: 64162,
    // dstAddr: 'clients4.google.com',
    // dstPort: 443 }

    // TODO: socksv5 only support 'BIND' method
    const socket = accept(true)
    // TODO: pipe

    if (typeof _next === 'function') {
      _next(info, socket)
    }
  })

  // TODO: handle port occupied situation
  server.listen(port, address, () => {
    logger.info('listen success')
  })

  server.useAuth(socks.auth.None())

  return server
}

if (module === require.main) {
  createServer({}, (info, socket) => {
    const targetSocket = createProxyConnection(info.dstAddr, info.dstPort)

    // targetSocket.on('close', (hadError) => {
    //   if (hadError) {
    //     console.log('target_socket_close_with_error')
    //   }
    // })
    targetSocket.on('error', (err) => console.log('target_socket', err))
    socket.on('error', (err) => console.log('socket_err', err))

    targetSocket.on('data', (buf) => {
      socket.write(buf)
    })

    socket.on('data', (buffer) => {
      targetSocket.write(buffer)
    })
  })
}
