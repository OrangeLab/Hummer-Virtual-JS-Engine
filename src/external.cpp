#include "external.h"

::napi::External::External(NAPIEnv env, void *data, NAPIFinalize finalizeCallback,
                                                   void *finalizeHint)
    : env(env), data(data), finalizeCallback(finalizeCallback), finalizeHint(finalizeHint)
{
}

NAPIEnv napi::External::getEnv() const
{
    return env;
}
void *napi::External::getData() const
{
    return data;
}
NAPIFinalize napi::External::getFinalizeCallback() const
{
    return finalizeCallback;
}
void *napi::External::getFinalizeHint() const
{
    return finalizeHint;
}
napi::External::~External()
{
    if (finalizeCallback)
    {
        finalizeCallback(env, data, finalizeHint);
    }
}
::hermes::vm::CallResult<::hermes::vm::HermesValue> napi::External::get(::hermes::vm::SymbolID symbolId)
{
    return ::hermes::vm::Runtime::getUndefinedValue().get();
}
::hermes::vm::CallResult<bool> napi::External::set(::hermes::vm::SymbolID symbolId,
                                                               ::hermes::vm::HermesValue value)
{
    return false;
}
::hermes::vm::CallResult<::hermes::vm::Handle<::hermes::vm::JSArray>> napi::External::getHostPropertyNames()
{
    return ::hermes::vm::Runtime::makeNullHandle<::hermes::vm::JSArray>();
}
