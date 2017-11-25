import { createMux, muxFree } from '../mux'

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
})
