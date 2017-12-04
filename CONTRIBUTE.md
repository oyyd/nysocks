# CONTRIBUTE

## Build C/C++ Isolately

### Build with CMAKE

**OSX**

```sh
$ ./deps/gyp/gyp --depth=. -D uv_library=static_library kcpuv.gyp && \
  xcodebuild -ARCHS="x86_64" -project kcpuv.xcodeproj -configuration Release -target kcpuv
```
