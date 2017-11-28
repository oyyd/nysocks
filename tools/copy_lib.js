// eslint-disable-next-line
const fs = require('fs-extra')
const path = require('path')

const BASE_DEP_PATH = path.resolve(__dirname, '../deps')
const BASE_PATH = path.resolve(__dirname, '../lib')

const NODE_GYP_LIBS = [
  './kcp/ikcp.h',
  './kcp/ikcp.c',
]

function main() {
  fs.emptyDirSync(BASE_PATH)

  NODE_GYP_LIBS.forEach(file => {
    const p = path.resolve(BASE_DEP_PATH, file)
    const dir = path.dirname(p)
    fs.ensureDirSync(dir)
    fs.copySync(p, path.resolve(BASE_PATH, file))
  })
}

if (module === require.main) {
  main()
}
