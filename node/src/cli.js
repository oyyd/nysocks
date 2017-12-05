import path from 'path'
import fs from 'fs'
import yargs from 'yargs'
import { createPACServer } from 'pac-server'
import { createClient, createServerRouter } from './index'
import { MODES } from './modes'
import { logger, changeLogger, logMemory } from './logger'
import { logConnections } from './monitor'
import * as pm from './pm'

const DEFAULT_SOCKS_CONFIG = {
  port: 1080,
}

const DEFAULT_PAC_SERVER = {
  pacServerPort: 8090,
}

function getConfigFile(configFile) {
  if (!configFile) {
    return null
  }

  return JSON.parse(fs.readFileSync(configFile, {
    encoding: 'utf8',
  }))
}

function formatConfig(configFile) {
  if (!configFile || path.isAbsolute(configFile)) {
    return configFile
  }

  return path.resolve(process.cwd(), configFile)
}

function getMode(mode) {
  if (!mode) {
    return null
  }

  if (!Object.hasOwnProperty.call(MODES, mode)) {
    throw new Error(`unexpected "mode": ${mode}`)
  }

  return MODES[mode]
}

function checkRequiredConfig(config) {
  if (!config.password) {
    throw new Error('you have to specify a valid "password"')
  }

  if (config.log_memory) {
    logMemory()
  }

  if (config.log_conn) {
    logConnections()
  }
}

function parseConfig(argv) {
  const configJsonFromFile = getConfigFile(formatConfig(argv.config)) || {}
  const modeKcpOptions = getMode(argv.mode ? argv.mode : configJsonFromFile.mode)
  let config = {
    pac: DEFAULT_PAC_SERVER,
    SOCKS: DEFAULT_SOCKS_CONFIG,
  }
  config.kcp = {}
  config.kcp = Object.assign(config.kcp, modeKcpOptions)
  config = Object.assign(config, configJsonFromFile, argv)

  if (config.log_path) {
    changeLogger(config.log_path)
  }

  return config
}

function runAsDaemon(config, type) {
  const { daemon } = config

  if (!Object.hasOwnProperty.apply(pm, [daemon])) {
    throw new Error('invalid daemon options')
  }

  const argv = String(process.argv.slice(2).join(' ')).replace(/-d\s+[^\s]+/, '')

  pm[daemon](argv, type)
}

export default function main() {
  // eslint-disable-next-line
  yargs
    .detectLocale(false)
    .option('daemon', {
      alias: 'd',
      describe: 'Run with a daemon(pm2): start, stop, restart',
    })
    .option('mode', {
      alias: 'm',
      describe: 'Like kcptun: normal, fast, fast2, fast3',
    })
    .option('config', {
      alias: 'c',
      describe: 'The path of a json file that describe your configuration',
    })
    .options('log_path', {
      describe: 'The file path for logging. If not set, will log to the console',
    })
    .options('log_memory', {
      describe: 'Log memory info',
    })
    .options('log_conn', {
      describe: 'Log connections info',
    })
    .command({
      command: 'server',
      desc: 'Start a tunnel server.',
      handler: (argv) => {
        const config = parseConfig(argv)

        if (config.daemon) {
          runAsDaemon(config, 'server')
          return
        }

        checkRequiredConfig(config)
        createServerRouter(config)

        logger.info(`Server is listening on ${config.serverPort}`)
      },
    })
    .command({
      command: 'client',
      desc: 'Start a tunnel client.',
      handler: (argv) => {
        const config = parseConfig(argv)
        if (config.daemon) {
          runAsDaemon(config, 'client')
          return
        }

        checkRequiredConfig(config)
        const pacConfig = Object.assign({ port: config.SOCKS.port }, config.pac)
        createPACServer(pacConfig)
        createClient(config)

        logger.info(`PAC service is listening on ${pacConfig.pacServerPort}`)
      },
    })
    .demandCommand(1, 'You need at least one command before moving on')
    .help()
    // NOTE: We have to access the "argv" property in order to trigger "yargs"
    .argv
}
