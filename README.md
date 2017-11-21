# kcpuv

Kcpuv binds [kcp](https://github.com/skywind3000/kcp) and [libuv](https://github.com/libuv/libuv) to provide a tunnel for nodejs.

![node_addon](https://cdn.rawgit.com/oyyd/kcpuv/b76a8cbd/imgs/node_addon.png)

![work](https://cdn.rawgit.com/oyyd/kcpuv/b76a8cbd/imgs/work.png)

## Usage

## CLI Options

## Build

## Encryption

`aes_256_cbc`

### Build with CMAKE

**OSX**

```sh
$ ./deps/gyp/gyp --depth=. -D uv_library=static_library kcpuv.gyp && \
  xcodebuild -ARCHS="x86_64" -project kcpuv.xcodeproj -configuration Release -target kcpuv
```

## LICENSE

BSD
