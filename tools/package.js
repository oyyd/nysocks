// Package '../build' into a zip file.
const yazl = require('yazl')
const path = require('path')
const os = require('os')
const fs = require('fs')
const packageJSON = require('../package.json')

function getName() {
  return `${packageJSON.name}_v${packageJSON.version}_${os.platform()}_${os.arch()}`
}

function main() {
  const zipfile = new yazl.ZipFile()
  const ADDON_NODE = 'addon.node'

  zipfile.addFile(path.resolve(__dirname, `../build/Release/${ADDON_NODE}`), ADDON_NODE)

  zipfile.outputStream.pipe(fs.createWriteStream(path.resolve(__dirname, `../${getName()}.zip`)))

  zipfile.end()
}

if (module === require.main) {
  main()
}
