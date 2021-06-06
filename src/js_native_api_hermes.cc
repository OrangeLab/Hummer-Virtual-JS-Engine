#include <napi/js_native_api_types.h>
#include <napi/js_native_api.h>
#include <hermes/hermes.h>
#include <hermes/DebuggerAPI.h>
#include <hermes/VM/JSObject.h>
#include <hermes/VM/JSArray.h>
#include <hermes/VM/Runtime.h>
#include <hermes/VM/HandleRootOwner.h>
#include <hermes/VM/HermesValue.h>
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
//    std::unique_ptr<facebook::hermes::HermesRuntime> context;
    hermes::vm::Runtime *context;
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

}


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

#pragma mark <Error Handling>

NAPIStatus napi_throw(NAPIEnv env, NAPIValue error){

    CHECK_ENV(env);
    CHECK_ARG(env, error);
    env->lastException = hermesImpl::HermesValueFromJsValue(error);
    return clearLastError(env);
}

NAPIStatus napi_throw_error(NAPIEnv env, const char *code, const char *msg){

    CHECK_ENV(env);
    hermes::vm::cr
    JSStringRef stringRef = JSStringCreateWithUTF8CString(code);
    RETURN_STATUS_IF_FALSE(stringRef, NAPIMemoryError);
    JSValueRef codeValue = JSValueMakeString(env->context, stringRef);
    JSStringRelease(stringRef);
    RETURN_STATUS_IF_FALSE(codeValue, NAPIMemoryError);
    stringRef = JSStringCreateWithUTF8CString(msg);
    RETURN_STATUS_IF_FALSE(stringRef, NAPIMemoryError);
    JSValueRef msgValue = JSValueMakeString(env->context, stringRef);
    JSStringRelease(stringRef);
    RETURN_STATUS_IF_FALSE(msgValue, NAPIMemoryError);
    NAPIValue error;
    CHECK_NAPI(napi_create_error(env, (NAPIValue)codeValue, (NAPIValue)msgValue, &error));

    return napi_throw(env, error);
}

NAPIStatus napi_throw_type_error(NAPIEnv env, const char *code, const char *msg){

}

NAPIStatus napi_throw_range_error(NAPIEnv env, const char *code, const char *msg){

}

NAPIStatus napi_is_error(NAPIEnv env, NAPIValue value, bool *result){

}

// Methods to support catching exceptions
NAPIStatus napi_is_exception_pending(NAPIEnv env, bool *result){

}

NAPIStatus napi_get_and_clear_last_exception(NAPIEnv env, NAPIValue *result){

}

#pragma mark <get>
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
    hermes::vm::GCScope gcScope(reinterpret_cast<hermes::vm::HandleRootOwner *>(env->context));

}

NAPIStatus napi_create_array_with_length(NAPIEnv env, size_t length, NAPIValue *result){
    CHECK_ENV(env);
    CHECK_ARG(env, result);
    uint32_t len = (uint32_t)length;
    auto res = hermes::vm::JSArray::create(env->context,len,len);
    hermesImpl::getException(res.getStatus(),env);
    CHECK_HERMES(env);


}

NAPIStatus napi_create_double(NAPIEnv env, double value, NAPIValue *result){

}

NAPIStatus napi_create_int32(NAPIEnv env, int32_t value, NAPIValue *result){

}

NAPIStatus napi_create_uint32(NAPIEnv env, uint32_t value, NAPIValue *result){

}

NAPIStatus napi_create_int64(NAPIEnv env, int64_t value, NAPIValue *result){

}

NAPIStatus napi_create_string_utf8(NAPIEnv env, const char *str, size_t length, NAPIValue *result){

}

NAPIStatus napi_create_string_utf16(NAPIEnv env, const char16_t *str, size_t length, NAPIValue *result){

}

NAPIStatus napi_create_symbol(NAPIEnv env, NAPIValue description, NAPIValue *result){

}

NAPIStatus napi_create_function(NAPIEnv env, const char *utf8name, size_t length, NAPICallback cb, void *data,
                                NAPIValue *result){

                                }

NAPIStatus napi_create_error(NAPIEnv env, NAPIValue code, NAPIValue msg, NAPIValue *result){

}

NAPIStatus napi_create_type_error(NAPIEnv env, NAPIValue code, NAPIValue msg, NAPIValue *result){

}

NAPIStatus napi_create_range_error(NAPIEnv env, NAPIValue code, NAPIValue msg, NAPIValue *result){

}


#pragma mark <scope>
// hermes gcscope 为栈构造，这里包装为堆成员之后，需要保持 napi_open_handle_scope 和 napi_close_handle_scope 对应关系。
// 否则 在 hermes 执行 gc 时会造成 crash。
NAPIStatus napi_open_handle_scope(NAPIEnv env, NAPIHandleScope *result){
    CHECK_ENV(env);
    CHECK_ARG(env, result);
    *result = hermesImpl::JsHandleScopeFromHermesScope(new hermesImpl::HandleScopeWrapper(env->context));
    env->open_handle_scopes++;
    return clearLastError(env);
}

NAPIStatus napi_close_handle_scope(NAPIEnv env, NAPIHandleScope scope){
    CHECK_ENV(env);
    CHECK_ARG(env, result);
    if (env->open_handle_scopes == 0) {
        return NAPIHandleScopeMismatch;
    }
    env->open_handle_scopes--;
    hermesImpl::HandleScopeWrapper *scopeWrapper = hermesImpl::HermesScopeFormeJsHandleScope(scope);
    delete scopeWrapper;
    return clearLastError(env);
}




