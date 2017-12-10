import EventEmitter from 'events'
import binding from '../../build/Release/addon.node'
import { createWithOptions, initCryptor,
  listen as socketListen, setAddr } from './socket'
import { createBaseSuite } from './utils'
import { record, get } from './monitor'
import { logger } from './logger'

const muxSuite = createBaseSuite('_mux')
const connSuite = createBaseSuite('_conn')

const DEFAULT_MUX_OPTIONS = {
  sess: null,
  event: null,
  // bind to a random port by default
  port: 0,
  password: null,
  targetPort: null,
  targetAddr: null,
  kcp: null,
}

const DEFAULT_CONN_OPTIONS = {
  mux: null,
}

function isValidProperty(value) {
  if (typeof value === 'number') {
    return true
  }

  return !!value
}

export function createMux(_options) {
  const options = Object.assign({}, DEFAULT_MUX_OPTIONS, _options)

  // create default sess
  const hasSess = options.sess
  let sess = null

  if (hasSess) {
    // eslint-disable-next-line
    sess = options.sess
  } else {
    const {
      port, password, targetPort, targetAddr,
    } = options

    if (!isValidProperty(port) || !isValidProperty(password)) {
      throw new Error('invalid mux options')
    }
    sess = createWithOptions(options.kcp)
    initCryptor(sess, password)
    socketListen(sess, port)
    if (isValidProperty(targetPort) && isValidProperty(targetAddr)) {
      setAddr(sess, targetAddr, targetPort)
    }
  }

  const mux = binding.createMux()
  mux.event = new EventEmitter()
  mux.sess = sess
  mux._mux = true
  mux.isClosed = false

  binding.muxInit(mux, sess)

  record('mux', get('mux') + 1)

  return mux
}

export const muxFree = muxSuite.wrap((mux, onFree) => {
  if (mux.isClosed) {
    return
  }

  mux.isClosed = true

  process.nextTick(() => {
    binding.muxFree(mux)
    record('mux', get('mux') - 1)
    if (typeof onFree === 'function') {
      onFree()
    }
  })
})

export const muxBindClose = muxSuite.wrap((mux, onClose) => {
  binding.muxBindClose(mux, onClose)
})

let id = 0

export function wrapMuxConn(conn) {
  // eslint-disable-next-line
  conn.id = id++

  conn._conn = true
  conn.isClosed = false
  record('conn', get('conn') + 1)
}

// NOTE: user should bind listen synchronously
// for the comming msg
export const muxBindConnection = muxSuite.wrap((mux, onConnection) => {
  if (typeof onConnection !== 'function') {
    throw new Error('muxBindConnection expect an "onConnection" function')
  }

  binding.muxBindConnection(mux, (conn) => {
    wrapMuxConn(conn)
    onConnection(conn)
  })
})

export const createMuxConn = muxSuite.wrap((mux, _options) => {
  // eslint-disable-next-line
  const options = Object.assign({}, DEFAULT_CONN_OPTIONS, _options)
  const conn = binding.createMuxConn()
  wrapMuxConn(conn)

  binding.connInit(mux, conn)

  return conn
})

export const connFree = connSuite.wrap((conn) => {
  if (conn.isClosed) {
    return
  }

  conn.isClosed = true

  process.nextTick(() => {
    binding.connFree(conn)
    record('conn', get('conn') - 1)
  })
})

export const connSend = connSuite.wrap((conn, buffer) => {
  if (conn.isClosed) {
    logger.warn('"send" after closing')
    return
  }

  binding.connSend(conn, buffer, buffer.length)
})

export const connSendClose = connSuite.wrap((conn) => {
  if (conn.isClosed) {
    return
  }

  binding.connSendClose(conn)
})

export const connListen = connSuite.wrap((conn, onMessage) => {
  binding.connListen(conn, onMessage)
})

export const connBindClose = connSuite.wrap((conn, onClose) => {
  binding.connBindClose(conn, onClose)
})

export const connSetTimeout = connSuite.wrap((conn, timeout) => {
  if (typeof timeout !== 'number' && timeout > 0) {
    throw new Error('invalid timeout')
  }
  binding.connSetTimeout(conn, timeout)
})

export const connEmitClose = connSuite.wrap((conn) => {
  binding.connEmitClose(conn)
})

// if (module === require.main) {
//   (() => {
//     const {
//       startKcpuv, listen, getPort, setAddr,
//     } = require('./socket')
//
//     startKcpuv()
//
//     const addr = '0.0.0.0'
//     const sess1 = createSess()
//     const sess2 = createSess()
//     listen(sess1, 0)
//     listen(sess2, 0)
//     const port1 = getPort(sess1)
//     const port2 = getPort(sess2)
//
//     setAddr(sess1, addr, port2)
//     setAddr(sess2, addr, port1)
//
//     const mux1 = createMux({
//       sess: sess1,
//     })
//
//     const mux2 = createMux({
//       sess: sess2,
//     })
//
//     muxBindConnection(mux2, (conn) => {
//       wrapMuxConn(conn)
//       connListen(conn, (data) => {
//         console.log('mux2_conn_msg: ', data.toString('utf8'))
//       })
//     })
//
//     const conn1 = createMuxConn(mux1)
//     const buffer = Buffer.from('hello')
//     connSend(conn1, buffer)
//   })()
// }
