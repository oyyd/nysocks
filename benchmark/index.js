// NOTE: Please run with node with version >= 6.
const net = require('net')
const { createClient, createServerRouter } = require('../node/lib/index')
const socks = require('socksv5-kcpuv')

const BAND_WIDTH = 10 * 1024 * 1024
const CONFIG = {
  serverAddr: '0.0.0.0',
  serverPort: 20000,
  socketAmount: 100,
  password: 'HELLO',
  kcp: {
    sndwnd: 2048,
    rcvwnd: 2048,
    nodelay: 1,
    interval: 10,
    resend: 2,
    nc: 1,
  },
  clientProtocol: 'SOCKS',
  pac: {
    pacServerPort: 8091,
  },
  SOCKS: {
    port: 1081,
  },
}

function createResourceServer() {
  return new Promise(resolve => {
    const server = net.createServer(c => {
      c.on('data', buf => {
        c.write(buf)
      })
    })

    server.listen(() => {
      const addr = server.address()

      resolve({
        addr,
        server,
      })
    })
  })
}

function createNysocksServer(resourcePort) {
  const config = Object.assign({}, CONFIG)

  const server = createServerRouter(config)

  return server
}

function createNysocksClient() {
  return new Promise((resolve) => {
    const client = createClient(Object.assign({}, CONFIG), () => {
      resolve(client)
    })
  })
}

function createAppClient(addr) {
  return new Promise((resolve) => {
    const { address, port } = addr
    socks.connect({
      host: address,
      port,
      proxyHost: '0.0.0.0',
      proxyPort: CONFIG.SOCKS.port,
      auths: [socks.auth.None()],
    }, socket => {
      resolve(socket)
    })
  })
}

function createEnvironment() {
  let addr = null
  let server = null
  let nysocksServer = null
  let nysocksClient = null
  let appSocket = null

  // 1. create resource server
  return (
    createResourceServer()
      .then((res) => {
        // eslint-disable-next-line
        addr = res.addr
        // eslint-disable-next-line
        server = res.server
      })
      // 2. create nysocks server
      .then(() => {
        return createNysocksServer(addr.port)
      })
      // 3. create nysocks client and send msg continually
      // 4. record nysocks server CPU and memory
      .then(_server => {
        console.log('nysocks server created')
        nysocksServer = _server

        return createNysocksClient()
      })
      .then(() => {
        console.log('nysocks client created')
        return createAppClient(addr)
      })
      .then((_appSocket) => {
        appSocket = _appSocket
      })
      .then(() => {
        return {
          addr,
          server,
          nysocksClient,
          nysocksServer,
          appSocket,
        }
      })
  )
}

function main() {
  createEnvironment().then((resources) => {
    const { appSocket } = resources
    const content = Buffer.allocUnsafe(1024 * 1024)

    setInterval(() => {
      appSocket.write(content)
    }, 100)

    // appSocket.on('data', (data) => {
    //   console.log('data', data.length)
    // })
    // console.log('resources', resources)
  }).catch(err => {
    setTimeout(() => {
      throw err
    })
  })
}

main()
