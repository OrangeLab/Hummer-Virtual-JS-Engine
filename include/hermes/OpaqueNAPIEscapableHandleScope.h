#pragma once

#include <hermes/OpaqueNAPIHandleScope.h>
#include <hermes/VM/Handle.h>

EXTERN_C_START

struct OpaqueNAPIEscapableHandleScope final : public OpaqueNAPIHandleScope {
 public:
  explicit OpaqueNAPIEscapableHandleScope(
      ::hermes::vm::HandleRootOwner &runtime,
      ::hermes::vm::MutableHandle<::hermes::vm::HermesValue>
          preservedMutableHandle);
  ~OpaqueNAPIEscapableHandleScope() override = default;

  OpaqueNAPIEscapableHandleScope(const OpaqueNAPIEscapableHandleScope &) =
      delete;

  OpaqueNAPIEscapableHandleScope(OpaqueNAPIEscapableHandleScope &&) = delete;

  OpaqueNAPIEscapableHandleScope &operator=(
      const OpaqueNAPIEscapableHandleScope &) = delete;

  OpaqueNAPIEscapableHandleScope &operator=(OpaqueNAPIEscapableHandleScope &&) =
      delete;

  [[nodiscard]] bool isEscapeCalled() const;

  const ::hermes::vm::PinnedHermesValue *escape(
      const ::hermes::vm::HermesValue &hermesValue);

 private:
  ::hermes::vm::MutableHandle<::hermes::vm::HermesValue> mutableHandle;
  bool escapeCalled;
};

EXTERN_C_END