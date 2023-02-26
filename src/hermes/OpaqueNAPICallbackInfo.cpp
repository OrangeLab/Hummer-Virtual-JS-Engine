#include <hermes/OpaqueNAPICallbackInfo.h>

OpaqueNAPICallbackInfo::OpaqueNAPICallbackInfo(
    ::hermes::vm::NativeArgs nativeArgs, void *data)
    : nativeArgs(std::move(nativeArgs)), data(data) {}

const ::hermes::vm::NativeArgs &OpaqueNAPICallbackInfo::getNativeArgs() const {
  return this->nativeArgs;
}

void *OpaqueNAPICallbackInfo::getData() const { return this->data; }