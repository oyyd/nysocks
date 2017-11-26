import { bindUdpSend } from '../socket'
import { connSend, createMux, createMuxConn, muxFree } from '../mux'

describe('mux', () => {
  describe('createMux', () => {
    it('should create a wrapped mux object', () => {
      const obj = createMux({
        password: 'hello',
        port: 0,
      })

      expect(obj).toBeTruthy()

      muxFree(obj)
    })
  })

  // TODO: refactor
  it('should work with bindUdpSend', () => {
    const mux = createMux({
      password: 'hello',
      port: 0,
      targetPort: 20005,
      targetAddr: '0.0.0.0',
    })
    const conn = createMuxConn(mux)

    bindUdpSend(mux.sess, (msg) => {
      // console.log('msg', msg)
      // muxFree(mux)
      // done()
    })

    // connSend(conn, Buffer.from('h'))
  })
})
