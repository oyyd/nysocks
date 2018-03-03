import net from 'net'

const PORT = 8123

const server = new net.Server((conn) => {
  const content = Buffer.alloc(10 * 1024 * 1024)
  conn.write(content)
  // setInterval(() => {
  //   conn.write(Buffer.alloc(10))
  // }, 100)
})

server.listen(PORT)

const socket = new net.Socket()

socket.connect(PORT, '127.0.0.1', () => {
})

setTimeout(() => {
  socket.destroy()
}, 500)
