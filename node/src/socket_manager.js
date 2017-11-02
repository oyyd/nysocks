import { create, send, startKcpuv, listen, setAddr } from './socket'

startKcpuv()

export function tunnel(manager, socket) {
  const socket = manager.conns[0]

  return socket
}

export function bind() {

}

function getConnectionPorts() {

}

export function createClient(options) {

}

export function createManager(options) {
  const { targetAddress, targetPort } = options

  // TODO:
  const socket = create()

  setAddr(socket, targetAddress, targetPort)

  const conns = [socket]

  return {
    conns,
  }
}
