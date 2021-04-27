## macOS 编译
1. gn gen out（如果希望编译 Debug 版本，需要加上 --args="debug=true" 参数）
2. ninja -C out
3. mv out/obj/libnapi_jsc.a .
4. include/napi 即为头文件

## 编译参数
1. debug -O0/s
2. code_coverage 代码覆盖率
3. asan/ubsan -fsanitize=undefined/address

## 代码静态分析
1. 静态分析 clang --analyze src/js_native_api_jsc.c --analyzer-output html -Wall -Wextra -Werror -Wpedantic -g -I./include -I./third_party/uthash/src
2. 报告输出 clang --analyze src/js_native_api_jsc.c --analyzer-output html -o ./static_analyze_report -Wall -Wextra -Werror -Wpedantic -g -I./include -I./third_party/uthash/src

## 单元测试
1. gn gen out --args="debug=true"
2. ./out/test

## 编辑器配置
1. 推荐使用 CLion
2. VSCode

### CLion
1. https://skia.googlesource.com/skia/+/refs/heads/master/gn/gn_to_cmake.py
2. gn gen clion --ide=json --json-ide-script=../gn_to_cmake.py

### VSCode
1. C/C++ 插件
