#!/bin/bash
if [ ! -d ./third_party/hermes ]; then
    echo "hermes submodule not found"
    exit -1
fi

if [  -d ./third_party/hermes/destroot/Library/Frameworks/iphoneos/hermes.framework ]; then

    rm -f ./third_party/hermes/destroot/
fi
cd ./third_party/hermes/
DEBUG=true
source ./utils/build-mac-framework.sh

cd ..
cd ..
if [ -d ./build_hermes ]; then
    rm -f ./build_hermes/*
else
    mkdir build_hermes
fi
target_dir="build_hermes"
source_dir="./third_party/hermes/build_macosx"
 
echo "copying static library"
## copy .a
cp $source_dir/API/hermes/libhermesapi.a $target_dir
cp $source_dir/external/dtoa/libdtoa.a $target_dir
cp $source_dir/external/llvh/lib/Support/libLLVHSupport.a $target_dir
cp $source_dir/external/llvh/lib/Demangle/libLLVHDemangle.a $target_dir
cp $source_dir/lib/libhermesFrontend.a $target_dir
cp $source_dir/lib/libhermesOptimizer.a $target_dir
cp $source_dir/lib/Inst/libhermesInst.a $target_dir
cp $source_dir/lib/FrontEndDefs/libhermesFrontEndDefs.a $target_dir
cp $source_dir/lib/InternalBytecode/libhermesInternalBytecode.a $target_dir
cp $source_dir/lib/ADT/libhermesADT.a $target_dir
cp $source_dir/lib/AST/libhermesAST.a $target_dir
cp $source_dir/lib/Parser/libhermesParser.a $target_dir
cp $source_dir/lib/SourceMap/libhermesSourceMap.a $target_dir
cp $source_dir/lib/Support/libhermesSupport.a $target_dir
cp $source_dir/lib/BCGen/libhermesBackend.a $target_dir
cp $source_dir/lib/BCGen/HBC/libhermesHBCBackend.a $target_dir
cp $source_dir/lib/Regex/libhermesRegex.a $target_dir
cp $source_dir/lib/Platform/libhermesPlatform.a $target_dir
cp $source_dir/lib/Platform/Unicode/libhermesPlatformUnicode.a $target_dir
cp $source_dir/lib/VM/libhermesVMRuntime.a $target_dir




