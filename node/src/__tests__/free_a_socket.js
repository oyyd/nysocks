import { startKcpuv, create, bindClose, destroy, close } from '../socket'

function main() {
  startKcpuv()
  const socket = create()

  destroy(socket)
}

main()
