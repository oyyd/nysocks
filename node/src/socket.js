import EventEmitter from 'events'
import binding from '../../build/Release/addon.node'

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

  binding.bindClose(sess, errorMsg => sess.event.emit('close', errorMsg))

  return sess
}

export function destroy(sess) {
  binding.free(sess)
}

export function listen(sess, port = 0, onMessage) {
  const errMsg = binding.listen(sess, port, onMessage)

  if (errMsg) {
    throw new Error(errMsg)
  }
}

export function bindListener(sess, onMessage) {
  binding.bindListen(sess, onMessage)
}

export function getPort(sess) {
  return binding.getPort(sess)
}

export function stopListen(sess) {
  binding.stopListen(sess)
}

export function send(sess, buf) {
  binding.send(sess, buf, buf.length)
}

export function close(sess, sendCloseMsg = false) {
  binding.close(sess, sendCloseMsg)
}

export function setAddr(sess, address, port) {
  binding.initSend(sess, address, port)
}

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
