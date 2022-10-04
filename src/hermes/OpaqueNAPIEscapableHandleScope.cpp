#include <hermes/OpaqueNAPIEscapableHandleScope.h>
#include <hermes/VM/Handle.h>
#include <hermes/VM/HandleRootOwner.h>

OpaqueNAPIEscapableHandleScope::OpaqueNAPIEscapableHandleScope(
    ::hermes::vm::HandleRootOwner &runtime,
    ::hermes::vm::MutableHandle<::hermes::vm::HermesValue>
        preservedMutableHandle)
    : OpaqueNAPIHandleScope(runtime),
      mutableHandle(::std::move(preservedMutableHandle)),
      escapeCalled(false) {}

bool OpaqueNAPIEscapableHandleScope::isEscapeCalled() const {
  return this->escapeCalled;
}

const ::hermes::vm::PinnedHermesValue *OpaqueNAPIEscapableHandleScope::escape(
    const ::hermes::vm::HermesValue &hermesValue) {
  if (this->escapeCalled) {
    return nullptr;
  }
  this->mutableHandle.set(hermesValue);
  this->escapeCalled = true;

  return this->mutableHandle.unsafeGetPinnedHermesValue();
}
