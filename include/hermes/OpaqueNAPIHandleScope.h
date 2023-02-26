#pragma once

#include <hermes/VM/HandleRootOwner.h>
#include <napi/js_native_api_types.h>

EXTERN_C_START

#include <sys/queue.h>

struct OpaqueNAPIHandleScope {
 public:
  explicit OpaqueNAPIHandleScope(::hermes::vm::HandleRootOwner &runtime);
  virtual ~OpaqueNAPIHandleScope() = default;

  OpaqueNAPIHandleScope(const OpaqueNAPIHandleScope &) = delete;

  OpaqueNAPIHandleScope(OpaqueNAPIHandleScope &&) = delete;

  OpaqueNAPIHandleScope &operator=(const OpaqueNAPIHandleScope &) = delete;

  OpaqueNAPIHandleScope &operator=(OpaqueNAPIHandleScope &&) = delete;

  SLIST_ENTRY(OpaqueNAPIHandleScope) node;

 private:
  ::hermes::vm::GCScope gcScope;
};

EXTERN_C_END