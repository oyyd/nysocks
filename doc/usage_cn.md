# 使用简介

安装完node和node-gyp后：

```
npm i -g nysocks
```

在对境外网络访问比较好的机器上开server：

```
nysocks server -k MY_PASSWORD -m fast
```

在境内机器或本机上开client（默认SOCKS5）：

```
nysocks client -k MY_PASSWORD -s my.host.net -m fast
```

或者开SS协议的client：

```
nysocks client -c config.json --cp SS --ss_password MY_SS_PASSWORD --ss_method aes-128-cfb
```

另外`-d start`开启守护进程，`-c config.json`指定配置文件

守护进程启动后，可以通过`-s`命令查看进程状态：

```
nysocks client -s
```

```
nysocks:client
pid: 86240, status: online, restart: 0, memory: 61.23 MB cpu: 0%
```

底层传输默认`aes_256_cbc`加密，流量过境时，协议应该不会被轻易识别阻拦，原理同SS。

其他参数参考[文档](https://github.com/oyyd/nysocks#configs)
