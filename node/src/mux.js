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

export const muxBindConnection = muxSuite.wrap((mux, onConnection) => {
  binding.muxBindConnection(mux, onConnection)
})

export const createConn = muxSuite.wrap((mux, _options) => {
  // eslint-disable-next-line
  const options = Object.assign({}, DEFAULT_CONN_OPTIONS, _options)
  const conn = binding.createConn()
  conn._conn = true

  binding.connInit(mux, conn)

  return conn
})

export function connFree(conn) {
  binding.connFree(conn)
}

if (module === require.main) {
  console.log(createMux())
}
