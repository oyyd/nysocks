import EventEmitter from 'events'
import binding from '../../build/Release/addon.node'
import { createBaseSuite } from './utils'
// import { record, get } from './monitor'

const suite = createBaseSuite('_sess')
const { wrap } = suite

const DEFAULT_KCP_OPTIONS = {
  sndwnd: 128,
  rcvwnd: 128,
  nodelay: 1,
  interval: 10,
  resend: 2,
  nc: 1,
}

binding.useDefaultLoop(true)
binding.initialize()

// const {
//   create, free,
//   listen, stopListen, initSend, send, bindClose, close,
//   initialize, destruct,
//   startLoop, destroyLoop,
// } = binding

export function create() {
  const sess = binding.create()
  sess.event = new EventEmitter()
  sess._sess = true

  binding.bindClose(sess, errorMsg => sess.event.emit('close', errorMsg))

  return sess
}

export const initCryptor = wrap((sess, key) => {
  if (typeof key !== 'string' || key.length === 0) {
    throw new Error('expect a string "key"')
  }

  binding.initCryptor(sess, key, key.length)
})

export const setWndSize = wrap((sess, sndWns, rcvWnd) => {
  binding.setWndSize(sess, sndWns, rcvWnd)
})

export const setNoDelay = wrap((sess, nodelay, interval, resend, nc) => {
  binding.setNoDelay(sess, nodelay, interval, resend, nc)
})

export function createWithOptions(_options) {
  const sess = create()
  const options = Object.assign({}, DEFAULT_KCP_OPTIONS, _options)
  const {
    sndwnd,
    rcvwnd,
    nodelay,
    interval,
    resend,
    nc,
  } = options

  setWndSize(sess, sndwnd, rcvwnd)
  setNoDelay(sess, nodelay, interval, resend, nc)

  return sess
}

export const destroy = wrap((sess) => {
  binding.free(sess)
})

export const listen = wrap((sess, port = 0, onMessage) => {
  const errMsg = binding.listen(sess, port, onMessage)

  if (errMsg) {
    throw new Error(`${errMsg} (port: ${port})`)
  }
})

export const bindListener = wrap((sess, onMessage) => {
  if (typeof onMessage !== 'function') {
    throw new Error('expect "onMessage" to be a function')
  }

  binding.bindListen(sess, onMessage)
})

export const getPort = wrap((sess) => binding.getPort(sess))

export const stopListen = wrap((sess) => {
  binding.stopListen(sess)
})

export const send = wrap((sess, buf) => {
  binding.send(sess, buf, buf.length)
})

export const close = wrap((sess, sendCloseMsg = false) => {
  binding.close(sess, sendCloseMsg)
})

export const setAddr = wrap((sess, address, port) => {
  binding.initSend(sess, address, port)
})

export function createConnection(targetAddress, targetPort, onMsg) {
  const sess = create()

  listen(sess, 0, onMsg)
  setAddr(sess, targetAddress, targetPort)

  return sess
}

export function startKcpuv() {
  binding.startLoop()
}

// TODO:
export function stopKcpuv() {
  binding.destroyLoop()
  // binding.destruct()
}
