import {
  createClient, createManager, freeManager,
  checkJSONMsg, checkHandshakeMsg,
  sendBuf, createConnection,
  sendClose, close, listen,
  bindEnd, sendStop,
  setWaitFinTimeout,
} from '../socket_manager'
import { startUpdaterTimer, stopUpdaterTimer, getPort } from '../socket'

describe('socket_manager', () => {
  describe('checkJSONMsg', () => {
    it('should return null if it\'s not the end', () => {
      const buffer = Buffer.from('{')
      const res = checkJSONMsg(buffer)

      expect(res).toBe(null)
    })

    it('should return an object if the msg is valid', () => {
      const buffer = Buffer.from('{}\\\\end')
      const res = checkJSONMsg(buffer)

      expect(typeof res).toBe('object')
    })

    it('should throw if the json msg is invalid', (done) => {
      const buffer = Buffer.from('{\\\\end')

      try {
        checkJSONMsg(buffer)
      } catch (err) {
        //
        done()
      }
    })
  })

  describe('checkHandshakeMsg', () => {
    it('should return true if it\' a valid handshake msg', () => {
      const buffer = Buffer.from('\\\\start')
      expect(checkHandshakeMsg(buffer)).toBe(true)
    })

    it('should return false if it\' a valid handshake msg', () => {
      const buffer = Buffer.from('unknown')
      expect(checkHandshakeMsg(buffer)).toBe(false)
    })
  })

  describe('createManager', () => {
    it('should create a manager and a client and then free them', (done) => {
      startUpdaterTimer()

      const serverAddr = '0.0.0.0'
      const password = 'hello'
      const socketAmount = 2
      const data = 'buffer'
      const timeout = 1000
      const bindEndCb = jest.fn()
      let serverPort = 0
      let manager = null
      let client = null

      const destruct = () => {
        expect(bindEndCb).toHaveBeenCalled()
        stopUpdaterTimer()
        done()
      }

      createManager({
        password,
        serverAddr,
        serverPort,
        socketAmount,
      }, (conn) => {
        sendBuf(conn, Buffer.from(data))

        listen(conn, msg => {
          expect(msg.toString('utf8')).toBe(data)
        })

        bindEnd(conn, () => {
          bindEndCb()

          setWaitFinTimeout(manager, timeout)
          setWaitFinTimeout(client, timeout)
          freeManager(client)
          freeManager(manager)
        })
      }, () => {
        destruct()
      }).then(m => {
        manager = m
        serverPort = getPort(manager.masterSocket)
        // eslint-disable-next-line
        _createClient()
      })

      function _createClient() {
        createClient({
          password,
          serverAddr,
          serverPort,
        }).then(c => {
          client = c
          // NOTE: This `conns` is actual muxes.
          const { conns, ports } = client
          expect(Array.isArray(ports)).toBe(true)
          expect(ports.length).toBe(socketAmount)
          expect(Array.isArray(conns)).toBe(true)
          expect(conns.length).toBe(socketAmount)

          const conn = createConnection(client)
          listen(conn, msg => {
            // console.log('client msg', msg)
          })
          sendBuf(conn, Buffer.from(data))
          sendStop(conn)
          // close(conn)
        })
      }
    })
  })
})
