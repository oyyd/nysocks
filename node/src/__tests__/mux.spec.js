import { createMux } from '../mux'

describe('mux', () => {
  describe('createMux', () => {
    it('should create a wrapped mux object', () => {
      const obj = createMux()

      expect(obj).toBeTruthy()
    })
  })
})
