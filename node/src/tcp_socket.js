import net from 'net'

export function connect(host, port) {
  return net.createConnection(port, host)
}
