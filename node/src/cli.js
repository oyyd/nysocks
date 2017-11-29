import path from 'path'
import fs from 'fs'
import yargs from 'yargs'
import { createPACServer } from 'pac-server'
import { createClient, createServerRouter } from './index'
import { MODES } from './modes'
import { changeLogger } from './logger'

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

function parseConfig(argv) {
  const configJsonFromFile = getConfigFile(formatConfig(argv.config))
  const modeKcpOptions = getMode(argv.mode ? argv.mode : configJsonFromFile.mode)
  let config = {}
  config.kcp = {}
  config.kcp = Object.assign(config.kcp, modeKcpOptions)
  config = Object.assign(config, configJsonFromFile, argv)

  if (config.log_path) {
    changeLogger(config.log_path)
  }

  return config
}

export default function main() {
  // eslint-disable-next-line
  yargs
    .detectLocale(false)
    .option('mode', {
      alias: 'm',
      describe: 'Like kcptun: normal, fast, fast2, fast3',
    })
    .option('config', {
      alias: 'c',
      describe: 'The path of a json file that describe your configuration',
    })
    .options('log_path', {
      describe: 'The file path for logging. If not set, will log to the console.',
    })
    .command({
      command: 'server',
      desc: 'Start a tunnel server.',
      handler: (argv) => createServerRouter(parseConfig(argv)),
    })
    .command({
      command: 'client',
      desc: 'Start a tunnel client.',
      handler: (argv) => {
        const config = parseConfig(argv)
        createPACServer(Object.assign({ port: config.SOCKS.port }, config.pac))
        createClient(config)
      },
    })
    .demandCommand(1, 'You need at least one command before moving on')
    .help()
    // NOTE: We have to access the "argv" property in order to trigger "yargs"
    .argv
}
