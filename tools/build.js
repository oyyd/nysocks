const cp = require('child_process')

const TARGETS = {
  // TODO: Assert the existence of the binary lib.
  // By gyp and make.
  // No need to build "Release" version as we only use it for tests.
  // The actual build for production is provided by node-gyp.
  build_libuv: 'cd deps/libuv/ && mkdir -p build/gyp && cp -r ../gyp/ ./build/gyp/ && ./gyp_uv.py -f make && make -C out',
  // TODO: Check the commands below.
  test: 'mkdir -p test/build && cd test/build && cmake .. && \
    make all && ./unit_test',
  build_kcpuv: `./deps/gyp/gyp --depth=. -D uv_library=static_library kcpuv.gyp && \
    xcodebuild -ARCHS="x86_64" -project kcpuv.xcodeproj -configuration Release -target kcpuv`
}

function main() {
  const length = process.argv.length
  const target = process.argv[length - 1]

  if (!TARGETS[target]) {
    console.log(`Unexpected target. Expect the target to be one of: ${Object.key(TARGETS).join(', ')}`)
    return
  }

  const cmd = TARGETS[target]

  const child = cp.exec(cmd)

  child.stdout.pipe(process.stdout)
  child.stderr.pipe(process.stderr)
}

module.exports = main

if (module === require.main) {
  main()
}
