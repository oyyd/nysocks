# kcpuv

Kcpuv binds [kcp](https://github.com/skywind3000/kcp) and [libuv](https://github.com/libuv/libuv) to provide a tunnel for nodejs.

## Build

### Build with CMAKE

**OSX**

```sh
$ ./deps/gyp/gyp --depth=. -D uv_library=static_library kcpuv.gyp && \
  xcodebuild -ARCHS="x86_64" -project kcpuv.xcodeproj -configuration Release -target kcpuv
```
