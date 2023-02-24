#
#构建Android quickjs 引擎
#
# sh build-hermes-android.sh
rm -rf armv7
rm -rf arm64
rm -rf i386
rm -rf x86_64

gn gen armv7 --args="build_android=true cross_compile_target=\"armv7\""
gn gen arm64 --args="build_android=true cross_compile_target=\"arm64\""
gn gen i386 --args="build_android=true cross_compile_target=\"i386\""
gn gen x86_64 --args="build_android=true cross_compile_target=\"x86_64\""

ninja -C armv7 hermes  && ninja -C arm64 hermes  && ninja -C i386 hermes  && ninja -C x86_64 hermes

mkdir -p napi/libs/armeabi-v7a && mkdir -p napi/libs/arm64-v8a && mkdir -p napi/libs/x86 && mkdir -p napi/libs/x86_64

cp armv7/obj/lib{hermes,qjs}.so napi/libs/armeabi-v7a
cp arm64/obj/lib{hermes,qjs}.so napi/libs/arm64-v8a
cp i386/obj/lib{hermes,qjs}.so napi/libs/x86
cp x86_64/obj/lib{hermes,qjs}.so napi/libs/x86_64


tar czvf napi_hermes_android.tar.gz -C napi .


