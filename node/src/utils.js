import dns from 'dns'
import ip from 'ip'

export const debug = false

if (debug) {
  // eslint-disable-next-line
  const SegfaultHandler = require('segfault-handler')
  SegfaultHandler.registerHandler('crash.logfile')
}

function checkValidSocket(name, obj) {
  if (typeof obj !== 'object' || !obj[name]) {
    throw new Error(`try to manipulate invalid ${name}`)
  }
}

function wrap(name, func) {
  return (obj, ...args) => {
    checkValidSocket(name, obj)
    return func(obj, ...args)
  }
}

export function createBaseSuite(name) {
  if (typeof name !== 'string') {
    throw new Error(`invalid base name: ${name}`)
  }

  const suite = {
    name,
    wrap: wrap.bind(null, name),
    checkValidSocket: checkValidSocket.bind(null, name),
  }

  return suite
}

const IP_CACHE = {}

export function getIP(address) {
  if (ip.isV4Format(address) || ip.isV6Format(address)) {
    return Promise.resolve(address)
  }

  if (IP_CACHE[address]) {
    return Promise.resolve(IP_CACHE[address])
  }

  return new Promise((resolve, reject) => {
    dns.lookup(address, (err, ipAddr) => {
      if (err) {
        reject(err)
        return
      }

      IP_CACHE[address] = ipAddr

      resolve(IP_CACHE[address])
    })
  })
}
