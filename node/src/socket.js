import EventEmitter from 'events'
import binding from '../../build/Release/addon.node'

binding.useDefaultLoop(true)

// const {
//   create, free,
//   listen, stopListen, initSend, send, bindClose, close,
//   initialize, destruct,
//   startLoop, destroyLoop,
// } = binding

export function create() {
  const sess = binding.create()
  sess.event = new EventEmitter()

  binding.bindClose(sess, (errorMsg) => sess.event.emit('close'))

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

export function getPort(sess) {
  return binding.getPort(sess)
}

export function stopListen(sess) {
  binding.stopListen(sess)
}

export function send(sess, buf) {
  binding.send(sess, buf, buf.length)
}

export function close(sess) {
  binding.close(sess)
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
  binding.initialize()
  binding.startLoop()
}

export function stopKcpuv() {
  binding.destroyLoop()
  binding.destruct()
}
