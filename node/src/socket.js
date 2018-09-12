// TODO: throw when create socket without starting loops
import EventEmitter from 'events'
import assert from 'assert'
import binding from '../../build/Release/addon.node'
import { createBaseSuite } from './utils'
// import { record, get } from './monitor'

const suite = createBaseSuite('_sess')
const { wrap } = suite

// TODO: Export this from cpp.
const SESS_STATE = {
  KCPUV_STATE_CREATED: 0,
  KCPUV_STATE_READY: 30,
  KCPUV_STATE_FIN: 31,
  KCPUV_STATE_FIN_ACK: 32,
  KCPUV_STATE_WAIT_FREE: 50,
}

const DEFAULT_KCP_OPTIONS = {
  // TODO:
  sndwnd: 2048,
  rcvwnd: 2048,
  nodelay: 1,
  interval: 10,
  resend: 2,
  nc: 1,
}

function init() {
  binding.initialize()
  binding.useDefaultLoop(true)
}

init()

// TODO: Change name to 'free'
export const destroy = wrap(sess => {
  if (sess.isClosed) {
    return
  }

  sess.isClosed = true

  binding.free(sess)
})

export function create(passive) {
  assert(typeof passive === 'boolean', 'expect a boolean `passive`')
  // eslint-disable-next-line
  const sess = new binding.create(passive ? 1 : 0)
  sess.event = new EventEmitter()
  sess._sess = true
  sess.isClosed = false

  binding.bindClose(sess, errorMsg => {
    destroy(sess)
    sess.event.emit('close', errorMsg)
  })

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

// Accept kcp options.
export function createWithOptions(passive, _options) {
  const sess = create(passive)
  const options = Object.assign({}, DEFAULT_KCP_OPTIONS, _options)
  const { sndwnd, rcvwnd, nodelay, interval, resend, nc } = options

  setWndSize(sess, sndwnd, rcvwnd)
  setNoDelay(sess, nodelay, interval, resend, nc)

  return sess
}

// NOTE: Do not use this for the transforming of proxy data.
export const input = wrap((sess, buffer, address, port) =>
  binding.input(sess, buffer, buffer.length, address, port),
)

// NOTE: Do not use this for the transforming of proxy data.
export const bindUdpSend = wrap((sess, next) => {
  binding.bindUdpSend(sess, next)
})

// export const forceClose = wrap((sess) => {
//   if (!sess.isClosed) {
//     sess.isClosed = true
//     destroy(sess)
//     // TODO: Different from common close procedure.
//     // TODO: error msg
//     sess.event.emit('close', null)
//   }
// })

// TODO: Move this to cpp?
export const close = wrap(sess => {
  if (sess.isClosed) {
    return
  }

  binding.close(sess)
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

export const getPort = wrap(sess => binding.getPort(sess))

export const stopListen = wrap(sess => {
  binding.stopListen(sess)
})

export const send = wrap((sess, buf) => {
  binding.send(sess, buf, buf.length)
})

export const setAddr = wrap((sess, address, port) => {
  binding.initSend(sess, address, port)
})

export const getSessState = wrap(sess => {
  const state = binding.getSessState(sess)
  let name = '(invalid state)'

  Object.keys(SESS_STATE).forEach(n => {
    if (SESS_STATE[n] === state) {
      name = n
    }
  })

  return name
})

export const setSessWaitFinTimeout = wrap((sess, timeout) => {
  binding.setWaitFinTimeout(sess, timeout)
})

export function startUpdaterTimer() {
  binding.startLoop()
}

export function stopUpdaterTimer() {
  binding.stopUpdaterTimer()
}
