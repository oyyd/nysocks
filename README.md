# <img src='https://raw.githubusercontent.com/oyyd/nysocks/40e4ed4a/imgs/icon.png' height='40'> nysocks

Nysocks binds [kcp](https://github.com/skywind3000/kcp) and [libuv](https://github.com/libuv/libuv) to provide a tunnel for nodejs that focus on packet loss.

![node_addon](https://cdn.rawgit.com/oyyd/kcpuv/b76a8cbd/imgs/node_addon.png)

![work](https://cdn.rawgit.com/oyyd/kcpuv/b76a8cbd/imgs/work.png)

## Usage

## CLI Options

## Encryption

## Known Issues

- Do not support ipv6 currently.

## Build

`aes_256_cbc`

### Build with CMAKE

**OSX**

```sh
$ ./deps/gyp/gyp --depth=. -D uv_library=static_library kcpuv.gyp && \
  xcodebuild -ARCHS="x86_64" -project kcpuv.xcodeproj -configuration Release -target kcpuv
```

## References

- [kcptun](https://github.com/xtaci/kcptun) - A Secure Tunnel Based On KCP with N:M Multiplexing

## LICENSE

BSD
