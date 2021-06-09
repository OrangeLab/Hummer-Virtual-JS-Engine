#include <napi/js_native_api_types.h>
#include <napi/js_native_api.h>
#include <hermes/hermes.h>
#include <hermes/DebuggerAPI.h>
#include <hermes/VM/JSObject.h>
#include <hermes/VM/JSArray.h>
#include <hermes/VM/Runtime.h>
#include <hermes/VM/Operations.h>
#include <hermes/VM/JSError.h>
#include <hermes/VM/Callable.h>
#include <hermes/VM/HandleRootOwner.h>
#include <hermes/VM/HermesValue.h>
#include <llvh/Support/ConvertUTF.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <uthash.h>

// 如果一个函数返回的 NAPIStatus 为 NAPIOK，则没有异常被挂起，并且不需要做任何处理
// 如果 NAPIStatus 是除了 NAPIOK 或者 NAPIPendingException，为了尝试恢复并继续执行，而不是直接返回，必须调用 napi_is_exception_pending 来确定是否存在挂起的异常

// 大多数情况下，调用 N-API 函数的时候，如果已经有异常被挂起，函数会立即返回 NAPIPendingException，但是，N-API 允许一些函数被调用用来在返回 JavaScript 环境前做最小化的清理
// 在这种情况下，NAPIStatus 会反映函数的执行结果状态，而不是当前存在异常，为了避免混淆，在每次函数执行后请检查结果

// 当异常被挂起的时候，有两种方法
// 1. 进行适当的清理并返回，那么执行环境会返回到 JavaScript，作为返回到 JavaScript 的过渡的一部分，异常将在 JavaScript 调用原生方法的语句处抛出，大多数 N-API 函数在出现异常被挂起的时候，都只是简单的返回 NAPIPendingException，，所以在这种情况下，建议尽量少做事情，等待返回到 JavaScript 中处理异常
// 2. 尝试处理异常，在原生代码能捕获到异常的地方采取适当的操作，然后继续执行。只有当异常能被安全的处理的少数特定情况下推荐这样做，在这些情况下，napi_get_and_clear_last_exception 可以被用来获取并清除异常，在此之后，如果发现异常还是无法处理，可以通过 napi_throw 重新抛出错误


#include <sys/queue.h>


// setLastErrorCode 会处理 env == NULL 问题
#define RETURN_STATUS_IF_FALSE(condition, status) \
    do                                                 \
    {                                                  \
        if (!(condition))                              \
        {                                              \
            return setLastErrorCode(env, status);  \
        }                                              \
    } while (0)

#define CHECK_ARG(env, arg) RETURN_STATUS_IF_FALSE(arg, NAPIInvalidArg)

// This does not call napi_set_last_error because the expression
// is assumed to be a NAPI function call that already did.
#define CHECK_NAPI(expr)            \
    do                              \
    {                               \
        NAPIStatus status = expr; \
        if (status != NAPIOK)       \
        {                           \
            return status;          \
        }                           \
    } while (0)

// clearLastError() 函数会调用 CHECK_ENV 检查 env 变量，所以不能在 CHECK_ENV 中调用 clearLastError(env)
#define CHECK_ENV(env)             \
    do                             \
    {                              \
        if (!(env))         \
        {                          \
            return setLastErrorCode(env, NAPIInvalidArg); \
        }                          \
    } while (0)

#define CHECK_HERMES(env)                                                             \
    do                                                                             \
    {                                                                              \
        CHECK_ENV(env);                                                            \
        RETURN_STATUS_IF_FALSE(!((env)->lastException.isEmpty()), NAPIPendingException); \
    } while (0)

#define NAPI_PREAMBLE(env)                                                                     \
    do                                                                                         \
    {                                                                                          \
        CHECK_HERMES(env); \
        clearLastError(env);                                                                 \
    } while (0)



// #define HASH_NONFATAL_OOM 1

// 先 hashError = false; 再判断是否变为 true，发生错误需要回滚内存分配
static bool hashError = false;

#define uthash_nonfatal_oom(elt) hashError = true;



#include <uthash.h>

struct OpaqueNAPIRef
{
    LIST_ENTRY(OpaqueNAPIRef) node;
    NAPIValue value;
    uint32_t count;
};

struct OpaqueNAPIEnv
{
  public:
    explicit OpaqueNAPIEnv() {  }
    virtual ~OpaqueNAPIEnv() {
    }
    hermes::vm::Runtime *context;
    // 默认为 HermesValue::encodeEmptyValue();
    hermes::vm::PinnedHermesValue lastException;
    NAPIExtendedErrorInfo lastError;
    int open_handle_scopes = 0;
    LIST_HEAD(, OpaqueNAPIRef) strongReferenceHead;
};



namespace hermesImpl {




// c++ RVO
class HandleScopeWrapper {
 public:
  explicit HandleScopeWrapper(hermes::vm::HandleRootOwner *runtime) : gcScope(runtime) {}

 private:
    hermes::vm::GCScope gcScope;
};

/*
 * C++'s idea of a reinterpret_cast lacks sufficient cojones.
 */
template<typename ToType, typename FromType>
inline ToType hermes_bitwise_cast(FromType from)
{
    static_assert(sizeof(FromType) == sizeof(ToType), "bitwise_cast size of FromType and ToType must be equal!");
    typename std::remove_const<ToType>::type to { };
    std::memcpy(static_cast<void*>(&to), static_cast<void*>(&from), sizeof(to));
    return to;
}

inline NAPIValue JsValueFromHermesHandle(hermes::vm::Handle<hermes::vm::HermesValue> handle) {
    return hermes_bitwise_cast<NAPIValue>(handle.get());
}

inline NAPIValue JsValueFromHermesValue(hermes::vm::HermesValue value) {
    return hermes_bitwise_cast<NAPIValue>(value);
}

inline hermes::vm::HermesValue HermesValueFromJsValue(NAPIValue value) {
    return hermes_bitwise_cast<hermes::vm::HermesValue>(value);
}

inline NAPIValue JsValueFromPseudoHermesHandle(hermes::vm::PseudoHandle<hermes::vm::HermesValue> handle) {
    return hermes_bitwise_cast<NAPIValue>(handle.get());
}

inline static NAPIHandleScope JsHandleScopeFromHermesScope(HandleScopeWrapper *scope) {
    return reinterpret_cast<NAPIHandleScope>(scope);
}

inline static HandleScopeWrapper* HermesScopeFormeJsHandleScope(NAPIHandleScope scope) {
    return reinterpret_cast<HandleScopeWrapper *>(scope);
}


inline static bool getException(hermes::vm::ExecutionStatus status, NAPIEnv env){
    if (status == hermes::vm::ExecutionStatus::RETURNED) {
        return true;
    }else{
        env->lastException = env->context->getThrownValue();
        return false;
    }
};
static void
convertUtf8ToUtf16(const uint8_t *utf8, size_t length, std::u16string &out) {
    // length is the number of input bytes
    out.resize(length);
    const llvh::UTF8 *sourceStart = (const llvh::UTF8 *)utf8;
    const llvh::UTF8 *sourceEnd = sourceStart + length;
    llvh::UTF16 *targetStart = (llvh::UTF16 *)&out[0];
    llvh::UTF16 *targetEnd = targetStart + out.size();
    llvh::ConversionResult cRes;
    cRes = ConvertUTF8toUTF16(
        &sourceStart,
        sourceEnd,
        &targetStart,
        targetEnd,
        llvh::lenientConversion);
    (void)cRes;
    assert(
        cRes != llvh::ConversionResult::targetExhausted &&
        "not enough space allocated for UTF16 conversion");
    out.resize((char16_t *)targetStart - &out[0]);
}

}
#pragma mark <Private>
// 以下函数需要 外置 scope
static inline NAPIStatus setLastErrorCode(NAPIEnv env, NAPIStatus errorCode)
{
    CHECK_ENV(env);
    env->lastError.errorCode = errorCode;

    return errorCode;
}

static inline NAPIStatus clearLastError(NAPIEnv env)
{
    // env 空指针检查
    CHECK_ENV(env);
    env->lastError.errorCode = NAPIOK;

    // TODO(boingoing): Should this be a callback?
    env->lastError.engineErrorCode = 0;
    env->lastError.engineReserved = NULL;

    return NAPIOK;
}

static inline NAPIStatus setErrorCode(NAPIEnv env, NAPIValue error, NAPIValue code)
{
    // 应当允许 code 为 undefined ？？？
    // 这是 Node.js 单元测试的要求
    if (!code)
    {
        return clearLastError(env);
    }
    CHECK_ENV(env);

    auto codeHermesValue = hermesImpl::HermesValueFromJsValue(code);
    if (codeHermesValue.isUndefined())
    {
        return clearLastError(env);
    }
    RETURN_STATUS_IF_FALSE(codeHermesValue.isString(), NAPIStringExpected);
    CHECK_NAPI(napi_set_named_property(env, error, "code", code));
    return clearLastError(env);
}

#pragma mark <Error Handling>

NAPIStatus napi_throw(NAPIEnv env, NAPIValue error){

    CHECK_ENV(env);
    CHECK_ARG(env, error);
    env->lastException = hermesImpl::HermesValueFromJsValue(error);
    return clearLastError(env);
}

NAPIStatus napi_throw_error(NAPIEnv env, const char *code, const char *msg){

    CHECK_ENV(env);
    std::u16string code16;
    std::u16string msg16;
    hermes::vm::GCScope gcScope(reinterpret_cast<hermes::vm::HandleRootOwner *>(env->context));
    {
        hermesImpl::convertUtf8ToUtf16(reinterpret_cast<const uint8_t *>(code), strlen(code),code16);
        hermesImpl::convertUtf8ToUtf16(reinterpret_cast<const uint8_t *>(msg), strlen(msg),msg16);
    }
    RETURN_STATUS_IF_FALSE(!code16.empty(), NAPIMemoryError);
    RETURN_STATUS_IF_FALSE(!msg16.empty(), NAPIMemoryError);

    auto codeRes = hermes::vm::StringPrimitive::createEfficient(env->context,std::move(code16));
    hermesImpl::getException(codeRes.getStatus(),env);
    CHECK_HERMES(env);

    auto msgRes = hermes::vm::StringPrimitive::createEfficient(env->context,std::move(msg16));
    hermesImpl::getException(msgRes.getStatus(),env);
    CHECK_HERMES(env);

    NAPIValue codeJsVal = hermesImpl::JsValueFromHermesValue(codeRes.getValue());
    RETURN_STATUS_IF_FALSE(codeJsVal, NAPIMemoryError);
    NAPIValue msgJsVal = hermesImpl::JsValueFromHermesValue(msgRes.getValue());
    RETURN_STATUS_IF_FALSE(msgJsVal, NAPIMemoryError);
    NAPIValue error;
    CHECK_NAPI(napi_create_error(env, codeJsVal, msgJsVal, &error));
    return napi_throw(env, error);
}


NAPIStatus napi_throw_type_error(NAPIEnv env, const char *code, const char *msg)
{
    CHECK_ENV(env);
    std::u16string code16;
    std::u16string msg16;
    hermes::vm::GCScope gcScope(reinterpret_cast<hermes::vm::HandleRootOwner *>(env->context));
    {
        hermesImpl::convertUtf8ToUtf16(reinterpret_cast<const uint8_t *>(code), strlen(code),code16);
        hermesImpl::convertUtf8ToUtf16(reinterpret_cast<const uint8_t *>(msg), strlen(msg),msg16);
    }
    RETURN_STATUS_IF_FALSE(!code16.empty(), NAPIMemoryError);
    RETURN_STATUS_IF_FALSE(!msg16.empty(), NAPIMemoryError);

    auto codeRes = hermes::vm::StringPrimitive::createEfficient(env->context,std::move(code16));
    hermesImpl::getException(codeRes.getStatus(),env);
    CHECK_HERMES(env);

    auto msgRes = hermes::vm::StringPrimitive::createEfficient(env->context,std::move(msg16));
    hermesImpl::getException(msgRes.getStatus(),env);
    CHECK_HERMES(env);

    NAPIValue codeJsVal = hermesImpl::JsValueFromHermesValue(codeRes.getValue());
    RETURN_STATUS_IF_FALSE(codeJsVal, NAPIMemoryError);
    NAPIValue msgJsVal = hermesImpl::JsValueFromHermesValue(msgRes.getValue());
    RETURN_STATUS_IF_FALSE(msgJsVal, NAPIMemoryError);
    NAPIValue error;
    CHECK_NAPI(napi_create_type_error(env, codeJsVal, msgJsVal, &error));
    return napi_throw(env, error);
}

NAPIStatus napi_throw_range_error(NAPIEnv env, const char *code, const char *msg)
{
    CHECK_ENV(env);
    std::u16string code16;
    std::u16string msg16;
    hermes::vm::GCScope gcScope(reinterpret_cast<hermes::vm::HandleRootOwner *>(env->context));
    {
        hermesImpl::convertUtf8ToUtf16(reinterpret_cast<const uint8_t *>(code), strlen(code),code16);
        hermesImpl::convertUtf8ToUtf16(reinterpret_cast<const uint8_t *>(msg), strlen(msg),msg16);
    }
    RETURN_STATUS_IF_FALSE(!code16.empty(), NAPIMemoryError);
    RETURN_STATUS_IF_FALSE(!msg16.empty(), NAPIMemoryError);

    auto codeRes = hermes::vm::StringPrimitive::createEfficient(env->context,std::move(code16));
    hermesImpl::getException(codeRes.getStatus(),env);
    CHECK_HERMES(env);

    auto msgRes = hermes::vm::StringPrimitive::createEfficient(env->context,std::move(msg16));
    hermesImpl::getException(msgRes.getStatus(),env);
    CHECK_HERMES(env);

    NAPIValue codeJsVal = hermesImpl::JsValueFromHermesValue(codeRes.getValue());
    RETURN_STATUS_IF_FALSE(codeJsVal, NAPIMemoryError);
    NAPIValue msgJsVal = hermesImpl::JsValueFromHermesValue(msgRes.getValue());
    RETURN_STATUS_IF_FALSE(msgJsVal, NAPIMemoryError);
    NAPIValue error;
    CHECK_NAPI(napi_create_range_error(env, codeJsVal, msgJsVal, &error));
    return napi_throw(env, error);
}


NAPIStatus napi_is_error(NAPIEnv env, NAPIValue value, bool *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    NAPIValue global;
    NAPIValue errorCtor;
    CHECK_NAPI(napi_get_global(env, &global));
    CHECK_NAPI(napi_get_named_property(env, global, "Error", &errorCtor));
    CHECK_NAPI(napi_instanceof(env, value, errorCtor, result));

    return clearLastError(env);
}

// Methods to support catching exceptions
NAPIStatus napi_is_exception_pending(NAPIEnv env, bool *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);
    *result = env->lastException.isEmpty();
    return clearLastError(env);
}

NAPIStatus napi_get_and_clear_last_exception(NAPIEnv env, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    if (env->lastException.isEmpty())
    {
        return napi_get_undefined(env, result);
    }
    else
    {
        *result = hermesImpl::JsValueFromHermesValue(env->lastException);
        env->lastException = hermes::vm::HermesValue::encodeEmptyValue();
    }
    return clearLastError(env);
}


NAPIStatus napi_create_error(NAPIEnv env, NAPIValue code, NAPIValue msg, NAPIValue *result){
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, msg);
    CHECK_ARG(env, result);
    using namespace hermes::vm;
    HandleRootOwner *superPtr = reinterpret_cast<HandleRootOwner *>(env->context);
    GCScope gcScope(superPtr);
    // hermes not throw error
    auto callRes = JSError::create(env->context, Handle<JSObject>::vmcast(&env->context->objectPrototype));
    // 重载 bool 运算符，本质为 value != nullptr;
    RETURN_STATUS_IF_FALSE(callRes, NAPIMemoryError);

    auto errorHandle = Handle<JSError>::dyn_vmcast(superPtr->makeHandle(callRes.getHermesValue()));
    RETURN_STATUS_IF_FALSE(callRes, NAPIMemoryError);

    auto msgHandle = Handle<JSError>::dyn_vmcast(superPtr->makeHandle(hermesImpl::HermesValueFromJsValue(msg)));
    RETURN_STATUS_IF_FALSE(msgHandle, NAPIMemoryError);

    auto res = JSError::setMessage(errorHandle,env->context,msgHandle);
    hermesImpl::getException(res,env);
    CHECK_HERMES(env);

    *result = hermesImpl::JsValueFromHermesValue(callRes.getHermesValue());
    CHECK_NAPI(setErrorCode(env, *result, code));
    return clearLastError(env);
}
// hermes 内置 TypeError 对象, 但没有暴露。
NAPIStatus napi_create_type_error(NAPIEnv env, NAPIValue code, NAPIValue msg, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, msg);
    CHECK_ARG(env, result);

    NAPIValue global;
    NAPIValue errorConstructor;

    CHECK_NAPI(napi_get_global(env, &global));
    CHECK_NAPI(napi_get_named_property(env, global, "TypeError", &errorConstructor));
    CHECK_NAPI(napi_new_instance(env, errorConstructor, 1, &msg, result));
    CHECK_NAPI(setErrorCode(env, *result, code));

    return clearLastError(env);
}

// hermes 内置 RangeError 对象, 但没有暴露。
NAPIStatus napi_create_range_error(NAPIEnv env, NAPIValue code, NAPIValue msg, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, msg);
    CHECK_ARG(env, result);

    NAPIValue global;
    NAPIValue errorConstructor;
    CHECK_NAPI(napi_get_global(env, &global));
    CHECK_NAPI(napi_get_named_property(env, global, "RangeError", &errorConstructor));
    CHECK_NAPI(napi_new_instance(env, errorConstructor, 1, &msg, result));
    CHECK_NAPI(setErrorCode(env, *result, code));

    return clearLastError(env);
}

#pragma mark <create | constructor>
NAPIStatus napi_get_undefined(NAPIEnv env, NAPIValue *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);
    *result = hermesImpl::JsValueFromHermesHandle(hermes::vm::Runtime::getUndefinedValue());
    return clearLastError(env);
}


NAPIStatus napi_get_null(NAPIEnv env, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);
    *result = hermesImpl::JsValueFromHermesHandle(hermes::vm::Runtime::getNullValue());
    return clearLastError(env);
}

NAPIStatus napi_get_global(NAPIEnv env, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env->context->getGlobal(), NAPIMemoryError);
    *result = (NAPIValue)hermesImpl::JsValueFromHermesHandle(env->context->getGlobal());

    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);
    return clearLastError(env);
}

NAPIStatus napi_get_boolean(NAPIEnv env, bool value, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);
    *result = hermesImpl::JsValueFromHermesHandle(hermes::vm::Runtime::getBoolValue(value));
    return clearLastError(env);
}

NAPIStatus napi_create_object(NAPIEnv env, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);
    *result = reinterpret_cast<NAPIValue>(hermes::vm::JSObject::create(env->context).get());
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);
    return clearLastError(env);
}


NAPIStatus napi_create_array(NAPIEnv env, NAPIValue *result){
    CHECK_ENV(env);
    CHECK_ARG(env, result);
    auto res = hermes::vm::JSArray::create(env->context,0,0);
    hermesImpl::getException(res.getStatus(),env);
    CHECK_HERMES(env);

    *result = hermesImpl::JsValueFromHermesValue(res->getHermesValue());
    return clearLastError(env);
}

NAPIStatus napi_create_array_with_length(NAPIEnv env, size_t length, NAPIValue *result){
    CHECK_ENV(env);
    CHECK_ARG(env, result);
    uint32_t len = (uint32_t)length;
    auto res = hermes::vm::JSArray::create(env->context,len,len);
    hermesImpl::getException(res.getStatus(),env);
    CHECK_HERMES(env);

    auto lengthNum = hermesImpl::JsValueFromHermesValue(hermes::vm::HermesValue::encodeNumberValue(length));
    auto arrayJSValue = hermesImpl::JsValueFromHermesValue(res->getHermesValue());
    CHECK_NAPI(napi_set_named_property(env, arrayJSValue, "length", lengthNum));
    *result = arrayJSValue;
    return clearLastError(env);
}

NAPIStatus napi_create_double(NAPIEnv env, double value, NAPIValue *result){

    CHECK_ENV(env);
    CHECK_ARG(env, result);
    *result = hermesImpl::JsValueFromHermesValue(hermes::vm::HermesValue::encodeDoubleValue(value));
    return clearLastError(env);
}

NAPIStatus napi_create_int32(NAPIEnv env, int32_t value, NAPIValue *result){

    CHECK_ENV(env);
    CHECK_ARG(env, result);
    *result = hermesImpl::JsValueFromHermesValue(hermes::vm::HermesValue::encodeNumberValue(value));
    return clearLastError(env);
}

NAPIStatus napi_create_uint32(NAPIEnv env, uint32_t value, NAPIValue *result){

    CHECK_ENV(env);
    CHECK_ARG(env, result);
    *result = hermesImpl::JsValueFromHermesValue(hermes::vm::HermesValue::encodeNativeUInt32(value));
    return clearLastError(env);
}

NAPIStatus napi_create_int64(NAPIEnv env, int64_t value, NAPIValue *result){

    CHECK_ENV(env);
    CHECK_ARG(env, result);
    *result = hermesImpl::JsValueFromHermesValue(hermes::vm::HermesValue::encodeNumberValue(value));
    return clearLastError(env);
}

NAPIStatus napi_create_string_utf8(NAPIEnv env, const char *str, size_t length, NAPIValue *result){

    CHECK_ENV(env);
    CHECK_ARG(env, result);

    std::u16string str16;
    {
        hermesImpl::convertUtf8ToUtf16(reinterpret_cast<const uint8_t *>(str), strlen(str),str16);
    }
    RETURN_STATUS_IF_FALSE(!str16.empty(), NAPIMemoryError);

    auto codeRes = hermes::vm::StringPrimitive::createEfficient(env->context,std::move(str16));
    hermesImpl::getException(codeRes.getStatus(),env);
    CHECK_HERMES(env);

    *result = hermesImpl::JsValueFromHermesValue(codeRes.getValue());
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return clearLastError(env);
}

NAPIStatus napi_create_string_utf16(NAPIEnv env, const char16_t *str, size_t length, NAPIValue *result){

    CHECK_ENV(env);
    CHECK_ARG(env, result);

    auto codeRes = hermes::vm::StringPrimitive::createEfficient(env->context,std::move(str));
    hermesImpl::getException(codeRes.getStatus(),env);
    CHECK_HERMES(env);

    *result = hermesImpl::JsValueFromHermesValue(codeRes.getValue());
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);
    return clearLastError(env);
}

NAPIStatus napi_create_symbol(NAPIEnv env, NAPIValue description, NAPIValue *result){

    CHECK_ENV(env);
    CHECK_ARG(env, result);

    NAPIValue global, symbolFunc;
    CHECK_NAPI(napi_get_global(env, &global));
    CHECK_NAPI(napi_get_named_property(env, global, "Symbol", &symbolFunc));
    CHECK_NAPI(napi_call_function(env, global, symbolFunc, 1, &description, result));
    return clearLastError(env);
}
#pragma mark <Working with JavaScript functions>

NAPIStatus napi_create_function(NAPIEnv env, const char *utf8name, size_t length, NAPICallback cb, void *data,
                                NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);
    CHECK_ARG(env, cb);



    // 不检查 errno，因为 errno 不可移植
    FunctionInfo *functionInfo = malloc(sizeof(FunctionInfo));
    if (!functionInfo)
    {
        return setLastErrorCode(env, NAPIMemoryError);
    }
    functionInfo->externalInfo.env = env;
    functionInfo->callback = cb;
    functionInfo->externalInfo.data = data;
    functionInfo->externalInfo.finalizeCallback = NULL;
    functionInfo->externalInfo.finalizeHint = NULL;
    SLIST_INIT(&functionInfo->externalInfo.finalizerHead);

    JSClassDefinition classDefinition = kJSClassDefinitionEmpty;
    // 使用 Object.prototype 作为 instance.[[prototype]]
    classDefinition.attributes = kJSClassAttributeNoAutomaticPrototype;
    // 最重要的是提供 finalize
    classDefinition.finalize = functionFinalize;
    JSClassRef classRef = JSClassCreate(&classDefinition);
    if (!classRef)
    {
        free(functionInfo);

        return setLastErrorCode(env, NAPIMemoryError);
    }
    JSObjectRef prototype = JSObjectMake(env->context, classRef, functionInfo);
    JSClassRelease(classRef);
    if (!prototype)
    {
        free(functionInfo);

        return setLastErrorCode(env, NAPIMemoryError);
    }

    // utf8name 会被当做函数的 .name 属性
    // V8 传入 NULL 会直接变成 ""
    JSStringRef stringRef = JSStringCreateWithUTF8CString(utf8name);
    RETURN_STATUS_IF_FALSE(stringRef, NAPIMemoryError);
    // JSObjectMakeFunctionWithCallback 传入函数名为 NULL 则为 anonymous
    JSObjectRef functionObjectRef = JSObjectMakeFunctionWithCallback(env->context, stringRef, callAsFunction);
    // JSStringRelease 不能传入 NULL
    JSStringRelease(stringRef);
    RETURN_STATUS_IF_FALSE(functionObjectRef, NAPIMemoryError);

    *result = (NAPIValue)functionObjectRef;

    // 修改原型链
    // 原型链修改失败也不影响 FunctionInfo 的析构，只是后续 callAsFunction
    // 会触发断言
    JSObjectSetPrototype(env->context, prototype, JSObjectGetPrototype(env->context, functionObjectRef));
    JSObjectSetPrototype(env->context, functionObjectRef, prototype);

    return clearLastError(env);
}

NAPIStatus napi_new_instance(NAPIEnv env, NAPIValue constructor, size_t argc, const NAPIValue *argv, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, constructor);
    if (argc > 0)
    {
        CHECK_ARG(env, argv);
    }
    CHECK_ARG(env, result);

    auto ctor_hermesValue = hermesImpl::HermesValueFromJsValue(constructor);
    auto ctor_Object = hermes::vm::dyn_vmcast_or_null<hermes::vm::JSObject>(ctor_hermesValue);
    RETURN_STATUS_IF_FALSE(ctorObject, NAPIMemoryError);
    RETURN_STATUS_IF_FALSE(hermes::vm::isConstructor(env->context,ctor_hermesValue), NAPIFunctionExpected);

    //todo:fix rt
    hermes::vm::HandleRootOwner *superRuntimePtr = reinterpret_cast<hermes::vm::HandleRootOwner *>(env->context);
    hermes::vm::Handle<hermes::vm::Callable> callableHandle = hermes::vm::Handle<hermes::vm::Callable>::dyn_vmcast(superRuntimePtr->makeHandle(ctor_Object));

    hermes::vm::CallResult<hermes::vm::PseudoHandle> ctorCallRes;
    if (argc > 0){

        ctorCallRes = hermes::vm::Callable::executeConstruct1(callableHandle, env->context, superRuntimePtr->makeHandle(hermesImpl::HermesValueFromJsValue()))
    }else{
        ctorCallRes = hermes::vm::Callable::executeConstruct0(callableHandle, env->context);
    }

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)constructor), NAPIObjectExpected);
    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)constructor, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(objectRef, NAPIMemoryError);
    RETURN_STATUS_IF_FALSE(JSObjectIsConstructor(env->context, objectRef), NAPIFunctionExpected);

    *result = (NAPIValue)JSObjectCallAsConstructor(env->context, objectRef, argc, (const JSValueRef *)argv,
                                                   &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return clearLastError(env);
}

#pragma mark <Working with JavaScript values and abstract operations>
NAPIStatus napi_typeof(NAPIEnv env, NAPIValue value, NAPIValueType *result){

    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    // Hermes does not support BigInt
    hermes::vm::HermesValue hv = hermesImpl::HermesValueFromJsValue(value);
    if (hv.isUndefined()){
        *result = NAPIUndefined;
    } else if (hv.isNull()){
        *result = NAPINull;
    } else if (hv.isBool()){
        *result = NAPIBoolean;
    } else if (hv.isNumber()){
        *result = NAPINumber;
    } else if (hv.isString()){
        *result = NAPIString;
    }
    else if (hv.isSymbol()){
        *result = NAPISymbol;
    }
    else if (hv.isObject())
    {
        auto function = hermes::vm::dyn_vmcast_or_null<hermes::vm::Callable>(hermesImpl::HermesValueFromJsValue(value));
        if (function)
        {
            *result = NAPIFunction;
        }
        else
        {
            auto hostObject =
                hermes::vm::dyn_vmcast_or_null<hermes::vm::HostObject>(hermesImpl::HermesValueFromJsValue(value));
            if (hostObject)
            {
                *result = NAPIExternal;
            }
            else
            {
                *result = NAPIObject;
            }
        }
    }else{
        assert(false);
        return setLastErrorCode(env, NAPIInvalidArg);
    }
    return clearLastError(env);
}

NAPIStatus napi_coerce_to_bool(NAPIEnv env, NAPIValue value, NAPIValue *result){

    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    *result = hermesImpl::JsValueFromHermesValue(hermes::vm::HermesValue::encodeBoolValue(hermes::vm::toBoolean(hermesImpl::HermesValueFromJsValue(value))));
    return clearLastError(env);
}

// The difference between this and toNumber is that it can return a
// BigInt.  But Hermes doesn't support BigInt yet, so for now, this
// is a placeholder.
NAPIStatus napi_coerce_to_number(NAPIEnv env, NAPIValue value, NAPIValue *result){

    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    //todo:fix rt
    hermes::vm::HandleRootOwner *rtPtr = reinterpret_cast<hermes::vm::HandleRootOwner *>(env->context);
    auto callRes = hermes::vm::toNumeric_RJS(env->context, rtPtr->makeHandle(hermesImpl::HermesValueFromJsValue(value)));
    hermesImpl::getException(callRes.getStatus(),env);
    CHECK_HERMES(env);

    *result = hermesImpl::JsValueFromHermesValue(callRes.getValue());
    return clearLastError(env);
}

NAPIStatus napi_coerce_to_object(NAPIEnv env, NAPIValue value, NAPIValue *result){

    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    //todo:fix rt
    hermes::vm::HandleRootOwner *rtPtr = reinterpret_cast<hermes::vm::HandleRootOwner *>(env->context);
    auto callRes = hermes::vm::toObject(env->context, rtPtr->makeHandle(hermesImpl::HermesValueFromJsValue(value)));
    hermesImpl::getException(callRes.getStatus(),env);
    CHECK_HERMES(env);

    *result = hermesImpl::JsValueFromHermesValue(callRes.getValue());
    return clearLastError(env);
}

NAPIStatus napi_coerce_to_string(NAPIEnv env, NAPIValue value, NAPIValue *result){

    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    //todo:fix rt
    hermes::vm::HandleRootOwner *rtPtr = reinterpret_cast<hermes::vm::HandleRootOwner *>(env->context);
    auto callRes = hermes::vm::toString_RJS(env->context, rtPtr->makeHandle(hermesImpl::HermesValueFromJsValue(value)));
    hermesImpl::getException(callRes.getStatus(),env);
    CHECK_HERMES(env);

    *result = hermesImpl::JsValueFromHermesValue(callRes->getHermesValue());
    return clearLastError(env);
}

NAPIStatus napi_instanceof(NAPIEnv env, NAPIValue object, NAPIValue constructor, bool *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, object);
    CHECK_ARG(env, result);

    auto obj_hv = hermesImpl::HermesValueFromJsValue(object);
    auto ctor_hv = hermesImpl::HermesValueFromJsValue(constructor);

    auto jsObject = hermes::vm::dyn_vmcast_or_null<hermes::vm::JSObject>(obj_hv);
    RETURN_STATUS_IF_FALSE(jsObject, NAPIObjectExpected);

    RETURN_STATUS_IF_FALSE(hermes::vm::isConstructor(env->context,obj_hv), NAPIFunctionExpected);


    //todo:fix rt
    hermes::vm::HandleRootOwner *rt = reinterpret_cast<hermes::vm::HandleRootOwner *>(env->context);
    auto callRes = hermes::vm::instanceOfOperator_RJS(env->context, rt->makeHandle(obj_hv), rt->makeHandle(ctor_hv));
    hermesImpl::getException(callRes.getStatus(),env);
    CHECK_HERMES(env);
    *result = callRes.getValue();
    return clearLastError(env);
}

#pragma mark <Methods to work with Objects>

NAPIStatus napi_get_prototype(NAPIEnv env, NAPIValue object, NAPIValue *result){

    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    auto jsObject = hermes::vm::dyn_vmcast_or_null<hermes::vm::JSObject>(hermesImpl::HermesValueFromJsValue(object));
    RETURN_STATUS_IF_FALSE(jsObject, NAPIObjectExpected);

    auto callRes = hermes::vm::JSObject::getPrototypeOf(hermes::vm::createPseudoHandle(jsObject), env->context);
    hermesImpl::getException(callRes.getStatus(),env);
    CHECK_HERMES(env);

    *result = hermesImpl::JsValueFromHermesValue(callRes->getHermesValue());
    return clearLastError(env);
}

NAPIStatus napi_get_property_names(NAPIEnv env, NAPIValue object, NAPIValue *result){


}

NAPIStatus napi_set_property(NAPIEnv env, NAPIValue object, NAPIValue key, NAPIValue value){

    NAPI_PREAMBLE(env);
    CHECK_ARG(env, key);
    CHECK_ARG(env, value);

    auto jsObject = hermes::vm::dyn_vmcast_or_null<hermes::vm::JSObject>(hermesImpl::HermesValueFromJsValue(object));
    RETURN_STATUS_IF_FALSE(jsObject, NAPIObjectExpected);

    auto keyString = hermes::vm::dyn_vmcast_or_null<hermes::vm::StringPrimitive>(hermesImpl::HermesValueFromJsValue(key));
    RETURN_STATUS_IF_FALSE(keyString, NAPIStringExpected);

    //symbol
    auto symbolRes = hermes::vm::stringToSymbolID(env->context, hermes::vm::createPseudoHandle(keyString));
    hermesImpl::getException(symbolRes.getStatus(),env);
    CHECK_HERMES(env);

    hermes::vm::HandleRootOwner *superRuntimePtr = reinterpret_cast<hermes::vm::HandleRootOwner *>(env->context);

    // call setter
    auto setRes = hermes::vm::JSObject::putNamed_RJS(superRuntimePtr->makeHandle(jsObject), env->context, symbolRes.getValue().get(), superRuntimePtr->makeHandle(hermesImpl::HermesValueFromJsValue(value)));
    hermesImpl::getException(setRes.getStatus(),env);
    CHECK_HERMES(env);

    return clearLastError(env);
}

NAPIStatus napi_has_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result){

    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    auto jsObject = hermes::vm::dyn_vmcast_or_null<hermes::vm::JSObject>(hermesImpl::HermesValueFromJsValue(object));
    RETURN_STATUS_IF_FALSE(jsObject, NAPIObjectExpected);

    auto keyString = hermes::vm::dyn_vmcast_or_null<hermes::vm::StringPrimitive>(hermesImpl::HermesValueFromJsValue(key));
    RETURN_STATUS_IF_FALSE(keyString, NAPIStringExpected);

    //symbol
    auto symbolRes = hermes::vm::stringToSymbolID(env->context, hermes::vm::createPseudoHandle(keyString));
    hermesImpl::getException(symbolRes.getStatus(),env);
    CHECK_HERMES(env);

    hermes::vm::HandleRootOwner *rtPtr = reinterpret_cast<hermes::vm::HandleRootOwner *>(env->context);
    auto callRes = hermes::vm::JSObject::hasNamed(rtPtr->makeHandle(jsObject), env->context ,symbolRes->get());
    hermesImpl::getException(symbolRes.getStatus(),env);
    CHECK_HERMES(env);

    *result = callRes.getValue();
    return clearLastError(env);
}

NAPIStatus napi_get_property(NAPIEnv env, NAPIValue object, NAPIValue key, NAPIValue *result){

    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    auto jsObject = hermes::vm::dyn_vmcast_or_null<hermes::vm::JSObject>(hermesImpl::HermesValueFromJsValue(object));
    RETURN_STATUS_IF_FALSE(jsObject, NAPIObjectExpected);

    auto keyString = hermes::vm::dyn_vmcast_or_null<hermes::vm::StringPrimitive>(hermesImpl::HermesValueFromJsValue(key));
    RETURN_STATUS_IF_FALSE(keyString, NAPIStringExpected);

    //symbol
    auto symbolRes = hermes::vm::stringToSymbolID(env->context, hermes::vm::createPseudoHandle(keyString));
    hermesImpl::getException(symbolRes.getStatus(),env);
    CHECK_HERMES(env);

    hermes::vm::HandleRootOwner *superRuntimePtr = reinterpret_cast<hermes::vm::HandleRootOwner *>(env->context);

    // call getter
    auto getterRes = hermes::vm::JSObject::getNamed_RJS(superRuntimePtr->makeHandle(jsObject), env->context, symbolRes.getValue().get());
    hermesImpl::getException(getterRes.getStatus(),env);
    CHECK_HERMES(env);

    //PseudoHandle 本质上为指针
    *result = hermesImpl::JsValueFromHermesValue(getterRes.getValue().get());
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return clearLastError(env);
}

NAPIStatus napi_delete_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result){

}

// hermes 没有暴露 hasOwnProperty 接口，但是 Object.cpp 内部有实现。
NAPIStatus napi_has_own_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result){

    CHECK_ENV(env);
    CHECK_ARG(env, key);
    CHECK_ARG(env, result);

    NAPIValueType valueType;
    CHECK_NAPI(napi_typeof(env, key, &valueType));
    RETURN_STATUS_IF_FALSE(valueType == NAPIString || valueType == NAPISymbol, NAPINameExpected);

    NAPIValue global, objectCtor, function, value;

    CHECK_NAPI(napi_get_global(env, &global));
    CHECK_NAPI(napi_get_named_property(env, global, "Object", &objectCtor));
    CHECK_NAPI(napi_get_named_property(env, objectCtor, "hasOwnProperty", &function));
    CHECK_NAPI(napi_call_function(env, object, function, 1, &key, &value));
    *result = hermesImpl::HermesValueFromJsValue(value).getBool();

    return clearLastError(env);
}

// This method is equivalent to calling napi_set_property with a napi_value created from the string passed in as utf8Name.
NAPIStatus napi_set_named_property(NAPIEnv env, NAPIValue object, const char *utf8name, NAPIValue value)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);

    std::u16string nameUTF16;
    {
        hermesImpl::convertUtf8ToUtf16(reinterpret_cast<const uint8_t *>(utf8name), strlen(utf8name),nameUTF16);
    }
    RETURN_STATUS_IF_FALSE(!nameUTF16.empty(), NAPIMemoryError);

    //hermes sting
    auto keyCallRes = hermes::vm::StringPrimitive::createEfficient(env->context,std::move(nameUTF16));
    hermesImpl::getException(keyCallRes.getStatus(),env);
    CHECK_HERMES(env);

    CHECK_NAPI(napi_set_property(env, object, hermesImpl::JsValueFromHermesValue(keyCallRes.getValue()), value));

    return clearLastError(env);
}

//This method is equivalent to calling napi_has_property with a napi_value created from the string passed in as utf8Name.
NAPIStatus napi_has_named_property(NAPIEnv env, NAPIValue object, const char *utf8name, bool *result){

    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    std::u16string nameUTF16;
    {
        hermesImpl::convertUtf8ToUtf16(reinterpret_cast<const uint8_t *>(utf8name), strlen(utf8name),nameUTF16);
    }
    RETURN_STATUS_IF_FALSE(!nameUTF16.empty(), NAPIMemoryError);

    //hermes sting
    auto keyCallRes = hermes::vm::StringPrimitive::createEfficient(env->context,std::move(nameUTF16));
    hermesImpl::getException(keyCallRes.getStatus(),env);
    CHECK_HERMES(env);

    CHECK_NAPI(napi_has_property(env, object, hermesImpl::JsValueFromHermesValue(keyCallRes.getValue()), result));
    return clearLastError(env);
}


//This method is equivalent to calling napi_get_property with a napi_value created from the string passed in as utf8Name.
NAPIStatus napi_get_named_property(NAPIEnv env, NAPIValue object, const char *utf8name, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    std::u16string code16;
    {
        hermesImpl::convertUtf8ToUtf16(reinterpret_cast<const uint8_t *>(utf8name), strlen(utf8name),code16);
    }
    RETURN_STATUS_IF_FALSE(!code16.empty(), NAPIMemoryError);

    //hermes sting
    auto keyCallRes = hermes::vm::StringPrimitive::createEfficient(env->context,std::move(code16));
    hermesImpl::getException(keyCallRes.getStatus(),env);
    CHECK_HERMES(env);

    CHECK_NAPI(napi_get_property(env, object, hermesImpl::JsValueFromHermesValue(keyCallRes.getValue()), result));
    return clearLastError(env);
}

NAPIStatus napi_set_element(NAPIEnv env, NAPIValue object, uint32_t index, NAPIValue value){

}

NAPIStatus napi_has_element(NAPIEnv env, NAPIValue object, uint32_t index, bool *result){

}

NAPIStatus napi_get_element(NAPIEnv env, NAPIValue object, uint32_t index, NAPIValue *result){

}

NAPIStatus napi_delete_element(NAPIEnv env, NAPIValue object, uint32_t index, bool *result){

}

NAPIStatus napi_define_properties(NAPIEnv env, NAPIValue object, size_t property_count,
                                  const NAPIPropertyDescriptor *properties){

}


#pragma mark <scope>
// hermes gcscope 为栈构造，这里包装为堆成员之后，需要保持 napi_open_handle_scope 和 napi_close_handle_scope 对应关系。
// 否则 在 hermes 执行 gc 时会造成 crash。
NAPIStatus napi_open_handle_scope(NAPIEnv env, NAPIHandleScope *result){
    CHECK_ENV(env);
    CHECK_ARG(env, result);
    hermes::vm::HandleRootOwner *superPtr = reinterpret_cast<hermes::vm::HandleRootOwner *>(env->context);
    *result = hermesImpl::JsHandleScopeFromHermesScope(new hermesImpl::HandleScopeWrapper(superPtr));
    env->open_handle_scopes++;
    return clearLastError(env);
}

NAPIStatus napi_close_handle_scope(NAPIEnv env, NAPIHandleScope result){
    CHECK_ENV(env);
    CHECK_ARG(env, result);
    if (env->open_handle_scopes == 0) {
        return NAPIHandleScopeMismatch;
    }
    env->open_handle_scopes--;
    hermesImpl::HandleScopeWrapper *scopeWrapper = hermesImpl::HermesScopeFormeJsHandleScope(result);
    delete scopeWrapper;
    return clearLastError(env);
}




