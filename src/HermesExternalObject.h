#ifndef SKIA_HERMESEXTERNALOBJECT_H
#define SKIA_HERMESEXTERNALOBJECT_H

#include <hermes/VM/HostModel.h>
#include <hermes/VM/JSArray.h>
#include <napi/js_native_api_types.h>

namespace napi
{
class HermesExternalObject final : public ::hermes::vm::HostObjectProxy
{
  public:
    HermesExternalObject(NAPIEnv env, void *data, NAPIFinalize finalizeCallback, void *finalizeHint);

    NAPIEnv getEnv() const;
    void *getData() const;
    NAPIFinalize getFinalizeCallback() const;
    void *getFinalizeHint() const;

    ~HermesExternalObject() override;

    ::hermes::vm::CallResult<::hermes::vm::HermesValue> get(::hermes::vm::SymbolID symbolId) override;

    ::hermes::vm::CallResult<bool> set(::hermes::vm::SymbolID symbolId, ::hermes::vm::HermesValue value) override;

    ::hermes::vm::CallResult<::hermes::vm::Handle<::hermes::vm::JSArray>> getHostPropertyNames() override;

    // copy ctor
    HermesExternalObject(const HermesExternalObject &) = delete;

    // move ctor
    HermesExternalObject(HermesExternalObject &&) = delete;

    // copy assign
    HermesExternalObject &operator=(const HermesExternalObject &) = delete;

    // move assign
    HermesExternalObject &operator=(HermesExternalObject &&) = delete;

  private:
    NAPIEnv env;
    void *data;
    NAPIFinalize finalizeCallback;
    void *finalizeHint;
};
} // namespace napi

#endif // SKIA_HERMESEXTERNALOBJECT_H
