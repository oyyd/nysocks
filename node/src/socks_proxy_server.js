/**
 * 1. listen
 * 2. redirect to another local port
 */
import net from 'net'
import socks from 'socksv5-kcpuv'
import { logger } from './logger'

const DEFAULT_OPTIONS = {
  address: '127.0.0.1',
  port: 1080,
  targetAddress: '127.0.0.1',
  targetPort: 8022,
}

export function parseDstInfo(data, offset = 0) {
  const atyp = data[offset]

  let dstAddr
  let dstPort
  let dstAddrLength
  let dstPortIndex
  let dstPortEnd
  // length of non-data field
  let totalLength

  switch (atyp) {
    case 0x01:
      dstAddrLength = 4
      dstAddr = data.slice(offset + 1, offset + 5)
      dstPort = data.slice(offset + 5, offset + 7)
      totalLength = offset + 7
      break
    case 0x04:
      dstAddrLength = 16
      dstAddr = data.slice(offset + 1, offset + 17)
      dstPort = data.slice(offset + 17, offset + 19)
      totalLength = offset + 19
      break
    case 0x03:
      dstAddrLength = data[offset + 1]
      dstPortIndex = 2 + offset + dstAddrLength
      dstAddr = data.slice(offset + 2, dstPortIndex)
      dstPortEnd = dstPortIndex + 2
      dstPort = data.slice(dstPortIndex, dstPortEnd)
      totalLength = dstPortEnd
      break
    default:
      return null
  }

  if (data.length < totalLength) {
    return null
  }

  return {
    atyp,
    dstAddrLength,
    dstAddr,
    dstPort,
    totalLength,
  }
}

export function createProxyConnection(options, buffer) {
  const socket = net.createConnection(options)

  if (buffer) {
    socket.write(buffer)
  }

  return socket
}

export function createServer(_options, next) {
  if (typeof next !== 'function') {
    throw new Error('socks_proxy_server expect a callback function')
  }

  const { port, address } = Object.assign({}, DEFAULT_OPTIONS, _options)

  // init socks server
  const server = socks.createServer((info, accept/* , deny */) => {
    // TODO: socksv5 only support 'BIND' method
    const socket = accept(true)
    next(info, socket)
  })

  // TODO: handle port occupied situation
  server.listen(port, address, () => {
    logger.info('connecting success')
  })

  server.useAuth(socks.auth.None())

  return server
}

// if (module === require.main) {
//   createServer({}, (info, socket) => {
//     const dstInfo = parseDstInfo(info.chunk)
//     const targetSocket = createProxyConnection({
//       host: info.dstAddr,
//       port: info.dstPort,
//     })
//
//     // targetSocket.on('close', (hadError) => {
//     //   if (hadError) {
//     //     console.log('target_socket_close_with_error')
//     //   }
//     // })
//     targetSocket.on('error', err => console.log('target_socket', err))
//     socket.on('error', err => console.log('socket_err', err))
//
//     targetSocket.on('data', buf => {
//       let data = buf
//
//       socket.write(buf)
//     })
//
//     socket.on('data', buffer => {
//       targetSocket.write(buffer)
//     })
//   })
// }
