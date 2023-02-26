#pragma once

#include <hermes/VM/NativeArgs.h>
#include <napi/js_native_api_types.h>

EXTERN_C_START

struct OpaqueNAPICallbackInfo final {
  explicit OpaqueNAPICallbackInfo(::hermes::vm::NativeArgs nativeArgs,
                                  void *data);
  ~OpaqueNAPICallbackInfo() = default;

  [[nodiscard]] const ::hermes::vm::NativeArgs &getNativeArgs() const;
  [[nodiscard]] void *getData() const;

  OpaqueNAPICallbackInfo(const OpaqueNAPICallbackInfo &) = delete;

  OpaqueNAPICallbackInfo(OpaqueNAPICallbackInfo &&) = delete;

  OpaqueNAPICallbackInfo &operator=(const OpaqueNAPICallbackInfo &) = delete;

  OpaqueNAPICallbackInfo &operator=(OpaqueNAPICallbackInfo &&) = delete;

 private:
  const ::hermes::vm::NativeArgs nativeArgs;
  void *const data;
};

EXTERN_C_END