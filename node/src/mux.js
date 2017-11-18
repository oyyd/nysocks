import EventEmitter from 'events'
import binding from '../../build/Release/addon.node'
import { create as createSess } from './socket'
import { createBaseSuite } from './utils'

const muxSuite = createBaseSuite('_mux')
const connSuite = createBaseSuite('_conn')

const DEFAULT_MUX_OPTIONS = {
  sess: null,
  event: null,
}

const DEFAULT_CONN_OPTIONS = {
  mux: null,
}

export function createMux(_options) {
  const options = Object.assign({}, DEFAULT_MUX_OPTIONS, _options)

  // create default sess
  const sess = options.sess ? options.sess : createSess()

  const mux = binding.createMux()
  mux.event = new EventEmitter()
  mux.sess = sess
  mux._mux = true

  binding.muxInit(mux, sess)

  return mux
}

export const muxFree = muxSuite.wrap((mux) => {
  binding.muxFree(mux)
})

export const muxBindClose = muxSuite.wrap((mux, onClose) => {
  binding.muxBindClose(mux, onClose)
})

// NOTE: user should bind listen synchronously
// for the comming msg
export const muxBindConnection = muxSuite.wrap((mux, onConnection) => {
  binding.muxBindConnection(mux, onConnection)
})

export const createMuxConn = muxSuite.wrap((mux, _options) => {
  // eslint-disable-next-line
  const options = Object.assign({}, DEFAULT_CONN_OPTIONS, _options)
  const conn = binding.createMuxConn()
  conn._conn = true

  binding.connInit(mux, conn)

  return conn
})

export function wrapMuxConn(conn) {
  // eslint-disable-next-line
  conn._conn = true

  return conn
}

export const connFree = connSuite.wrap((conn) => {
  binding.connFree(conn)
})

export const connSend = connSuite.wrap((conn, buffer) => {
  binding.connSend(conn, buffer, buffer.length)
})

export const connSendClose = connSuite.wrap((conn) => {
  binding.connSendClose(conn)
})

export const connListen = connSuite.wrap((conn, onMessage) => {
  binding.connListen(conn, onMessage)
})

export const connBindClose = connSuite.wrap((conn, onClose) => {
  binding.connBindClose(conn, onClose)
})

if (module === require.main) {
  (() => {
    const {
      startKcpuv, listen, getPort, setAddr,
    } = require('./socket')

    startKcpuv()

    const addr = '0.0.0.0'
    const sess1 = createSess()
    const sess2 = createSess()
    listen(sess1, 0)
    listen(sess2, 0)
    const port1 = getPort(sess1)
    const port2 = getPort(sess2)

    setAddr(sess1, addr, port2)
    setAddr(sess2, addr, port1)

    const mux1 = createMux({
      sess: sess1,
    })

    const mux2 = createMux({
      sess: sess2,
    })

    muxBindConnection(mux2, (conn) => {
      wrapMuxConn(conn)
      connListen(conn, (data) => {
        console.log('mux2_conn_msg: ', data.toString('utf8'))
      })
    })

    const conn1 = createMuxConn(mux1)
    const buffer = Buffer.from('hello')
    connSend(conn1, buffer)
  })()
}
