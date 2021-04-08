## macOS 编译
1. gn
2. ninja
3. gn gen out
4. ninja -C out
5. mv out/obj/libnapi_jsc.a .
6. include/napi 即为头文件

## 编辑器配置
1. 推荐使用 CLion
2. VSCode

### CLion
1. https://skia.googlesource.com/skia/+/refs/heads/master/gn/gn_to_cmake.py
2. ~/Downloads/gn-mac-amd64/gn gen out --ide=json --json-ide-script=/path/to/gn_to_cmake.py

### VSCode
1. C/C++ 插件