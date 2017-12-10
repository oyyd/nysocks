# nysocks

[![npm-version](https://img.shields.io/npm/v/nysocks.svg?style=flat-square)](https://www.npmjs.com/package/nysocks)

Nysocks binds [kcp](https://github.com/skywind3000/kcp) and [libuv](https://github.com/libuv/libuv) to provide a tcp tunnel in nodejs.

    Nysocks is in an early stage. Please submit PRs or issues to help us improve it if you like it!

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

![work](https://cdn.rawgit.com/oyyd/nysocks/faae8240/imgs/work.png)

**protocol(unstable):**

```
+-----+-----+---------+-------+---------+--------+------------+
| ver | cmd |  nonce  |  kcp  | mux.cmd | mux.id | mux.length |
+-----+-----+---------+-------+---------+--------+------------+
|  1  |  1  |    8    |  24   |    1    |    2   |     4      |
+-----+-----+---------+-------+---------+--------+------------+
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

#### 1. Create server service

In your server, start nysocks with `server` command:

```
nysocks server -p 20000 -k YOUR_PASSWORD -m fast
```

#### 2. Create client service

In your client, start nysocks with `client` command to create a tunnel client that will connect to your server and provide proxy service:

```
nysocks client -a YOUR_SERVER_HOST -p 20000 -k YOUR_PASSWORD -m fast
```

Nysocks will start a SOCKS5 service to tunnel your tcp connections. Now you can [utilize the SOCKS5 service](https://github.com/oyyd/nysocks#how-to-utilize-the-socks5-service) (default port `1080`). A [PAC](https://en.wikipedia.org/wiki/Proxy_auto-config) file server will also be served(default port `8090`) for convenience.

#### 3. Use `config.json`

You can create a `config.json` file like [this](#config) that containing your configs to avoid verbose cli options:

```
nysocks client -c config.json
```

#### 4. Use daemons

Add `-d` options if you want to run under daemons([pm2](http://pm2.keymetrics.io/)):

```
nysocks client -d restart -c config.json
```

#### 5. Check other options

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
- [C++ and Node.js Integration](http://scottfrees.com/ebooks/nodecpp/)

## LICENSE

BSD
