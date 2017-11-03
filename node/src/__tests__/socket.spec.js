import * as binding from '../socket'

function main() {
  binding.startKcpuv()

  const sess = binding.create()

  binding.listen(sess, 0, () => {})

  setTimeout(() => {
    console.log('port', binding.getPort(sess))
  }, 1000)
}
if (module === require.main) {
  main()
}
