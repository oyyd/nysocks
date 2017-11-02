export const logger = {}

const logFunc = (...args) => console.log(...args)

logger.info = logFunc
logger.debug = logFunc
logger.warn = logFunc
logger.error = logFunc
