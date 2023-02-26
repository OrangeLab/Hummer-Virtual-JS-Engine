#pragma once

#include <hermes/VM/HostModel.h>
#include <napi/js_native_api_types.h>

namespace orangelabs {
class External final : public ::hermes::vm::HostObjectProxy {
 public:
  explicit External(::hermes::vm::Runtime &runtime, void *data,
                    NAPIFinalize finalizeCallback, void *finalizeHint);

  [[nodiscard]] void *getData() const;

  ~External() override;

  ::hermes::vm::CallResult<::hermes::vm::HermesValue> get(
      ::hermes::vm::SymbolID symbolId) override;

  ::hermes::vm::CallResult<bool> set(::hermes::vm::SymbolID symbolId,
                                     ::hermes::vm::HermesValue value) override;

  ::hermes::vm::CallResult<::hermes::vm::Handle<::hermes::vm::JSArray>>
  getHostPropertyNames() override;

  External(const External &) = delete;

  External(External &&) = delete;

  External &operator=(const External &) = delete;

  External &operator=(External &&) = delete;

 private:
  ::hermes::vm::Runtime &runtime;
  void *const data;
  const NAPIFinalize finalizeCallback;
  void *const finalizeHint;
};
}  // namespace orangelabs
