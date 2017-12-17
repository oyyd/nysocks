import { createServer as _createServer } from 'encryptsocks'
import { logger } from './logger'

export function createServer(config, onConnection) {
  if (typeof onConnection !== 'function') {
    throw new Error('expect a function "onConnection"')
  }

  // NOTE: force false
  config.udpActive = false
  // Called when ssServers ask for proxy connections
  config.connect = onConnection

  return _createServer(config, false, logger)
}

// if (module === require.main) {
//   createServer()
// }
