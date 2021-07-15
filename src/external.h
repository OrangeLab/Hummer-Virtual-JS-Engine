#ifndef SKIA_HERMESEXTERNALOBJECT_H
#define SKIA_HERMESEXTERNALOBJECT_H

#include <hermes/VM/HostModel.h>
#include <hermes/VM/JSArray.h>
#include <napi/js_native_api_types.h>

#define NAPI_HIDDEN __attribute((visibility("hidden")))

namespace napi
{
class NAPI_HIDDEN External final : public ::hermes::vm::HostObjectProxy
{
  public:
    External(NAPIEnv env, void *data, NAPIFinalize finalizeCallback, void *finalizeHint);

    NAPIEnv getEnv() const;
    void *getData() const;
    NAPIFinalize getFinalizeCallback() const;
    void *getFinalizeHint() const;

    ~External() override;

    ::hermes::vm::CallResult<::hermes::vm::HermesValue> get(::hermes::vm::SymbolID symbolId) override;

    ::hermes::vm::CallResult<bool> set(::hermes::vm::SymbolID symbolId, ::hermes::vm::HermesValue value) override;

    ::hermes::vm::CallResult<::hermes::vm::Handle<::hermes::vm::JSArray>> getHostPropertyNames() override;

    // copy ctor
    External(const External &) = delete;

    // move ctor
    External(External &&) = delete;

    // copy assign
    External &operator=(const External &) = delete;

    // move assign
    External &operator=(External &&) = delete;

  private:
    NAPIEnv env;
    void *data;
    NAPIFinalize finalizeCallback;
    void *finalizeHint;
};
} // namespace napi

#endif // SKIA_HERMESEXTERNALOBJECT_H
