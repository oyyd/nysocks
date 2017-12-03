import {
  createClient, createManager, freeManager,
  checkJSONMsg, checkHandshakeMsg,
} from '../socket_manager'
import { startKcpuv, stopKcpuv } from '../socket'

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

  // describe('createManager', () => {
  //   it('should create a manager and a client and then free them', (done) => {
  //     startKcpuv()
  //
  //     const serverAddr = '0.0.0.0'
  //     const password = 'hello'
  //     const serverPort = 20020
  //
  //     const manager = createManager({
  //       password,
  //       serverAddr,
  //       serverPort,
  //     }, () => {})
  //
  //     createClient({
  //       password,
  //       serverAddr,
  //       serverPort,
  //     }).then(client => {
  //       console.log('client', client)
  //       freeManager(client)
  //       // done()
  //     })
  //   })
  // })
})
