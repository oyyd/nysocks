import path from 'path'
import winston from 'winston'
import fecha from 'fecha'
import chalk from 'chalk'

const { format } = winston

function chalkLevel(level) {
  if (level === 'info') {
    return chalk.cyan(level)
  } else if (level === 'warn') {
    return chalk.yellow(level)
  } else if (level === 'error') {
    return chalk.red(level)
  }

  return level
}

const nysocksFormat = format.combine(
  format.timestamp(),
  format.printf(info =>
    `${chalkLevel(info.level)} ${chalk.underline(fecha.format(new Date(info.timestamp), 'YYYY-MM-DD HH:mm:ss'))}: ${info.message}`),
)

let _logger = winston.createLogger({
  level: 'info',
  format: nysocksFormat,
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
  'warn',
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

function memoryLogOnce() {
  memoryLogger.info(JSON.stringify(process.memoryUsage()))
}

export function logMemory() {
  setInterval(() => {
    memoryLogOnce()
  }, 60 * 1000)
}
