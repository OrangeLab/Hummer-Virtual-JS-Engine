## N-API

Node-API（以前称为 N-API）是一个用于和 JavaScript 引擎交互并独立于 JavaScript 运行时的 API 接口层，旨在将 C 原生代码和 JavaScript 引擎隔离开来。API 通常用于创建和操作
JavaScript 值，概念和操作通常映射到 ECMA-262 语言规范，API 具有以下特点：

1. 所有节点API调用都返回类型为 NAPIStatus 的状态代码。此状态指示 API 调用是成功还是失败。
2. API 的返回值通过 out 参数传递。
3. 所有 JavaScript 值都抽象在一个名为 NAPIValue 的不透明类型后面。
4. 如果出现 NAPIPendingException 说明出现 JS 异常，可以通过 napi_get_and_clear_last_exception 获取

## 代码静态分析

### JavaScriptCore

1. 静态分析 clang --analyze src/js_native_api_jsc.c --analyzer-output html -I./include -I./third_party/uthash/src
2. 报告输出 clang --analyze src/js_native_api_jsc.c --analyzer-output html -o ./static_analyze_report -I./include
   -I./third_party/uthash/src

### QuickJS

1. 静态分析 clang --analyze src/js_native_api_qjs.c --analyzer-output html -I./include -I./third_party/quickjs

## 单元测试

1. gn gen out --args="debug=true asan=true ubsan=true"（Release 模式下 assert 失效，会隐藏很多问题，特殊情况下启用 asan + ubsan）
2. ninja -C out test_{qjs|jsc|hermes}
3. ./out/test_{qjs|jsc|hermes}
4. ubsan/asan 对于 QuickJS 开启，需要注释掉 CONFIG_STACK_CHECK，否则会报告 InternalError: stack overflow，需要手动修改 QuickJS 源代码
5. QuickJS 单元测试建议开启 DUMP_LEAKS，需要手动修改 QuickJS 源代码

## 编辑器配置

1. 推荐使用 CLion
2. VSCode

### CLion

1. gn gen clion --ide=json --json-ide-script=../gn_to_cmake.py

### VSCode

1. C/C++ 插件
2. 自行配置头文件搜索路径

## 交叉编译 iOS 静态库

1. gn gen x86_64 --args="build_ios=true ios_archtecture=\"x86_64\""
2. gn gen i386 --args="build_ios=true ios_archtecture=\"i386\""
3. gn gen armv7 --args="build_ios=true ios_simulator=false ios_archtecture=\"armv7\""
4. gn gen arm64 --args="build_ios=true ios_simulator=false ios_archtecture=\"arm64\""
5. mkdir universal

### JavaScriptCore

1. ninja -C x86_64 napi_jsc
2. ninja -C i386 napi_jsc
3. ninja -C armv7 napi_jsc
4. ninja -C arm64 napi_jsc
5. lipo -create x86_64/obj/libnapi_jsc.a i386/obj/libnapi_jsc.a armv7/obj/libnapi_jsc.a arm64/obj/libnapi_jsc.a -output
   universal/libnapi_jsc.a
6. universal/libnapi_jsc.a 即为最终产物

### QuickJS

1. ninja -C x86_64 quickjs && ninja -C x86_64 napi_qjs
2. ninja -C i386 quickjs && ninja -C i386 napi_qjs
3. ninja -C armv7 quickjs && ninja -C armv7 napi_qjs
4. ninja -C arm64 quickjs && ninja -C arm64 napi_qjs
5. lipo -create x86_64/obj/libquickjs.a i386/obj/libquickjs.a armv7/obj/libquickjs.a arm64/obj/libquickjs.a -output
   universal/libquickjs.a
6. lipo -create x86_64/obj/libnapi_qjs.a i386/obj/libnapi_qjs.a armv7/obj/libnapi_qjs.a arm64/obj/libnapi_qjs.a -output
   universal/libnapi_qjs.a
7. universal/lib{napi_qjs|quickjs}.a 即为最终产物

## 交叉编译 Android 动态库

1. gn gen armv7 --args="build_android=true android_target=\"armv7-none-linux-androideabi18\"" && ninja -C armv7 napi_qjs
2. gn gen arm64 --args="build_android=true android_target=\"aarch64-none-linux-android21\"" && ninja -C arm64 napi_qjs
3. gn gen i386 --args="build_android=true android_target=\"i686-none-linux-android18\"" && ninja -C i386 napi_qjs
4. gn gen x86_64 --args="build_android=true android_target=\"x86_64-none-linux-android21\"" && ninja -C x86_64 napi_qjs
5. mkdir -p universal/armv7 && mkdir -p universal/arm64 && mkdir -p universal/i386 && mkdir -p universal/x86_64
6. mv armv7/obj/*.so universal/armv7 && mv arm64/obj/*.so universal/arm64 && mv i386/obj/*.so universal/i386 && mv
   x86_64/obj/*.so universal/x86_64
7. cp -r include universal 复制头文件
6. 使用非 LTS 版本 NDK 需要添加 -Wno-implicit-const-int-float-conversion 参数，建议使用 BUILDCONFIG.gn 中定义的 NDK 版本

## 注意事项

1. 只能在单一线程执行，不跨多线程执行，JavaScriptCore 通过锁机制保证同步，但是其他引擎很可能没有该保证
