#pragma once

#include <napi/js_native_api_types.h>
#include <variant>

#include <hermes/VM/HermesValue.h>
#include <hermes/VM/WeakRoot.h>

namespace hermes::vm {
class JSObject;
} // namespace hermes::vm

EXTERN_C_START

#include <sys/queue.h>

struct OpaqueNAPIRef final {

public:
  explicit OpaqueNAPIRef(
      NAPIEnv env, const ::hermes::vm::PinnedHermesValue &pinnedHermesValue,
      uint8_t referenceCount);

  OpaqueNAPIRef(const OpaqueNAPIRef &) = delete;

  OpaqueNAPIRef(OpaqueNAPIRef &&) = delete;

  OpaqueNAPIRef &operator=(const OpaqueNAPIRef &) = delete;

  OpaqueNAPIRef &operator=(OpaqueNAPIRef &&) = delete;

  ~OpaqueNAPIRef();

  void ref();

  void unref();

  [[nodiscard]] uint8_t getReferenceCount() const;

  [[nodiscard]] const ::hermes::vm::PinnedHermesValue *getHermesValue() const;

  ::std::variant<::std::monostate, ::hermes::vm::PinnedHermesValue,
                 ::hermes::vm::WeakRoot<::hermes::vm::JSObject>>
      storage;

  LIST_ENTRY(OpaqueNAPIRef) node;

private:
  const NAPIEnv env;
  uint8_t referenceCount;
};

EXTERN_C_END