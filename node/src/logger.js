import winston from 'winston'
import fecha from 'fecha'

const { format } = winston

export const logger = winston.createLogger({
  level: 'info',
  format: format.combine(
    format.timestamp(),
    format.printf(info =>
      `${info.level} ${fecha.format(new Date(info.timestamp), 'YYYY-MM-DD HH:mm:ss')}: ${info.message}`),
  ),
  transports: [
    new winston.transports.Console(),
  ],
})
