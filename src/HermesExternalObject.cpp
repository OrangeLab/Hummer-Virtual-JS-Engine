#include "HermesExternalObject.h"

::napi::HermesExternalObject::HermesExternalObject(NAPIEnv env, void *data, NAPIFinalize finalizeCallback,
                                                   void *finalizeHint)
    : env(env), data(data), finalizeCallback(finalizeCallback), finalizeHint(finalizeHint)
{
}

NAPIEnv napi::HermesExternalObject::getEnv() const
{
    return env;
}
void *napi::HermesExternalObject::getData() const
{
    return data;
}
NAPIFinalize napi::HermesExternalObject::getFinalizeCallback() const
{
    return finalizeCallback;
}
void *napi::HermesExternalObject::getFinalizeHint() const
{
    return finalizeHint;
}
napi::HermesExternalObject::~HermesExternalObject()
{
    if (finalizeCallback)
    {
        finalizeCallback(env, data, finalizeHint);
    }
}
::hermes::vm::CallResult<::hermes::vm::HermesValue> napi::HermesExternalObject::get(::hermes::vm::SymbolID symbolId)
{
    return ::hermes::vm::Runtime::getUndefinedValue().get();
}
::hermes::vm::CallResult<bool> napi::HermesExternalObject::set(::hermes::vm::SymbolID symbolId,
                                                               ::hermes::vm::HermesValue value)
{
    return false;
}
::hermes::vm::CallResult<::hermes::vm::Handle<::hermes::vm::JSArray>> napi::HermesExternalObject::getHostPropertyNames()
{
    return ::hermes::vm::Runtime::makeNullHandle<::hermes::vm::JSArray>();
}
