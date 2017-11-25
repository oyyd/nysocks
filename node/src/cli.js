import path from 'path'
import fs from 'fs'
import yargs from 'yargs'
import { createPACServer } from 'pac-server'
import { createClient, createServerRouter } from './index'

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

function parseConfig(argv) {
  const configJson = getConfigFile(formatConfig(argv.config))

  return Object.assign({}, configJson, argv)
}

export default function main() {
  // eslint-disable-next-line
  yargs
    .detectLocale(false)
    .option('config', {
      alias: 'c',
      describe: 'The path of a json file that describe your configuration',
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
        createPACServer(config.pac)
        createClient(config)
      },
    })
    .demandCommand(1, 'You need at least one command before moving on')
    .help()
    // NOTE: We have to access the "argv" property in order to trigger "yargs"
    .argv
}
