# CHANGELOG

## 2.0.0

- Dep: Update libuv to the latest master branch.
- Feature: Add buffers for each connections to make them call js callbacks less frequently.
- Deployment: Add docker configuration.
- CLI: Log pid.
- Doc: Add basic docs.

## 1.2.9
- Feature: Use `--daemon_status`, `-s` to show the status of running daemons.
- C/C++: Refactor cpp tests.

## 1.2.8
- Fix: Fix memory leaks when using `Nan::Persistent<T>` which will create a new instance.

## 1.2.7
- Fix: Fix memory leaks in `binding.cc`.

## 1.2.6
- Fix: Close socks5 client clearly before restarting.

## 1.2.5
- Fix: SOCKS port occupied by the client itself and throw when restarted.

## 1.2.4
- Feature: Support SS protocol in clients.

## 1.1.1
- Fix: Calculate heart beating time correctly.
- Fix: Close uv handle correctly.

## 1.1.0
- Released.
