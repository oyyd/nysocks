import path from 'path'
import winston from 'winston'
import fecha from 'fecha'

const { format } = winston
const kcpuvFormat = format.combine(
  format.timestamp(),
  format.printf(info =>
    `${info.level} ${fecha.format(new Date(info.timestamp), 'YYYY-MM-DD HH:mm:ss')}: ${info.message}`),
)

let _logger = winston.createLogger({
  level: 'info',
  format: kcpuvFormat,
  transports: [
    new winston.transports.Console(),
  ],
})

export const memoryLogger = winston.createLogger({
  level: 'info',
  format: format.simple(),
  transports: [
    new winston.transports.File({
      filename: path.resolve(process.cwd(), './memory.log'),
    }),
  ],
})

export const monitorLogger = winston.createLogger({
  level: 'info',
  format: format.simple(),
  transports: [
    new winston.transports.File({
      filename: path.resolve(process.cwd(), './monitor.log'),
    }),
  ],
})

export const logger = {};

([
  'debug',
  'info',
  'error',
  'warning',
]).forEach(level => {
  Object.defineProperty(logger, level, {
    get: () => _logger[level].bind(_logger),
  })
})

export function changeLogger(logPath) {
  _logger = winston.createLogger({
    level: 'info',
    format: format.simple(),
    transports: [
      new winston.transports.File({
        filename: path.resolve(process.cwd(), logPath),
      }),
    ],
  })
}

function logMemory() {
  memoryLogger.info(JSON.stringify(process.memoryUsage()))
}

setInterval(() => {
  logMemory()
}, 60 * 1000)
