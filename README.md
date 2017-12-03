# nysocks

Nysocks binds [kcp](https://github.com/skywind3000/kcp) and [libuv](https://github.com/libuv/libuv) to provide a tunnel for nodejs that focus on packet loss.

![node_addon](https://cdn.rawgit.com/oyyd/kcpuv/b76a8cbd/imgs/node_addon.png)

![work](https://cdn.rawgit.com/oyyd/kcpuv/b76a8cbd/imgs/work.png)

## Usage

### SOCKS Clients

## CLI Options

## Config

```json
{
  "serverAddr": "YOUR_SERVER_HOST",
  "serverPort": 20000,
  "socketAmount": 20,
  "password": "YOUR_PASSWORD",
  "kcp": {
    "sndwnd": 4096,
    "rcvwnd": 4096,
    "nodelay": 0,
    "interval": 30,
    "resend": 2,
    "nc": 1
  },
  "pac": {
    "pacServerPort": 8090
  },
  "SOCKS": {
    "port": 1080
  }
}
```

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
