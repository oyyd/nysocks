# nysocks

[![npm-version](https://img.shields.io/npm/v/nysocks.svg?style=flat-square)](https://www.npmjs.com/package/nysocks) [![travis-ci](https://travis-ci.org/oyyd/nysocks.svg?branch=dev)](https://travis-ci.org/oyyd/nysocks)

[使用简介](./doc/usage_cn.md)

Nysocks binds [kcp](https://github.com/skywind3000/kcp) and [libuv](https://github.com/libuv/libuv) to provide a tcp tunnel in nodejs.

- Features
  - Aggresive ARQ make it works better than TCP in environments where packets loss always happens.
  - Support both [SOCKS5](https://www.ietf.org/rfc/rfc1928.txt) and [SS](https://shadowsocks.org/en/spec/Protocol.html) protocols.
  - Encrypt transmission.

Proxy tests from a Linode instance(Tokyo 2, JP) where 10% packet loss always happens when trasmitting data from to China mainland:

**tcp proxy:**
<br/>
<img src="https://cdn.rawgit.com/oyyd/nysocks/fa173e5c/imgs/tcp.png" width="475" />

**nysocks fast2 mode:**
<br/>
<img src="https://cdn.rawgit.com/oyyd/nysocks/fa173e5c/imgs/fast2.png" width="475" />

## How it works

**for SOCKS client**

![work-socks](https://cdn.rawgit.com/oyyd/nysocks/faae8240/imgs/work.png)

**for SS client**

![work-ss](https://cdn.rawgit.com/oyyd/nysocks/dev/imgs/work-ss.png)

**protocol(unstable):**

**NOTE:** The protocol of nysocks has been changed from v1.3.0 so that you have to install nysocks v1.3.x in both of your client and server.


```
+-------+-----+-----+---------+---------+--------+------------+
|  kcp  | ver | cmd |  nonce  | mux.cmd | mux.id | mux.length |
+-------+-----+-----+---------+---------+--------+------------+
|  24   |  1  |  1  |    8    |    1    |    2   |     4      |
+-------+-----+-----+---------+---------+--------+------------+
```

## About the Tunnel Implementation

The tunnel connections in nysocks is implemented as a [node-addon(C/CPP)](https://nodejs.org/api/addons.html) mainly because:

0. Node don't support setting send/recv buffer size of udp connections before v8.7.0.
0. In large data transmissions, udp message callback is too frequently and manipulating buffers in js(or any other script languages) is relatively expensive which would make the total performace unacceptable. You can check my pure js implementation [here](https://github.com/oyyd/kcp-node/).

Still, I'm not a professional C/CPP language user or network programmer. Please submit PRs or issues to help us improve it!

## Installation

`node >= 6.x`

**Make sure** you have [node-gyp](https://github.com/nodejs/node-gyp#installation) installed successfully as nysocks will build C/CPP code, then:

```
npm i nysocks -g
```

Or check the executable [here](https://github.com/oyyd/nysocks/releases).

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

**Use SS Protocol**

Nysocks supports using shadowsocks protocol to replace SOCKS5 protocol in your client:

```
nysocks client -a YOUR_SERVER_HOST -p 20000 -k YOUR_PASSWORD -m fast --client_protocol SS --ss_password YOUR_SS_PASSWORD --ss_method aes-128-cfb
```

Check [the ssServer from encryptsocks](https://github.com/oyyd/encryptsocks) for more details.

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

CLI:

```
nysocks <command>

Commands:
  nysocks server  Start a tunnel server.
  nysocks client  Start a tunnel client.

Options:
  --version                Show version number                         [boolean]
  --config, -c             The path of a json file that describe your
                           configuration.
  --daemon, -d             Run with a daemon(pm2): start, stop, restart.
  --daemon_status, -s      Show daemoned(pm2) processes status
  --mode, -m               Like kcptun: normal, fast, fast2, fast3.
  --password, -k           The passowrd/key for the encryption of transmissio.
  --socket_amount          The amount of connections to be created for each
                           client (default: 10)
  --server_addr, -a        The host of your server.
  --server_port, -p        The port of your server.
  --client_protocol, --cp  The protocol that will be used by clients: SS, SOCKS
                           (default: SOCKS)
  --socks_port             Specify the local port for SOCKS service (default:
                           1080)
  --ss_port                Specify the local port for ssServer service (default:
                           8083)
  --ss_password            Specify the key for the encryption of ss
  --ss_method              Specify the method of the encryption for ss (default:
                           aes-128-cfb)
  --log_path               The file path for logging. If not set, will log to
                           the console.
  --log_memory             Log memory info.
  --log_conn               Log connections info.
  --help                   Show help                                   [boolean]

```

`config.json` example:

```json
{
  "serverAddr": "YOUR_SERVER_HOST",
  "serverPort": 20000,
  "socketAmount": 20,
  "password": "YOUR_PASSWORD",
  "kcp": {
    "sndwnd": 1024,
    "rcvwnd": 1024,
    "nodelay": 0,
    "interval": 30,
    "resend": 2,
    "nc": 1
  },
  "pac": {
    "pacServerPort": 8090
  },
  "clientProtocol": "SOCKS",
  "SOCKS": {
    "port": 1080
  },
  "SS": {
    "password": "YOUR_SS_PASSWORD",
    "method": "aes-128-cfb",
    "serverPort": 8083,
  }
}
```

### How to utilize the SOCKS5 service

Most OSes support SOCKS5 proxy by default:

![osx-set-proxy](https://cdn.comparitech.com/wp-content/uploads/2017/01/MacOS-Set-proxy.png)

Use chrome extensions like [SwitchyOmega](https://github.com/FelisCatus/SwitchyOmega) to help browse web pages by proxy.

### How to utilize the SS service

Install [clients](https://shadowsocks.org/en/download/clients.html) in your devices and connecting to the ssServer set up by nysocks.

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
