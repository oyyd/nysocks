import os from 'os'
import { TOTAL_TIMEOUT } from './socket_manager'

const INTERVAL_TIME = 5 * 1000

// NOTE: We assume there would be only one valid ipv4 interface.
function getValidIP() {
  const ns = os.networkInterfaces()
  const interfaceNames = Object.keys(ns)

  for (let i = 0; i < interfaceNames.length; i += 1) {
    const name = interfaceNames[i]
    const interfaces = ns[name]

    for (let j = 0; j < interfaces.length; j += 1) {
      const intf = interfaces[j]

      if (!intf.internal && intf.family === 'IPv4') {
        return intf.address
      }
    }
  }

  return null
}

export function createMonitor(next) {
  if (typeof next !== 'function') {
    throw new Error('expect a next "function"')
  }

  const obj = {
    ts: Date.now(),
    ip: getValidIP(),
  }

  obj.id = setInterval(() => {
    const oriIP = obj.ip
    const now = Date.now()
    const ip = getValidIP()

    obj.ip = ip

    // trigger when we could access non-internal network
    if (oriIP !== ip || now - obj.ts >= TOTAL_TIMEOUT) {
      next()
    }

    obj.ts = now
  }, INTERVAL_TIME)

  return obj
}

export function clearMonitor(monitor) {
  clearInterval(monitor.id)
}
