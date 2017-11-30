import pm2 from 'pm2'
import path from 'path'

// eslint-disable-next-line
const log = console.log

function disconnect() {
  return new Promise(resolve => {
    pm2.disconnect(err => {
      if (err) {
        throw err
      }

      resolve()
    })
  })
}

function connect() {
  return new Promise(resolve => {
    pm2.connect(err => {
      if (err) {
        throw err
      }

      resolve()
    })
  })
}

function handleError(err) {
  return disconnect().then(() => {
    // TODO:
    // eslint-disable-next-line
    console.error(err)
  })
}

function getPM2Config(argv, type) {
  return connect().then(() => {
    const filePath = path.resolve(__dirname, '../../bin/nysocks')
    const pm2Config = {
      name: `nysocks:${type}`,
      script: filePath,
      exec_mode: 'fork',
      instances: 1,
      minUptime: 2000,
      maxRestarts: 3,
      args: argv,
      cwd: process.cwd(),
    }

    return {
      pm2Config,
    }
  })
}

function _start(argv, type) {
  return getPM2Config(argv, type)
    .then(({ pm2Config }) => new Promise(resolve => {
      pm2.start(pm2Config, (err, apps) => {
        if (err) {
          throw err
        }

        log('start')
        resolve(apps)
      })
    }))
    .then(() => disconnect())
}

function getRunningInfo(name) {
  return new Promise(resolve => {
    pm2.describe(name, (err, descriptions) => {
      if (err) {
        throw err
      }

      // TODO: there should not be more than one process
      //  “online”, “stopping”,
      //  “stopped”, “launching”,
      //  “errored”, or “one-launch-status”
      const status =
        descriptions.length > 0 &&
        descriptions[0].pm2_env.status !== 'stopped' &&
        descriptions[0].pm2_env.status !== 'errored'

      resolve(status)
    })
  })
}

function _stop(argv, type) {
  let config = null

  return getPM2Config(argv, type)
    .then(conf => {
      config = conf
      const { name } = config.pm2Config
      return getRunningInfo(name)
    })
    .then(isRunning => {
      const { pm2Config } = config
      const { name } = pm2Config

      if (!isRunning) {
        log('already stopped')
        return
      }

      // eslint-disable-next-line
      return new Promise(resolve => {
        pm2.stop(name, err => {
          if (err && err.message !== 'process name not found') {
            throw err
          }

          log('stop')
          resolve()
        })
      })
    })
    .then(() => disconnect())
}

/**
 * @public
 * @param  {[type]} args [description]
 * @return {[type]}      [description]
 */
export function start(...args) {
  return _start(...args).catch(handleError)
}

/**
 * @public
 * @param  {[type]} args [description]
 * @return {[type]}      [description]
 */
export function stop(...args) {
  return _stop(...args).catch(handleError)
}

/**
 * @public
 * @param  {[type]} args [description]
 * @return {[type]}      [description]
 */
export function restart(...args) {
  return _stop(...args).then(() => _start(...args)).catch(handleError)
}

// if (module === require.main) {
//   restart('local', {
//     password: 'holic123',
//     serverAddr: 'kr.oyyd.net',
//   });
// }
