#include <hermes/OpaqueNAPIHandleScope.h>
#include <hermes/VM/HandleRootOwner.h>

OpaqueNAPIHandleScope::OpaqueNAPIHandleScope(
    ::hermes::vm::HandleRootOwner &runtime)
    : gcScope(runtime) {}