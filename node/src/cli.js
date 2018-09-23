import path from 'path'
import fs from 'fs'
import yargs from 'yargs'
import childProcess from 'child_process'
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

function camelize(key) {
  let nextKey = key
  let index = nextKey.indexOf('_')

  while (index >= 0 && index !== 0 && index !== nextKey.length) {
    nextKey = nextKey.slice(0, index) + nextKey[index + 1].toUpperCase() + nextKey.slice(index + 2)
    index = nextKey.indexOf('_')
  }

  return nextKey
}

function getFile(configFile) {
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

function checkAuthList(config) {
  const { authList } = config.SOCKS
  if (authList !== null && authList !== undefined
      && !(authList instanceof Object && !Array.isArray(authList))) {
    throw new Error('expect auth list to be like {"name_a": "password_a", "name_b": "password_b"}')
  }
}

function checkRequiredConfig(config) {
  if (config.log_memory) {
    logMemory()
  }

  if (config.log_conn) {
    logConnections()
  }
}

function checkClientArgs(config) {
  const { clientProtocol, SS } = config
  const { password } = SS

  if (clientProtocol !== 'SOCKS' && clientProtocol !== 'SS') {
    throw new Error('expect client_protocol to be "SOCKS" or "SS"')
  }

  if (clientProtocol === 'SS' && !password) {
    throw new Error('expect ss_password to be specified')
  }
}

function safelyAssign(obj, key, value) {
  if (value) {
    obj[key] = value
  }
}

function parseConfig(argv) {
  const configJsonFromFile = getFile(formatConfig(argv.config)) || {}
  // const authList = getFile(formatConfig(argv.socks_auth)) || null
  const modeKcpOptions = getMode(argv.mode ? argv.mode : configJsonFromFile.mode)

  let config = {
    pac: DEFAULT_PAC_SERVER,
    SOCKS: DEFAULT_SOCKS_CONFIG,
    SS: {},
    clientProtocol: 'SOCKS',
  }

  config = Object.assign(config, configJsonFromFile)
  config.kcp = config.kcp || {}
  config.kcp = Object.assign(config.kcp, modeKcpOptions)

  Object.keys(argv).forEach(key => {
    if (argv[key]) {
      config[key] = argv[key]
    }
  })

  if (config.log_path) {
    changeLogger(config.log_path)
  }

  Object.keys(config).forEach(key => {
    if (config[key]) {
      config[camelize(key)] = config[key]
    }
  })

  // SOCKS
  // config.SOCKS.authList = authList
  config.SOCKS.authList = null
  safelyAssign(config.SOCKS, 'port', config.socksPort)

  // SS
  safelyAssign(config.SOCKS, 'password', config.ssPassword)
  safelyAssign(config.SOCKS, 'method', config.ssMethod)
  safelyAssign(config.SOCKS, 'serverPort', config.ssPort)

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
    .version()
    .option('config', {
      alias: 'c',
      describe: 'The path of a json file that describe your configuration.',
    })
    .option('daemon', {
      alias: 'd',
      describe: 'Run with a daemon(pm2): start, stop, restart.',
    })
    .option('daemon_status', {
      alias: 's',
      describe: 'Show daemoned(pm2) processes status',
    })
    .option('mode', {
      alias: 'm',
      describe: 'Like kcptun: normal, fast, fast2, fast3.',
    })
    .option('password', {
      alias: 'k',
      describe: 'The passowrd/key for the encryption of transmissio.',
    })
    // TODO:
    .option('socket_amount', {
      describe: 'The amount of connections to be created for each client (default: 10)',
    })
    .option('server_addr', {
      alias: 'a',
      describe: 'The host of your server.',
    })
    .option('server_port', {
      alias: 'p',
      describe: 'The port of your server.',
    })
    .option('client_protocol', {
      alias: 'cp',
      describe: 'The protocol that will be used by clients: SS, SOCKS (default: SOCKS)',
    })
    // SOCKS options
    // .option('socks_auth', {
    //   describe: 'Specify a list of username/password pairs for the socks5 authentication.',
    // })
    .option('socks_port', {
      describe: 'Specify the local port for SOCKS service (default: 1080)',
    })
    // SS options
    .option('ss_port', {
      describe: 'Specify the local port for ssServer service (default: 8083)',
    })
    .option('ss_password', {
      describe: 'Specify the key for the encryption of ss',
    })
    .option('ss_method', {
      describe: 'Specify the method of the encryption for ss (default: aes-128-cfb)',
    })

    .option('log_path', {
      describe: 'The file path for logging. If not set, will log to the console.',
    })
    .option('log_memory', {
      describe: 'Log memory info.',
    })
    .option('log_conn', {
      describe: 'Log connections info.',
    })
    .command({
      command: 'server',
      desc: 'Start a tunnel server.',
      handler: (argv) => {
        const config = parseConfig(argv)

        checkRequiredConfig(config)

        if (config.daemon) {
          runAsDaemon(config, 'server')
          return
        } else if (config.daemon_status) {
          pm.logStatus()
          return
        }

        createServerRouter(config)

        logger.info(`Server is listening on ${config.serverPort}`)
      },
    })
    .command({
      command: 'client',
      desc: 'Start a tunnel client.',
      handler: (argv) => {
        const config = parseConfig(argv)

        checkClientArgs(config)
        checkAuthList(config)
        checkRequiredConfig(config)

        if (config.daemon) {
          runAsDaemon(config, 'client')
          return
        } else if (config.daemon_status) {
          pm.logStatus()
          return
        }

        const pacConfig = Object.assign({ port: config.SOCKS.port }, config.pac)
        createPACServer(pacConfig)
        logger.info(`PAC service is listening on ${pacConfig.pacServerPort}`)

        createClient(config)
      },
    })
    .command({
      command: 'pm',
      desc: 'Alias of PM2(process manager) CLI that you can use to check or operate processes directly.',
      handler: (argv) => {
        const args = argv._.slice(1)

        childProcess.fork(path.resolve(__dirname, '../../node_modules/pm2/bin/pm2'), args, {
          stdio: 'inherit',
        })
      },
    })
    .demandCommand(1, 'You need at least one command before moving on')
    .help()
    // NOTE: We have to access the "argv" property in order to trigger "yargs"
    .argv
}
