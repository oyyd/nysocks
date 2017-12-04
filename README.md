# nysocks

Nysocks binds [kcp](https://github.com/skywind3000/kcp) and [libuv](https://github.com/libuv/libuv) to provide a tcp tunnel in nodejs.

Proxy tests from a Linode instance(Tokyo 2, JP) where 10% packet loss always happend:

**tcp proxy:**
<br/>
<img src="https://cdn.rawgit.com/oyyd/nysocks/fa173e5c/imgs/tcp.png" style="height: 200px"/>

**nysocks fast mode:**
<br/>
<img src="https://cdn.rawgit.com/oyyd/nysocks/fa173e5c/imgs/fast.png" style="height: 200px"/>

**nysocks fast2 mode:**
<br/>
<img src="https://cdn.rawgit.com/oyyd/nysocks/fa173e5c/imgs/fast2.png" style="height: 200px"/>

## Architecture

![work](https://cdn.rawgit.com/oyyd/nysocks/fa173e5c/imgs/work.png)

## Installation

```
npm i nysocks -g
```

## Usage

In your server, create your `config.json` file like [this](#config) and start:

```
nysocks server -c config.json
```

In your client, create a same `config.json` file and start:

```
nysocks client -c config.json
```

Nysocks will start a SOCKS5 service to tunnel your tcp connections. A PAC file server will also be served(default port `8090`).

Add `-d` options if you want to run under daemons([pm2](http://pm2.keymetrics.io/)):

```
nysocks client -d restart -c config.json
```

Modify your options in the CLI. See other options here:

```
nysocks -h
```

### How to utilize the SOCKS5 service

Most OSes support SOCKS5 proxy by default:

![osx-set-proxy](https://cdn.comparitech.com/wp-content/uploads/2017/01/MacOS-Set-proxy.png)

Chrome extension [Proxy SwitchySharp](https://chrome.google.com/webstore/detail/proxy-switchysharp/dpplabbmogkhghncfbfdeeokoefdjegm).

## Configs

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

`aes_256_cbc`

## Known Issues

- Do not support ipv6.
- Changing the ip of the client will disconnect all the connections.

## References

- [kcptun](https://github.com/xtaci/kcptun) - A Secure Tunnel Based On KCP with N:M Multiplexing
- [kcp](https://github.com/skywind3000/kcp) - A Fast and Reliable ARQ Protocol

## LICENSE

BSD
