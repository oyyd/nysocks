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

  binding.bindClose(sess, () => sess.event.emit('close'))

  return sess
}

export function destroy(sess) {
  binding.free(sess)
}

export function listen(sess, port = 0, onMessage) {
  binding.listen(sess, port, onMessage)
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

export function startKcpuv() {
  binding.initialize()
  binding.startLoop()
}

export function stopKcpuv() {
  binding.destroyLoop()
  binding.destruct()
}
