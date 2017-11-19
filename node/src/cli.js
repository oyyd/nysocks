import path from 'path'
import fs from 'fs'
import yargs from 'yargs'
// import { createClient, createServer } from './index'

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

function main() {
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
      handler: (argv) => console.log('ENTER', parseConfig(argv))
    })
    // .command('client', 'Start a socks client.')
    .help()
    // NOTE: We have to access the "argv" property in order to trigger "yargs"
    .argv
}

if (module === require.main) {
  main()
}
