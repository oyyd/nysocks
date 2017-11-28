import { monitorLogger } from './logger'

const DEFAULT_DATA = {
  sess: 0,
  mux: 0,
  conn: 0,
}

const statistic = Object.assign({}, DEFAULT_DATA)

export function get(key) {
  return statistic[key]
}

export function record(key, value) {
  statistic[key] = value
}

setInterval(() => {
  monitorLogger.info(JSON.stringify(statistic))
}, 5 * 1000)
