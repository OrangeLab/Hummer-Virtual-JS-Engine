#include <hermes/BCGen/HBC/BytecodeProviderFromSrc.h>
#include <hermes/External.h>
#include <hermes/FunctionInfo.h>
#include <hermes/OpaqueNAPICallbackInfo.h>
#include <hermes/OpaqueNAPIEnv.h>
#include <hermes/OpaqueNAPIEscapableHandleScope.h>
#include <hermes/OpaqueNAPIHandleScope.h>
#include <hermes/OpaqueNAPIRef.h>
#include <hermes/VM/Callable.h>
#include <hermes/VM/HostModel.h>
#include <hermes/VM/JSArray.h>
#include <hermes/VM/Operations.h>
#include <hermes/VM/Runtime.h>
#include <hermes/VM/StringPrimitive.h>
#include <llvh/Support/ConvertUTF.h>
#include <napi/js_native_api_types.h>

#include <utility>

#define RETURN_STATUS_IF_FALSE(condition, status) \
  if (!(condition)) {                             \
    return status;                                \
  }

#define CHECK_ARG(arg, status)       \
  if (!arg) {                        \
    return NAPI##status##InvalidArg; \
  }

#define CHECK_NAPI(expr, exprStatus, returnStatus) \
  {                                                \
    NAPI##exprStatus##Status status = expr;        \
    if (status != NAPI##exprStatus##OK) {          \
      return (NAPI##returnStatus##Status)status;   \
    }                                              \
  }

#define CHECK_HERMES(expr)                                  \
  if ((expr) == ::hermes::vm::ExecutionStatus::EXCEPTION) { \
    return NAPIExceptionPendingException;                   \
  }

#define NAPI_PREAMBLE(env)                                             \
  CHECK_ARG(env, Exception)                                            \
  RETURN_STATUS_IF_FALSE(env->getRuntime().getThrownValue().isEmpty(), \
                         NAPIExceptionPendingException)

NAPICommonStatus napi_get_undefined(NAPIEnv /*env*/, NAPIValue *result) {
  CHECK_ARG(result, Common)

  *result = (NAPIValue)::hermes::vm::HandleRootOwner::getUndefinedValue()
                .unsafeGetPinnedHermesValue();

  return NAPICommonOK;
}

NAPICommonStatus napi_get_null(NAPIEnv /*env*/, NAPIValue *result) {
  CHECK_ARG(result, Common)

  *result = (NAPIValue)::hermes::vm::HandleRootOwner::getNullValue()
                .unsafeGetPinnedHermesValue();

  return NAPICommonOK;
}

NAPIErrorStatus napi_get_global(NAPIEnv env, NAPIValue *result) {
  CHECK_ARG(env, Error)
  CHECK_ARG(result, Error)

  *result =
      (NAPIValue)env->getRuntime().getGlobal().unsafeGetPinnedHermesValue();

  return NAPIErrorOK;
}

NAPIErrorStatus napi_get_boolean(NAPIEnv /*env*/, bool value,
                                 NAPIValue *result) {
  CHECK_ARG(result, Error)

  *result = (NAPIValue)::hermes::vm::HandleRootOwner::getBoolValue(value)
                .unsafeGetPinnedHermesValue();

  return NAPIErrorOK;
}

NAPIErrorStatus napi_create_double(NAPIEnv env, double value,
                                   NAPIValue *result) {
  CHECK_ARG(env, Error)
  CHECK_ARG(result, Error)

  // HermesValue.h -> encodeNumberValue
  *result = (NAPIValue)env->getRuntime()
                .makeHandle(::hermes::vm::HermesValue::encodeNumberValue(value))
                .unsafeGetPinnedHermesValue();

  return NAPIErrorOK;
}

NAPIExceptionStatus napi_create_string_utf8(NAPIEnv env, const char *str,
                                            NAPIValue *result) {
  NAPI_PREAMBLE(env)
  CHECK_ARG(result, Exception)

  auto callResult = ::hermes::vm::StringPrimitive::createEfficient(
      env->getRuntime(),
      ::llvh::makeArrayRef<uint8_t>(
          reinterpret_cast<const unsigned char *>(str), str ? strlen(str) : 0));
  CHECK_HERMES(callResult)
  *result = (NAPIValue)env->getRuntime()
                .makeHandle(callResult.getValue())
                .unsafeGetPinnedHermesValue();

  return NAPIExceptionOK;
}

NAPIExceptionStatus napi_create_function(NAPIEnv env, const char *utf8name,
                                         NAPICallback callback, void *data,
                                         NAPIValue *result) {
  NAPI_PREAMBLE(env)
  CHECK_ARG(callback, Exception)
  CHECK_ARG(result, Exception)

  ::hermes::vm::MutableHandle<::hermes::vm::HermesValue> mutableHandle(
      env->getRuntime());

  ::hermes::vm::GCScope gcScope(env->getRuntime());

  ::hermes::vm::StringPrimitive *stringPrimitive;
  {
    auto callResult = ::hermes::vm::StringPrimitive::createEfficient(
        env->getRuntime(),
        ::llvh::makeArrayRef<uint8_t>(
            reinterpret_cast<const unsigned char *>(utf8name),
            utf8name ? strlen(utf8name) : 0));
    CHECK_HERMES(callResult)

    stringPrimitive = ::hermes::vm::vmcast<::hermes::vm::StringPrimitive>(
        callResult.getValue());
  }
  RETURN_STATUS_IF_FALSE(stringPrimitive, NAPIExceptionMemoryError)
  auto callResult = ::hermes::vm::stringToSymbolID(
      env->getRuntime(), ::hermes::vm::createPseudoHandle(stringPrimitive));
  CHECK_HERMES(callResult)
  auto symbolId = callResult.getValue().get();

  auto functionInfo =
      new (::std::nothrow)::orangelabs::FunctionInfo(env, callback, data);
  RETURN_STATUS_IF_FALSE(functionInfo, NAPIExceptionMemoryError)

  ::hermes::vm::NativeFunctionPtr nativeFunctionPtr =
      [](void *context, ::hermes::vm::Runtime &runtime,
         ::hermes::vm::NativeArgs args)
      -> ::hermes::vm::CallResult<::hermes::vm::HermesValue> {
    ::hermes::vm::MutableHandle<::hermes::vm::HermesValue> inlineMutableHandle(
        runtime);

    ::hermes::vm::GCScope inlineGCScope(runtime);

    auto innerFunctionInfo = (::orangelabs::FunctionInfo *)context;
    if (!innerFunctionInfo->getCallback() || !innerFunctionInfo->getEnv()) {
      assert(false);

      return {::hermes::vm::HandleRootOwner::getUndefinedValue().get()};
    }
    struct OpaqueNAPICallbackInfo callbackInfo(::std::move(args),
                                               innerFunctionInfo->getData());
    NAPIValue returnValue = innerFunctionInfo->getCallback()(
        innerFunctionInfo->getEnv(), &callbackInfo);
    RETURN_STATUS_IF_FALSE(
        innerFunctionInfo->getEnv()->getRuntime().getThrownValue().isEmpty(),
        ::hermes::vm::ExecutionStatus::EXCEPTION)
    if (!returnValue) {
      return {::hermes::vm::HandleRootOwner::getUndefinedValue().get()};
    }

    inlineMutableHandle.set(
        *(const ::hermes::vm::PinnedHermesValue *)returnValue);

    return inlineMutableHandle.get();
  };
  ::hermes::vm::FinalizeNativeFunctionPtr finalizeNativeFunctionPtr =
      [](void *context) { delete (::orangelabs::FunctionInfo *)context; };
  auto functionCallResult =
      ::hermes::vm::FinalizableNativeFunction::createWithoutPrototype(
          env->getRuntime(), functionInfo, nativeFunctionPtr,
          finalizeNativeFunctionPtr, symbolId, 0);
  if (functionCallResult == ::hermes::vm::ExecutionStatus::EXCEPTION) {
    delete functionInfo;

    return NAPIExceptionPendingException;
  }
  mutableHandle.set(functionCallResult.getValue());
  *result = (NAPIValue)mutableHandle.unsafeGetPinnedHermesValue();

  return NAPIExceptionOK;
}

NAPICommonStatus napi_typeof(NAPIEnv /*env*/, NAPIValue value,
                             NAPIValueType *result) {
  CHECK_ARG(value, Common)
  CHECK_ARG(result, Common)

  ::hermes::vm::HermesValue hermesValue =
      *(const ::hermes::vm::PinnedHermesValue *)value;
  if (hermesValue.isUndefined()) {
    *result = NAPIUndefined;
  } else if (hermesValue.isNull()) {
    *result = NAPINull;
  } else if (hermesValue.isBool()) {
    *result = NAPIBoolean;
  } else if (hermesValue.isNumber()) {
    *result = NAPINumber;
  } else if (hermesValue.isString()) {
    *result = NAPIString;
  } else if (hermesValue.isObject()) {
    bool isFunction = ::hermes::vm::vmisa<::hermes::vm::Callable>(hermesValue);
    if (isFunction) {
      *result = NAPIFunction;
    } else {
      *result = ::hermes::vm::vmisa<::hermes::vm::HostObject>(hermesValue)
                    ? NAPIExternal
                    : NAPIObject;
    }
  } else {
    assert(false);

    return NAPICommonInvalidArg;
  }

  return NAPICommonOK;
}

NAPIErrorStatus napi_get_value_double(NAPIEnv /*env*/, NAPIValue value,
                                      double *result) {
  CHECK_ARG(value, Error)
  CHECK_ARG(result, Error)

  RETURN_STATUS_IF_FALSE(
      (*(const ::hermes::vm::PinnedHermesValue *)value).isNumber(),
      NAPIErrorNumberExpected)

  *result = (*(const ::hermes::vm::PinnedHermesValue *)value).getNumber();

  return NAPIErrorOK;
}

NAPIErrorStatus napi_get_value_bool(NAPIEnv /*env*/, NAPIValue value,
                                    bool *result) {
  CHECK_ARG(value, Error)
  CHECK_ARG(result, Error)

  RETURN_STATUS_IF_FALSE(
      ((const ::hermes::vm::PinnedHermesValue *)value)->isBool(),
      NAPIErrorBooleanExpected)
  *result = ((const ::hermes::vm::PinnedHermesValue *)value)->getBool();

  return NAPIErrorOK;
}

NAPIExceptionStatus napi_coerce_to_bool(NAPIEnv env, NAPIValue value,
                                        NAPIValue *result) {
  CHECK_ARG(env, Exception)
  CHECK_ARG(value, Exception)
  CHECK_ARG(result, Exception)

  *result = (NAPIValue)env->getRuntime()
                .makeHandle(::hermes::vm::HermesValue::encodeBoolValue(
                    ::hermes::vm::toBoolean(
                        *(const ::hermes::vm::PinnedHermesValue *)value)))
                .unsafeGetPinnedHermesValue();

  return NAPIExceptionOK;
}

NAPIExceptionStatus napi_coerce_to_number(NAPIEnv env, NAPIValue value,
                                          NAPIValue *result) {
  NAPI_PREAMBLE(env)
  CHECK_ARG(value, Exception)
  CHECK_ARG(result, Exception)

  auto callResult = ::hermes::vm::toNumber_RJS(
      env->getRuntime(), ::hermes::vm::Handle<::hermes::vm::HermesValue>(
                             (const ::hermes::vm::PinnedHermesValue *)value));
  CHECK_HERMES(callResult)
  *result = (NAPIValue)env->getRuntime()
                .makeHandle(callResult.getValue())
                .unsafeGetPinnedHermesValue();

  return NAPIExceptionOK;
}

NAPIExceptionStatus napi_coerce_to_string(NAPIEnv env, NAPIValue value,
                                          NAPIValue *result) {
  NAPI_PREAMBLE(env)
  CHECK_ARG(value, Exception)
  CHECK_ARG(result, Exception)

  auto callResult = ::hermes::vm::toString_RJS(
      env->getRuntime(), ::hermes::vm::Handle<::hermes::vm::HermesValue>(
                             (const ::hermes::vm::PinnedHermesValue *)value));
  CHECK_HERMES(callResult)
  *result = (NAPIValue)env->getRuntime()
                .makeHandle(callResult->getHermesValue())
                .unsafeGetPinnedHermesValue();

  return NAPIExceptionOK;
}

NAPIExceptionStatus napi_set_property(NAPIEnv env, NAPIValue object,
                                      NAPIValue key, NAPIValue value) {
  NAPI_PREAMBLE(env)
  CHECK_ARG(object, Exception)
  CHECK_ARG(key, Exception)
  CHECK_ARG(value, Exception)

  ::hermes::vm::GCScope gcScope(env->getRuntime());

  RETURN_STATUS_IF_FALSE(::hermes::vm::vmisa<::hermes::vm::JSObject>(
                             *(const ::hermes::vm::PinnedHermesValue *)object),
                         NAPIExceptionObjectExpected)
  auto callResult = ::hermes::vm::valueToSymbolID(
      env->getRuntime(), ::hermes::vm::Handle<::hermes::vm::HermesValue>(
                             (const ::hermes::vm::PinnedHermesValue *)key));
  CHECK_HERMES(callResult)
  auto setCallResult = ::hermes::vm::JSObject::putNamedOrIndexed(
      ::hermes::vm::Handle<::hermes::vm::JSObject>::vmcast(
          (const ::hermes::vm::PinnedHermesValue *)object),
      env->getRuntime(), callResult.getValue().get(),
      ::hermes::vm::Handle<::hermes::vm::HermesValue>(
          (const ::hermes::vm::PinnedHermesValue *)value));
  CHECK_HERMES(setCallResult)
  RETURN_STATUS_IF_FALSE(setCallResult.getValue(), NAPIExceptionGenericFailure)

  return NAPIExceptionOK;
}

NAPIExceptionStatus napi_has_property(NAPIEnv env, NAPIValue object,
                                      NAPIValue key, bool *result) {
  NAPI_PREAMBLE(env)
  CHECK_ARG(object, Exception)
  CHECK_ARG(key, Exception)
  CHECK_ARG(result, Exception)

  ::hermes::vm::GCScope gcScope(env->getRuntime());

  RETURN_STATUS_IF_FALSE(::hermes::vm::vmisa<::hermes::vm::JSObject>(
                             *(const ::hermes::vm::PinnedHermesValue *)object),
                         NAPIExceptionObjectExpected)
  auto callResult = ::hermes::vm::valueToSymbolID(
      env->getRuntime(), ::hermes::vm::Handle<::hermes::vm::HermesValue>(
                             (const ::hermes::vm::PinnedHermesValue *)key));
  CHECK_HERMES(callResult)
  auto hasCallResult = ::hermes::vm::JSObject::hasNamedOrIndexed(
      ::hermes::vm::Handle<::hermes::vm::JSObject>::vmcast(
          (const ::hermes::vm::PinnedHermesValue *)object),
      env->getRuntime(), callResult.getValue().get());
  CHECK_HERMES(hasCallResult)
  *result = hasCallResult.getValue();

  return NAPIExceptionOK;
}

NAPIExceptionStatus napi_get_property(NAPIEnv env, NAPIValue object,
                                      NAPIValue key, NAPIValue *result) {
  NAPI_PREAMBLE(env)
  CHECK_ARG(object, Exception)
  CHECK_ARG(key, Exception)
  CHECK_ARG(result, Exception)

  ::hermes::vm::MutableHandle<::hermes::vm::HermesValue> mutableHandle(
      env->getRuntime());
  ::hermes::vm::GCScope gcScope(env->getRuntime());

  RETURN_STATUS_IF_FALSE(::hermes::vm::vmisa<::hermes::vm::JSObject>(
                             *(const ::hermes::vm::PinnedHermesValue *)object),
                         NAPIExceptionObjectExpected)
  auto callResult = ::hermes::vm::valueToSymbolID(
      env->getRuntime(), ::hermes::vm::Handle<::hermes::vm::HermesValue>(
                             (const ::hermes::vm::PinnedHermesValue *)key));
  CHECK_HERMES(callResult)
  auto getCallResult = ::hermes::vm::JSObject::getNamedOrIndexed(
      ::hermes::vm::Handle<::hermes::vm::JSObject>::vmcast(
          (const ::hermes::vm::PinnedHermesValue *)object),
      env->getRuntime(), callResult.getValue().get());
  CHECK_HERMES(getCallResult)
  mutableHandle.set(getCallResult.getValue().get());
  *result = (NAPIValue)mutableHandle.unsafeGetPinnedHermesValue();

  return NAPIExceptionOK;
}

NAPIExceptionStatus napi_delete_property(NAPIEnv env, NAPIValue object,
                                         NAPIValue key, bool *result) {
  NAPI_PREAMBLE(env)
  CHECK_ARG(object, Exception)
  CHECK_ARG(key, Exception)

  RETURN_STATUS_IF_FALSE(::hermes::vm::vmisa<::hermes::vm::JSObject>(
                             *(const ::hermes::vm::PinnedHermesValue *)object),
                         NAPIExceptionObjectExpected)

  bool isSuccess;
  if (((const ::hermes::vm::PinnedHermesValue *)key)->isNumber()) {
    isSuccess = ::hermes::vm::JSObject::deleteOwnIndexed(
        ::hermes::vm::Handle<::hermes::vm::JSObject>::vmcast(
            (const ::hermes::vm::PinnedHermesValue *)object),
        env->getRuntime(),
        (uint32_t)((const ::hermes::vm::PinnedHermesValue *)key)->getDouble());
  } else {
    ::hermes::vm::GCScope gcScope(env->getRuntime());

    auto callResult = ::hermes::vm::valueToSymbolID(
        env->getRuntime(), ::hermes::vm::Handle<::hermes::vm::HermesValue>(
                               (const ::hermes::vm::PinnedHermesValue *)key));
    CHECK_HERMES(callResult)
    auto deleteCallResult = ::hermes::vm::JSObject::deleteNamed(
        ::hermes::vm::Handle<::hermes::vm::JSObject>::vmcast(
            (const ::hermes::vm::PinnedHermesValue *)object),
        env->getRuntime(), callResult.getValue().get());
    CHECK_HERMES(deleteCallResult)
    isSuccess = deleteCallResult.getValue();
  }
  if (result) {
    *result = isSuccess;
  }

  return NAPIExceptionOK;
}

NAPICommonStatus napi_is_array(NAPIEnv /*env*/, NAPIValue value, bool *result) {
  CHECK_ARG(value, Common)
  CHECK_ARG(result, Common)

  *result = ::hermes::vm::vmisa<::hermes::vm::JSArray>(
      *(const ::hermes::vm::PinnedHermesValue *)value);

  return NAPICommonOK;
}

NAPIExceptionStatus napi_call_function(NAPIEnv env, NAPIValue thisValue,
                                       NAPIValue func, size_t argc,
                                       const NAPIValue *argv,
                                       NAPIValue *result) {
  NAPI_PREAMBLE(env)
  CHECK_ARG(func, Exception)
  if (argc) {
    CHECK_ARG(argv, Exception)
  }

  RETURN_STATUS_IF_FALSE(::hermes::vm::vmisa<::hermes::vm::Callable>(
                             *(const ::hermes::vm::PinnedHermesValue *)func),
                         NAPIExceptionFunctionExpected)

  ::hermes::vm::MutableHandle<::hermes::vm::HermesValue> mutableHandle(
      env->getRuntime());
  ::hermes::vm::GCScope gcScope(env->getRuntime());

  if (!thisValue) {
    thisValue =
        (NAPIValue)env->getRuntime().getGlobal().unsafeGetPinnedHermesValue();
  }

  auto callResult = ::hermes::vm::Arguments::create(
      env->getRuntime(), argc,
      ::hermes::vm::HandleRootOwner::makeNullHandle<::hermes::vm::Callable>(),
      true);
  CHECK_HERMES(callResult)
  for (size_t i = 0; i < argc; ++i) {
    ::hermes::vm::ArrayImpl::setElementAt(
        callResult.getValue(), env->getRuntime(), i,
        ::hermes::vm::Handle<::hermes::vm::HermesValue>(
            (const ::hermes::vm::PinnedHermesValue *)argv[i]));
  }
  auto executeCallResult = ::hermes::vm::Callable::executeCall(
      ::hermes::vm::Handle<::hermes::vm::Callable>::vmcast(
          (const ::hermes::vm::PinnedHermesValue *)func),
      env->getRuntime(), ::hermes::vm::HandleRootOwner::getUndefinedValue(),
      ::hermes::vm::Handle<::hermes::vm::HermesValue>(
          (const ::hermes::vm::PinnedHermesValue *)thisValue),
      callResult.getValue());
  CHECK_HERMES(executeCallResult)
  if (result) {
    mutableHandle.set(executeCallResult.getValue().get());
    *result = (NAPIValue)mutableHandle.unsafeGetPinnedHermesValue();
  }

  return NAPIExceptionOK;
}

NAPIExceptionStatus napi_new_instance(NAPIEnv env, NAPIValue constructor,
                                      size_t argc, const NAPIValue *argv,
                                      NAPIValue *result) {
  NAPI_PREAMBLE(env)
  CHECK_ARG(constructor, Exception)
  if (argc) {
    CHECK_ARG(argv, Exception)
  }
  CHECK_ARG(result, Exception)

  {
    auto callResult = ::hermes::vm::isConstructor(
        env->getRuntime(),
        *(const ::hermes::vm::PinnedHermesValue *)constructor);
    CHECK_HERMES(callResult)
    RETURN_STATUS_IF_FALSE(callResult.getValue(), NAPIExceptionFunctionExpected)
  }

  ::hermes::vm::MutableHandle<::hermes::vm::HermesValue> mutableHandle(
      env->getRuntime());
  ::hermes::vm::GCScope gcScope(env->getRuntime());

  auto callResult = ::hermes::vm::Arguments::create(
      env->getRuntime(), argc,
      ::hermes::vm::HandleRootOwner::makeNullHandle<::hermes::vm::Callable>(),
      true);
  CHECK_HERMES(callResult)
  for (size_t i = 0; i < argc; ++i) {
    ::hermes::vm::ArrayImpl::setElementAt(
        callResult.getValue(), env->getRuntime(), i,
        ::hermes::vm::Handle<::hermes::vm::HermesValue>(
            (const ::hermes::vm::PinnedHermesValue *)argv[i]));
  }
  auto functionHandle = ::hermes::vm::Handle<::hermes::vm::Callable>::vmcast(
      (const ::hermes::vm::PinnedHermesValue *)constructor);
  auto createThisCallResult = ::hermes::vm::Callable::createThisForConstruct(
      functionHandle, env->getRuntime());
  CHECK_HERMES(createThisCallResult)
  auto thisHandle = env->getRuntime().makeHandle(
      createThisCallResult.getValue().getHermesValue());
  auto executeCallResult = ::hermes::vm::Callable::executeCall(
      functionHandle, env->getRuntime(), functionHandle, thisHandle,
      callResult.getValue());
  CHECK_HERMES(executeCallResult)
  if (executeCallResult.getValue()->isObject()) {
    mutableHandle.set(executeCallResult.getValue().get());
    *result = (NAPIValue)mutableHandle.unsafeGetPinnedHermesValue();
  } else {
    mutableHandle.set(thisHandle.get());
    *result = (NAPIValue)mutableHandle.unsafeGetPinnedHermesValue();
  }

  return NAPIExceptionOK;
}

NAPIExceptionStatus napi_instanceof(NAPIEnv env, NAPIValue object,
                                    NAPIValue constructor, bool *result) {
  NAPI_PREAMBLE(env)
  CHECK_ARG(object, Exception)
  CHECK_ARG(constructor, Exception)
  CHECK_ARG(result, Exception)

  auto callResult = ::hermes::vm::instanceOfOperator_RJS(
      env->getRuntime(),
      ::hermes::vm::Handle<::hermes::vm::HermesValue>(
          (const ::hermes::vm::PinnedHermesValue *)object),
      ::hermes::vm::Handle<::hermes::vm::HermesValue>(
          (const ::hermes::vm::PinnedHermesValue *)constructor));
  CHECK_HERMES(callResult)
  *result = callResult.getValue();

  return NAPIExceptionOK;
}

NAPICommonStatus napi_get_cb_info(NAPIEnv env, NAPICallbackInfo callbackInfo,
                                  size_t *argc, NAPIValue *argv,
                                  NAPIValue *thisArg, void **data) {
  CHECK_ARG(env, Common)
  CHECK_ARG(callbackInfo, Common)

  if (argv) {
    CHECK_ARG(argc, Common)
    unsigned int i = 0;
    size_t min = ::std::min(callbackInfo->getNativeArgs().getArgCount(),
                            (unsigned int)*argc);
    for (; i < min; ++i) {
      argv[i] = (NAPIValue)callbackInfo->getNativeArgs()
                    .getArgHandle(i)
                    .unsafeGetPinnedHermesValue();
    }
    if (i < *argc) {
      NAPIValue undefined =
          (NAPIValue)::hermes::vm::HandleRootOwner::getUndefinedValue()
              .unsafeGetPinnedHermesValue();
      for (; i < *argc; ++i) {
        argv[i] = undefined;
      }
    }
  }
  if (argc) {
    *argc = callbackInfo->getNativeArgs().getArgCount();
  }
  if (thisArg) {
    *thisArg = (NAPIValue)callbackInfo->getNativeArgs()
                   .getThisHandle()
                   .unsafeGetPinnedHermesValue();
  }
  if (data) {
    *data = callbackInfo->getData();
  }

  return NAPICommonOK;
}

NAPICommonStatus napi_get_new_target(NAPIEnv env, NAPICallbackInfo callbackInfo,
                                     NAPIValue *result) {
  CHECK_ARG(env, Common)
  CHECK_ARG(callbackInfo, Common)
  CHECK_ARG(result, Common)

  if (callbackInfo->getNativeArgs().getNewTarget().isUndefined()) {
    *result = nullptr;
  } else {
    *result = (NAPIValue)callbackInfo->getNativeArgs()
                  .getNewTargetHandle()
                  .unsafeGetPinnedHermesValue();
  }

  return NAPICommonOK;
}

NAPIExceptionStatus napi_create_external(NAPIEnv env, void *data,
                                         NAPIFinalize finalizeCB,
                                         void *finalizeHint,
                                         NAPIValue *result) {
  NAPI_PREAMBLE(env)
  CHECK_ARG(result, Exception)

  auto hermesExternalObject = new (::std::nothrow)::orangelabs::External(
      env->getRuntime(), data, finalizeCB, finalizeHint);
  RETURN_STATUS_IF_FALSE(hermesExternalObject, NAPIExceptionMemoryError)

  auto callResult = ::hermes::vm::HostObject::createWithoutPrototype(
      env->getRuntime(),
      std::unique_ptr<::orangelabs::External>(hermesExternalObject));
  CHECK_HERMES(callResult)
  *result = (NAPIValue)env->getRuntime()
                .makeHandle(callResult.getValue())
                .unsafeGetPinnedHermesValue();

  return NAPIExceptionOK;
}

NAPIErrorStatus napi_get_value_external(NAPIEnv env, NAPIValue value,
                                        void **result) {
  CHECK_ARG(env, Error)
  CHECK_ARG(value, Error)
  CHECK_ARG(result, Error)

  RETURN_STATUS_IF_FALSE(::hermes::vm::vmisa<::hermes::vm::HostObject>(
                             *(const ::hermes::vm::PinnedHermesValue *)value),
                         NAPIErrorExternalExpected)

  auto external = reinterpret_cast<::orangelabs::External *>(
      ::hermes::vm::vmcast<::hermes::vm::HostObject>(
          *(const ::hermes::vm::PinnedHermesValue *)value)
          ->getProxy());
  *result = external ? external->getData() : nullptr;

  return NAPIErrorOK;
}

NAPIExceptionStatus napi_create_reference(NAPIEnv env, NAPIValue value,
                                          uint32_t initialRefCount,
                                          NAPIRef *result) {
  CHECK_ARG(env, Exception)
  CHECK_ARG(value, Exception)
  CHECK_ARG(result, Exception)

  *result = new (::std::nothrow) struct OpaqueNAPIRef(
      env, *(const ::hermes::vm::PinnedHermesValue *)value, initialRefCount);
  RETURN_STATUS_IF_FALSE(*result, NAPIExceptionMemoryError)

  return NAPIExceptionOK;
}

NAPIExceptionStatus napi_delete_reference(NAPIEnv env, NAPIRef ref) {
  CHECK_ARG(env, Exception)
  CHECK_ARG(ref, Exception)

  delete ref;

  return NAPIExceptionOK;
}

NAPIExceptionStatus napi_reference_ref(NAPIEnv env, NAPIRef ref,
                                       uint32_t *result) {
  CHECK_ARG(env, Exception)
  CHECK_ARG(ref, Exception)

  ref->ref();
  if (result) {
    *result = ref->getReferenceCount();
  }

  return NAPIExceptionOK;
}

NAPIExceptionStatus napi_reference_unref(NAPIEnv env, NAPIRef ref,
                                         uint32_t *result) {
  CHECK_ARG(env, Exception)
  CHECK_ARG(ref, Exception)

  RETURN_STATUS_IF_FALSE(ref->getReferenceCount(), NAPIExceptionGenericFailure)
  ref->unref();
  if (result) {
    *result = ref->getReferenceCount();
  }

  return NAPIExceptionOK;
}

NAPIErrorStatus napi_get_reference_value(NAPIEnv env, NAPIRef ref,
                                         NAPIValue *result) {
  CHECK_ARG(env, Error)
  CHECK_ARG(ref, Error)
  CHECK_ARG(result, Error)

  *result = (NAPIValue)ref->getHermesValue();

  return NAPIErrorOK;
}

NAPIErrorStatus napi_open_handle_scope(NAPIEnv env, NAPIHandleScope *result) {
  CHECK_ARG(env, Error)
  CHECK_ARG(result, Error)

  *result =
      new (::std::nothrow) struct OpaqueNAPIHandleScope(env->getRuntime());
  RETURN_STATUS_IF_FALSE(*result, NAPIErrorMemoryError)
  SLIST_INSERT_HEAD(&env->handleScopeList, *result, node);

  return NAPIErrorOK;
}

NAPIErrorStatus napi_close_handle_scope(NAPIEnv env, NAPIHandleScope scope) {
  CHECK_ARG(env, Error)
  CHECK_ARG(scope, Error)

  RETURN_STATUS_IF_FALSE(SLIST_FIRST(&env->handleScopeList) == scope,
                         NAPIErrorHandleScopeMismatch)
  SLIST_REMOVE_HEAD(&env->handleScopeList, node);
  delete scope;

  return NAPIErrorOK;
}

NAPIErrorStatus napi_open_escapable_handle_scope(
    NAPIEnv env, NAPIEscapableHandleScope *result) {
  CHECK_ARG(env, Error)
  CHECK_ARG(result, Error)

  ::hermes::vm::MutableHandle<::hermes::vm::HermesValue> mutableHandle(
      env->getRuntime());

  *result = new (::std::nothrow) struct OpaqueNAPIEscapableHandleScope(
      env->getRuntime(), ::std::move(mutableHandle));
  RETURN_STATUS_IF_FALSE(*result, NAPIErrorMemoryError)
  SLIST_INSERT_HEAD(&env->handleScopeList, *result, node);

  return NAPIErrorOK;
}

NAPIErrorStatus napi_close_escapable_handle_scope(
    NAPIEnv env, NAPIEscapableHandleScope scope) {
  return napi_close_handle_scope(env, scope);
}

NAPIErrorStatus napi_escape_handle(NAPIEnv env, NAPIEscapableHandleScope scope,
                                   NAPIValue escapee, NAPIValue *result) {
  CHECK_ARG(env, Error)
  CHECK_ARG(scope, Error)
  CHECK_ARG(escapee, Error)
  CHECK_ARG(result, Error)

  auto escapedValue =
      scope->escape(*(const ::hermes::vm::PinnedHermesValue *)escapee);
  RETURN_STATUS_IF_FALSE(!escapedValue, NAPIErrorEscapeCalledTwice)
  *result = (NAPIValue)escapedValue;

  return NAPIErrorOK;
}

NAPIExceptionStatus napi_throw(NAPIEnv env, NAPIValue error) {
  NAPI_PREAMBLE(env)
  CHECK_ARG(error, Exception)

  // 直接忽略返回值
  env->getRuntime().setThrownValue(
      *(const ::hermes::vm::PinnedHermesValue *)error);

  return NAPIExceptionOK;
}

NAPIErrorStatus napi_get_and_clear_last_exception(NAPIEnv env,
                                                  NAPIValue *result) {
  CHECK_ARG(env, Error)
  CHECK_ARG(result, Error)

  if (env->getRuntime().getThrownValue().isEmpty()) {
    *result = (NAPIValue)::hermes::vm::HandleRootOwner::getUndefinedValue()
                  .unsafeGetPinnedHermesValue();
  } else {
    *result = (NAPIValue)env->getRuntime()
                  .makeHandle(env->getRuntime().getThrownValue())
                  .unsafeGetPinnedHermesValue();
    env->getRuntime().clearThrownValue();
  }

  return NAPIErrorOK;
}

NAPICommonStatus NAPIClearLastException(NAPIEnv env) {
  CHECK_ARG(env, Common)

  env->getRuntime().clearThrownValue();

  return NAPICommonOK;
}

NAPIExceptionStatus NAPIRunScript(NAPIEnv env, const char *script,
                                  const char *sourceUrl, NAPIValue *result) {
  NAPI_PREAMBLE(env)

  ::hermes::hbc::CompileFlags compileFlags = {};
  compileFlags.lazy = true;
  compileFlags.debug = true;
  auto callResult = env->getRuntime().run(script, sourceUrl, compileFlags);
  CHECK_HERMES(callResult)
  if (result) {
    *result = (NAPIValue)env->getRuntime()
                  .makeHandle(callResult.getValue())
                  .unsafeGetPinnedHermesValue();
  }

  return NAPIExceptionOK;
}

NAPIExceptionStatus NAPIDefineClass(NAPIEnv env, const char *utf8name,
                                    NAPICallback constructor, void *data,
                                    NAPIValue *result) {
  NAPI_PREAMBLE(env)
  CHECK_ARG(constructor, Exception)
  CHECK_ARG(result, Exception)

  ::hermes::vm::MutableHandle<::hermes::vm::HermesValue> mutableHandle(
      env->getRuntime());
  ::hermes::vm::GCScope gcScope(env->getRuntime());

  auto functionInfo =
      new (::std::nothrow)::orangelabs::FunctionInfo(env, constructor, data);
  RETURN_STATUS_IF_FALSE(functionInfo, NAPIExceptionMemoryError)
  NAPIValue externalValue;
  auto finalizeCallback = [](void *finalizeData, void * /*finalizeHint*/) {
    delete (::orangelabs::FunctionInfo *)finalizeData;
  };
  auto createExternalStatus = napi_create_external(
      env, functionInfo, finalizeCallback, nullptr, &externalValue);
  if (createExternalStatus != NAPIExceptionOK) {
    delete functionInfo;

    return createExternalStatus;
  }

  ::hermes::vm::NativeFunctionPtr nativeFunctionPtr =
      [](void *context, ::hermes::vm::Runtime &runtime,
         ::hermes::vm::NativeArgs args)
      -> ::hermes::vm::CallResult<::hermes::vm::HermesValue> {
    ::hermes::vm::MutableHandle<::hermes::vm::HermesValue> inlineMutableHandle(
        runtime);
    ::hermes::vm::GCScope inlineGCScope(runtime);

    auto innerFunctionInfo = (::orangelabs::FunctionInfo *)context;
    if (!innerFunctionInfo->getCallback() || !innerFunctionInfo->getEnv()) {
      assert(false);

      return args.getThisArg();
    }
    struct OpaqueNAPICallbackInfo callbackInfo(args,
                                               innerFunctionInfo->getData());
    NAPIValue returnValue = innerFunctionInfo->getCallback()(
        innerFunctionInfo->getEnv(), &callbackInfo);
    RETURN_STATUS_IF_FALSE(
        innerFunctionInfo->getEnv()->getRuntime().getThrownValue().isEmpty(),
        ::hermes::vm::ExecutionStatus::EXCEPTION)
    if (!returnValue ||
        !((const ::hermes::vm::PinnedHermesValue *)returnValue)->isObject()) {
      return args.getThisArg();
    }
    inlineMutableHandle.set(
        *(const ::hermes::vm::PinnedHermesValue *)returnValue);

    return inlineMutableHandle.get();
  };
  auto nativeConstructor = ::hermes::vm::NativeConstructor::create(
      env->getRuntime(),
      ::hermes::vm::Handle<::hermes::vm::JSObject>::vmcast(
          &env->getRuntime().functionPrototype),
      functionInfo, nativeFunctionPtr, 0,
      ::hermes::vm::NativeConstructor::creatorFunction<::hermes::vm::JSObject>,
      ::hermes::vm::CellKind::JSObjectKind);

  ::hermes::vm::StringPrimitive *stringPrimitive;
  {
    auto callResult = ::hermes::vm::StringPrimitive::createEfficient(
        env->getRuntime(),
        ::llvh::makeArrayRef<uint8_t>(
            reinterpret_cast<const unsigned char *>(utf8name),
            utf8name ? strlen(utf8name) : 0));
    CHECK_HERMES(callResult)
    stringPrimitive = ::hermes::vm::vmcast<::hermes::vm::StringPrimitive>(
        callResult.getValue());
  }
  auto callResult = ::hermes::vm::stringToSymbolID(
      env->getRuntime(), ::hermes::vm::createPseudoHandle(stringPrimitive));
  CHECK_HERMES(callResult)
  auto symbolId = callResult.getValue().get();

  auto rawObject = ::hermes::vm::JSObject::create(env->getRuntime());

  auto defineCallResult = ::hermes::vm::Callable::defineNameLengthAndPrototype(
      env->getRuntime().makeHandle(nativeConstructor.get()), env->getRuntime(),
      symbolId, 0, env->getRuntime().makeHandle(rawObject.get()),
      ::hermes::vm::Callable::WritablePrototype::No, true);
  CHECK_HERMES(defineCallResult)
  mutableHandle.set(nativeConstructor.getHermesValue());
  *result = (NAPIValue)mutableHandle.unsafeGetPinnedHermesValue();
  NAPIValue privateKeyString;
  CHECK_NAPI(napi_create_string_utf8(env, "__constructor__", &privateKeyString),
             Exception, Exception)
  CHECK_NAPI(napi_set_property(env, *result, privateKeyString, externalValue),
             Exception, Exception)

  return NAPIExceptionOK;
}

NAPIErrorStatus NAPICreateRuntime(NAPIRuntime *) { return NAPIErrorOK; }

NAPICommonStatus NAPIFreeRuntime(NAPIRuntime) { return NAPICommonOK; }

NAPIErrorStatus NAPICreateEnv(NAPIEnv *env, NAPIRuntime) {
  CHECK_ARG(env, Error)

  *env = new (::std::nothrow) OpaqueNAPIEnv(::hermes::vm::RuntimeConfig());
  RETURN_STATUS_IF_FALSE(*env, NAPIErrorMemoryError)

  return NAPIErrorOK;
}

// NAPICommonStatus NAPIEnableDebugger(NAPIEnv env, const char *debuggerTitle,
//                                     bool waitForDebugger) {
//   CHECK_ARG(env, Common)
//
//   env->enableDebugger(debuggerTitle, waitForDebugger);
//
//   return NAPICommonOK;
// }
//
// NAPICommonStatus NAPIDisableDebugger(NAPIEnv env) {
//   CHECK_ARG(env, Common)
//
//   env->disableDebugger();
//
//   return NAPICommonOK;
// }

// NAPICommonStatus NAPISetMessageQueueThread(
//     NAPIEnv env, MessageQueueThreadWrapper jsQueueWrapper) {
// #ifdef HERMES_ENABLE_DEBUGGER
//   CHECK_ARG(env, Common);
//   env->setMessageQueueThread(jsQueueWrapper->thread_);
// #endif
//   return NAPICommonOK;
// }

NAPICommonStatus NAPIFreeEnv(NAPIEnv env) {
  CHECK_ARG(env, Common)

  delete env;

  return NAPICommonOK;
}

NAPIErrorStatus NAPIGetValueStringUTF8(NAPIEnv env, NAPIValue value,
                                       const char **result) {
  CHECK_ARG(env, Error)
  CHECK_ARG(value, Error)
  CHECK_ARG(result, Error)

  RETURN_STATUS_IF_FALSE(::hermes::vm::vmisa<::hermes::vm::StringPrimitive>(
                             *(const ::hermes::vm::PinnedHermesValue *)value),
                         NAPIErrorStringExpected)

  auto stringPrimitive = ::hermes::vm::vmcast<::hermes::vm::StringPrimitive>(
      *(const ::hermes::vm::PinnedHermesValue *)value);
  if (stringPrimitive->isASCII()) {
    auto asciiStringRef = stringPrimitive->getStringRef<char>();
    char *buffer =
        static_cast<char *>(malloc(sizeof(char) * (asciiStringRef.size() + 1)));
    RETURN_STATUS_IF_FALSE(buffer, NAPIErrorMemoryError)
    ::std::memmove(buffer, asciiStringRef.data(), asciiStringRef.size());
    buffer[asciiStringRef.size()] = '\0';
    *result = buffer;
  } else {
    auto utf16StringRef = stringPrimitive->getStringRef<char16_t>();
    auto length = utf16StringRef.size() * 3 + 1;
    char *buffer = static_cast<char *>(malloc(sizeof(char) * length));
    RETURN_STATUS_IF_FALSE(buffer, NAPIErrorMemoryError)
    auto sourceStart = utf16StringRef.begin();
    char *targetStart = buffer;
    auto conversionResult = ::llvh::ConvertUTF16toUTF8(
        reinterpret_cast<const ::llvh::UTF16 **>(&sourceStart),
        reinterpret_cast<const ::llvh::UTF16 *>(utf16StringRef.end()),
        reinterpret_cast<::llvh::UTF8 **>(&targetStart),
        reinterpret_cast<::llvh::UTF8 *>(buffer + length - 1),
        ::llvh::strictConversion);
    if (conversionResult == ::llvh::ConversionResult::targetExhausted) {
      assert(false);
      free(buffer);

      return NAPIErrorMemoryError;
    } else if (conversionResult != ::llvh::ConversionResult::conversionOK) {
      free(buffer);

      return NAPIErrorMemoryError;
    }
    *targetStart = '\0';
    *result = buffer;
  }

  return NAPIErrorOK;
}

NAPICommonStatus NAPIFreeUTF8String(NAPIEnv env, const char *cString) {
  CHECK_ARG(env, Common)
  CHECK_ARG(cString, Common)

  free((void *)cString);

  return NAPICommonOK;
}

NAPI_EXPORT NAPIExceptionStatus NAPICompileToByteBuffer(NAPIEnv, const char *,
                                                        const char *,
                                                        const uint8_t **,
                                                        size_t *) {
  return NAPIExceptionOK;
}

NAPI_EXPORT NAPICommonStatus NAPIFreeByteBuffer(NAPIEnv, const uint8_t *) {
  return NAPICommonOK;
}

NAPI_EXPORT NAPIExceptionStatus NAPIRunByteBuffer(NAPIEnv, const uint8_t *,
                                                  size_t, NAPIValue *) {
  return NAPIExceptionOK;
}
