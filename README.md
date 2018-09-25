# nysocks

[![npm-version](https://img.shields.io/npm/v/nysocks.svg?style=flat-square)](https://www.npmjs.com/package/nysocks) [![travis-ci](https://travis-ci.org/oyyd/nysocks.svg)](https://travis-ci.org/oyyd/nysocks)

[English](./doc/README_EN.md)

Nysocks是nodejs上利用[libuv](https://github.com/libuv/libuv)和[kcp](https://github.com/skywind3000/kcp)实现的代理工具。

- 特性
  - 通过kcp，它在经常发生丢包的网络环境下也能有较好的表现。
  - 支持[SOCKS5](https://www.ietf.org/rfc/rfc1928.txt)和[SS](https://shadowsocks.org/en/spec/Protocol.html)协议。
  - 加密传输数据。

对Linode上Tokyo 2, JP机房的测试（经常会发生10%的丢包）：

**一般ss代理:**
<br/>
<img src="https://cdn.rawgit.com/oyyd/nysocks/fa173e5c/imgs/tcp.png" width="475" />

**nysocks“fast2”模式:**
<br/>
<img src="https://cdn.rawgit.com/oyyd/nysocks/fa173e5c/imgs/fast2.png" width="475" />

## 工作方式

你需要一台可以连接到的服务器来代理网络请求。

**对于SOCKS协议**

![work-socks](https://cdn.rawgit.com/oyyd/nysocks/faae8240/imgs/work.png)

**对于SS协议**

![work-ss](https://cdn.rawgit.com/oyyd/nysocks/dev/imgs/work-ss.png)

目前支持ss协议的客户端较较多，因此你可以直接在服务器上同时启动一个ss协议的client和server来支持这些设备。但直接连接服务器上的client会没办法发挥kcp的优势。

**protocol(unstable):**

**NOTE:** 在nysocks v2.0.0中协议发生了改变，请确保客户端和服务器的版本都在该版本之上.

```
+-------+-----+-----+---------+---------+--------+------------+
|  kcp  | ver | cmd |  nonce  | mux.cmd | mux.id | mux.length |
+-------+-----+-----+---------+---------+--------+------------+
|  24   |  1  |  1  |    8    |    1    |    2   |     4      |
+-------+-----+-----+---------+---------+--------+------------+
```

## 安装

`node >= v6.x`

**请确保[node-gyp](https://github.com/nodejs/node-gyp#installation)可用**。

```
npm i nysocks -g
```

## 使用

#### 1. 创建server

在你的服务器上，执行`server`命令:

```
nysocks server -p 20000 -k YOUR_PASSWORD -m fast
```

#### 2 创建client

在你的客户端上，执行`client`命令连接到你的`server`上。之后`client`就可以对本地的其他应用提供代理服务了：

```
nysocks client -a YOUR_SERVER_HOST -p 20000 -k YOUR_PASSWORD -m fast
```

执行完上述命令后，当你的`client`显示`SOCKS5 service is listening on 1080`后，你就可以[利用SOCKS5协议](https://github.com/oyyd/nysocks#how-to-utilize-the-socks5-service)来进行代理了。nysocks的client同时会提供一个[PAC](https://en.wikipedia.org/wiki/Proxy_auto-config)（默认端口`8090`）文件服务来帮助你的应用确定哪些请求不需要转发。

**SS协议client**

你可以开启ss协议的client来代替SOCKS5协议：

```
nysocks client -a YOUR_SERVER_HOST -p 20000 -k YOUR_PASSWORD -m fast --client_protocol SS --ss_password YOUR_SS_PASSWORD --ss_method aes-128-cfb
```

关于nysocks中ss协议的实现，可以直接参考[encryptsocks](https://github.com/oyyd/encryptsocks) 中ssServer的实现。

#### 3. 用`config.json`文件

类似与ss，你可以将配置数据储存在`config.json`文件中，像[这里](#config)，以避免繁琐的命令行参数：

```
nysocks client -c config.json
```

#### 4. 开启守护进程

在实际使用时，你可以利用`-d`选项让client和server运行在守护进程下，以便在发生问题以后自动重启：

```
nysocks client -d restart -c config.json
```

nysock使用[pm2](http://pm2.keymetrics.io/)进行进程守护，你也可以通过nysocks命令行直接操作pm2，如查看正在运行的进程：

```
nysocks pm -- ls
```

清除所有的进程：

```
nysocks pm -- kill
```

#### 5. 查看其他配置项

你可以直接在命令行工具上查看所有可用的配置项:

```
nysocks -h
```

## 可用配置

```
nysocks <command>

Commands:
  nysocks server  Start a tunnel server.
  nysocks client  Start a tunnel client.
  nysocks pm      Alias of PM2(process manager) CLI that you can use to check or
                  operate processes directly.

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

`config.json`的例子:

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

### 如何利用SOCKS5服务

大多数操作系统本身支持直接利用SOCKS5进行代理:

![osx-set-proxy](https://cdn.comparitech.com/wp-content/uploads/2017/01/MacOS-Set-proxy.png)

chrome扩展[SwitchyOmega](https://github.com/FelisCatus/SwitchyOmega)可以帮助你的浏览器进行代理。

### 利用ss服务

查看这些[客户端](https://shadowsocks.org/en/download/clients.html)。nysocks中的ss服务相当于一般ss工具中server提供的服务。

### http/https代理

可以参考[hpts](https://github.com/oyyd/http-proxy-to-socks)和其他类似的工具，在SOCKS服务上支持http代理。

## 实现

通过[node-addon(C/CPP)](https://nodejs.org/api/addons.html)来实现（而非纯js）主要是因为:

1. Node在v8.7.0版本之前不支持设置send/recv buffer的大小.
2. 对于大量的数据传输而言，udp的收发非常频繁，并且我们需要不断修改buffer上的数据。直接在node上进行这类操作所造成的性能消耗对于网络代理工具而言是不可接受的，它会使整体的性能直接下降到不可用的程度。之前纯脚本的实现可以参考这里[here](https://github.com/oyyd/kcp-node/)。

**但addon在部署上相比node脚本而言显得十分困难**，主要有以下几点：
  1. addon需要编译，编译本身对运行环境有较多要求。对于部分window环境来说，需要额外安装一些依赖。
  2. prebuild的形式要求开发人员具备在多环境下编译的能力，并能持续稳定跟上node版本的迭代，支持新的node版本（N-API/ABI可能会减轻这一问题）。
  3. 要在其他node环境上（如electron）上使用addon基本上都需要额外的支持。也更难使用在非Node的js运行环境上（RN，web, chrome-extension）。

## 传输加密

目前默认使用`aes_256_cbc`进行加密，不可配置。

## 已知问题

- 不支持ipv6

## 参考

- [kcptun](https://github.com/xtaci/kcptun) - A Secure Tunnel Based On KCP with N:M Multiplexing
- [kcp](https://github.com/skywind3000/kcp) - A Fast and Reliable ARQ Protocol
- [C++ and Node.js Integration](http://scottfrees.com/ebooks/nodecpp/)

## LICENSE

BSD
