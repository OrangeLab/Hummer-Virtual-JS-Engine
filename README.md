## macOS 编译

1. gn gen out（如果希望编译 Debug 版本，需要加上 --args="debug=true" 参数）
2. ninja -C out napi_jsc
3. out/obj/libnapi_jsc.a 即为编译生成的静态库
4. include/napi 即为头文件

## 代码静态分析

### JavaScriptCore

1. 静态分析 clang --analyze src/js_native_api_jsc.c --analyzer-output html -I./include -I./third_party/uthash/src
2. 报告输出 clang --analyze src/js_native_api_jsc.c --analyzer-output html -o ./static_analyze_report -I./include
   -I./third_party/uthash/src

### QuickJS

1. 静态分析 clang --analyze src/js_native_api_qjs.c --analyzer-output html -I./include -I./third_party/quickjs

## 单元测试

1. gn gen out --args="debug=true"（Release 模式下 assert 失效，会隐藏很多问题，特殊情况下启用 asan + ubsan）
2. ninja -C out test
3. ./out/test
4. ubsan/asan 对于 QuickJS 开启，需要注释掉 CONFIG_STACK_CHECK，否则会报告 InternalError: stack overflow
5. QuickJS 单元测试建议开启 DUMP_LEAKS

## 编辑器配置

1. 推荐使用 CLion
2. VSCode

### CLion

1. gn gen clion --ide=json --json-ide-script=../gn_to_cmake.py

### VSCode

1. C/C++ 插件
2. 自行配置头文件搜索路径

## 交叉编译 iOS 静态库

1. gn gen x86_64 --args="build_ios=true ios_archtecture=\"x86_64\"" && ninja -C x86_64 napi_jsc
2. gn gen i386 --args="build_ios=true ios_archtecture=\"i386\"" && ninja -C i386 napi_jsc
3. gn gen armv7 --args="build_ios=true ios_simulator=false ios_archtecture=\"armv7\"" && ninja -C armv7 napi_jsc
4. gn gen arm64 --args="build_ios=true ios_simulator=false ios_archtecture=\"arm64\"" && ninja -C arm64 napi_jsc
5. mkdir universal && lipo -create x86_64/obj/libnapi_jsc.a i386/obj/libnapi_jsc.a armv7/obj/libnapi_jsc.a
   arm64/obj/libnapi_jsc.a -output universal/libnapi_jsc.a
6. universal/libnapi_jsc.a 即为最终产物

## 交叉编译 Android 动态库

1. gn gen armv7 --args="build_android=true android_target=\"armv7-none-linux-androideabi18\"" && ninja -C armv7 napi_qjs
2. gn gen arm64 --args="build_android=true android_target=\"aarch64-none-linux-android21\"" && ninja -C arm64 napi_qjs
3. gn gen i386 --args="build_android=true android_target=\"i686-none-linux-android18\"" && ninja -C i386 napi_qjs
4. gn gen x86_64 --args="build_android=true android_target=\"x86_64-none-linux-android21\"" && ninja -C x86_64 napi_qjs
5. mkdir -p universal/armv7 && mkdir -p universal/arm64 && mkdir -p universal/i386 && mkdir -p universal/x86_64
6. mv armv7/obj/*.so universal/armv7 && mv arm64/obj/*.so universal/arm64 && mv i386/obj/*.so universal/i386 && mv
   x86_64/obj/*.so universal/x86_64
7. cp -r include universal
6. 使用非 LTS 版本 NDK 需要添加 -Wno-implicit-const-int-float-conversion 参数

## 注意事项

1. 只能在单一线程执行，不跨多线程执行，JavaScriptCore 通过锁机制保证同步，但是其他引擎很可能没有该保证
