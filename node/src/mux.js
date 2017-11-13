import EventEmitter from 'events'
import binding from '../../build/Release/addon.node'

export function createMux() {
  const mux = binding.createMux()
  mux.event = new EventEmitter()

  return mux
}


if (module === require.main) {
  console.log(createMux())
}
