#pragma once

#include <napi/js_native_api_types.h>

namespace orangelabs {
class FunctionInfo final {
public:
  explicit FunctionInfo(NAPIEnv env, NAPICallback callback, void *data);
  [[nodiscard]] NAPIEnv getEnv() const;
  [[nodiscard]] NAPICallback getCallback() const;
  [[nodiscard]] void *getData() const;

  FunctionInfo(const FunctionInfo &) = delete;

  FunctionInfo(FunctionInfo &&) = delete;

  FunctionInfo &operator=(const FunctionInfo &) = delete;

  FunctionInfo &operator=(FunctionInfo &&) = delete;

private:
  NAPIEnv env;
  NAPICallback callback;
  void *data;
};
} // namespace orangelabs