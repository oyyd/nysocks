# nysocks

[![npm-version](https://img.shields.io/npm/v/nysocks.svg?style=flat-square)](https://www.npmjs.com/package/nysocks)

Nysocks binds [kcp](https://github.com/skywind3000/kcp) and [libuv](https://github.com/libuv/libuv) to provide a tcp tunnel in nodejs.

Proxy tests from a Linode instance(Tokyo 2, JP) where 10% packet loss always happens when trasmission data from to China mainland:

**tcp proxy:**
<br/>
<img src="https://cdn.rawgit.com/oyyd/nysocks/fa173e5c/imgs/tcp.png" width="475" />

**nysocks fast mode:**
<br/>
<img src="https://cdn.rawgit.com/oyyd/nysocks/fa173e5c/imgs/fast.png" width="475" />

**nysocks fast2 mode:**
<br/>
<img src="https://cdn.rawgit.com/oyyd/nysocks/fa173e5c/imgs/fast2.png" width="475" />

## How it works

![work](https://cdn.rawgit.com/oyyd/nysocks/fa173e5c/imgs/work.png)

**protocol(unstable):**

```
+-----+-----+---------+---------+--------+------------+
| kcp | CMD |  nonce  | mux.cmd | mux.id | mux.length |
+-----+-----+---------+---------+--------+------------+
| 24  |  1  |    8    |    1    |    2   |     4      |
+-----+-----+---------+---------+--------+------------+
```

## About the Tunnel Implementation

The tunnel connections in nysocks is implemented as a [node-addon(C/CPP)](https://nodejs.org/api/addons.html) mainly because:

0. Node don't support setting send/recv buffer size of udp connections before v8.7.0.
0. In large data transmissions, udp message callback is too frequently and manipulating buffers in js(or any other script languages) is relatively expensive which would make the total performace unacceptable. You can check my pure js implementation [here](https://github.com/oyyd/kcp-node/).

Still, I'm not a professional C/CPP language user or network programmer. Please submit PRs or issues to help us improve it!

## Installation

**Make sure** you have [node-gyp](https://github.com/nodejs/node-gyp#installation) installed successfully as nysocks will build C/CPP code, then:

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

### How to utilize the SOCKS5 service

Most OSes support SOCKS5 proxy by default:

![osx-set-proxy](https://cdn.comparitech.com/wp-content/uploads/2017/01/MacOS-Set-proxy.png)

Use chrome extensions like [SwitchyOmega](https://github.com/FelisCatus/SwitchyOmega) to help browse web pages by proxy.

## Encryption

`aes_256_cbc`

## Known Issues

- Do not support ipv6 currently.
- Changing the ip of the client will disconnect all the connections temporary.

## References

- [kcptun](https://github.com/xtaci/kcptun) - A Secure Tunnel Based On KCP with N:M Multiplexing
- [kcp](https://github.com/skywind3000/kcp) - A Fast and Reliable ARQ Protocol

## LICENSE

BSD
