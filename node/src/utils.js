function checkValidSocket(name, obj) {
  if (typeof obj !== 'object' || !obj[name]) {
    throw new Error(`try to manipulate invalid ${name}`)
  }
}

function wrap(name, func) {
  return (obj, ...args) => {
    checkValidSocket(name, obj)
    return func(obj, ...args)
  }
}

export function createBaseSuite(name) {
  if (typeof name !== 'string') {
    throw new Error(`invalid base name: ${name}`)
  }

  const suite = {
    name,
    wrap: wrap.bind(null, name),
    checkValidSocket: checkValidSocket.bind(null, name),
  }

  return suite
}
