// 如果一个函数返回的 NAPIStatus 为
// NAPIOK，则没有异常被挂起，并且不需要做任何处理 如果 NAPIStatus 是除了 NAPIOK
// 或者 NAPIPendingException，为了尝试恢复并继续执行，而不是直接返回，必须调用
// napi_is_exception_pending 来确定是否存在挂起的异常

// 大多数情况下，调用 N-API 函数的时候，如果已经有异常被挂起，函数会立即返回
// NAPIPendingException，但是，N-API 允许一些函数被调用用来在返回 JavaScript
// 环境前做最小化的清理 在这种情况下，NAPIStatus
// 会反映函数的执行结果状态，而不是当前存在异常，为了避免混淆，在每次函数执行后请检查结果

// 当异常被挂起的时候，有两种方法
// 1. 进行适当的清理并返回，那么执行环境会返回到 JavaScript，作为返回到
// JavaScript 的过渡的一部分，异常将在 JavaScript
// 调用原生方法的语句处抛出，大多数 N-API
// 函数在出现异常被挂起的时候，都只是简单的返回
// NAPIPendingException，，所以在这种情况下，建议尽量少做事情，等待返回到
// JavaScript 中处理异常
// 2.
// 尝试处理异常，在原生代码能捕获到异常的地方采取适当的操作，然后继续执行。只有当异常能被安全的处理的少数特定情况下推荐这样做，在这些情况下，napi_get_and_clear_last_exception
// 可以被用来获取并清除异常，在此之后，如果发现异常还是无法处理，可以通过
// napi_throw 重新抛出错误

#include <sys/queue.h>

#include <napi/js_native_api.h>

// setLastErrorCode 会处理 env == NULL 问题
#define RETURN_STATUS_IF_FALSE(condition, status)                                                                      \
    {                                                                                                                  \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            return status;                                                                                             \
        }                                                                                                              \
    }

#define CHECK_ARG(env, arg) RETURN_STATUS_IF_FALSE(arg, NAPIInvalidArg)

// This does not call napi_set_last_error because the expression
// is assumed to be a NAPI function call that already did.
#define CHECK_NAPI(expr)                                                                                               \
    {                                                                                                                  \
        NAPIStatus status = expr;                                                                                      \
        if (status != NAPIOK)                                                                                          \
        {                                                                                                              \
            return status;                                                                                             \
        }                                                                                                              \
    }

#define CHECK_JSC(env)                                                                                                 \
    {                                                                                                                  \
        CHECK_ARG(env);                                                                                                \
        RETURN_STATUS_IF_FALSE(!((env)->lastException), NAPIPendingException);                                         \
    }

#include <JavaScriptCore/JavaScriptCore.h>
#include <assert.h>
#include <napi/js_native_api_types.h>
#include <stdio.h>
#include <stdlib.h>

#define HASH_NONFATAL_OOM 1

// 先 hashError = false; 再判断是否变为 true，发生错误需要回滚内存分配
static bool hashError = false;

#define uthash_nonfatal_oom(elt) hashError = true;

#include <uthash.h>

struct OpaqueNAPIRef
{
    LIST_ENTRY(OpaqueNAPIRef) node;
    JSValueRef value;
    uint32_t count;
};

struct OpaqueNAPIEnv
{
    JSGlobalContextRef context;
    // undefined 和 null 实际上也可以当做 exception
    // 抛出，所以异常检查只需要检查是否为 C NULL
    JSValueRef lastException;
    NAPIExtendedErrorInfo lastError;
    LIST_HEAD(, OpaqueNAPIRef) strongReferenceHead;
};

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
    // 应当允许 code 为 undefined
    // 这是 Node.js 单元测试的要求
    if (!code)
    {
        return clearLastError(env);
    }
    CHECK_ENV(env);

    if (JSValueIsUndefined(env->context, (JSValueRef)code))
    {
        return clearLastError(env);
    }
    RETURN_STATUS_IF_FALSE(JSValueIsString(env->context, (JSValueRef)code), NAPIStringExpected);
    CHECK_NAPI(napi_set_named_property(env, error, "code", code));

    return clearLastError(env);
}

static inline bool checkIsExceptionPendingAndClear(NAPIEnv env)
{
    if (!env)
    {
        return false;
    }
    bool isPending = false;
    napi_is_exception_pending(env, &isPending);
    if (isPending)
    {
        // 忽略错误异常
        NAPIValue exception;
        napi_get_and_clear_last_exception(env, &exception);
    }

    return isPending;
}

// Warning: Keep in-sync with NAPIStatus enum
static const char *errorMessages[] = {NULL,
                                      "Invalid argument",
                                      "An object was expected",
                                      "A string was expected",
                                      "A string or symbol was expected",
                                      "A function was expected",
                                      "A number was expected",
                                      "A boolean was expected",
                                      "An array was expected",
                                      "Unknown failure",
                                      "An exception is pending",
                                      "The async work item was cancelled",
                                      "napi_escape_handle already called on scope",
                                      "Invalid handle scope usage",
                                      "Invalid callback scope usage",
                                      "Thread-safe function queue is full",
                                      "Thread-safe function handle is closing",
                                      "A bigint was expected",
                                      "A date was expected",
                                      "An arraybuffer was expected",
                                      "A detachable arraybuffer was expected",
                                      "Main thread would deadlock",
                                      "Memory allocation error"};

// The value of the constant below must be updated to reference the last
// message in the `napi_status` enum each time a new error message is added.
// We don't have a napi_status_last as this would result in an ABI
// change each time a message was added.
#define LAST_STATUS NAPIMemoryError

NAPIStatus napi_get_last_error_info(NAPIEnv env, const NAPIExtendedErrorInfo **result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    static_assert(sizeof(errorMessages) / sizeof(*errorMessages) == LAST_STATUS + 1,
                  "Count of error messages must match count of error values");

    if (env->lastError.errorCode <= LAST_STATUS)
    {
        // Wait until someone requests the last error information to fetch the
        // error message string
        env->lastError.errorMessage = errorMessages[env->lastError.errorCode];
    }
    else
    {
        // Release 模式会添加 NDEBUG 关闭 C 断言
        // 顶多缺失 errorMessage，尽量不要 Crash
        assert(false);
    }

    *result = &(env->lastError);

    // 这里不能使用 clearLastError(env);
    return NAPIOK;
}

NAPIStatus napi_get_undefined(NAPIEnv env, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue)JSValueMakeUndefined(env->context);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return clearLastError(env);
}

NAPIStatus napi_get_null(NAPIEnv env, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue)JSValueMakeNull(env->context);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return clearLastError(env);
}

NAPIStatus napi_get_global(NAPIEnv env, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue)JSContextGetGlobalObject(env->context);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return clearLastError(env);
}

NAPIStatus napi_get_boolean(NAPIEnv env, bool value, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue)JSValueMakeBoolean(env->context, value);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return clearLastError(env);
}

NAPIStatus napi_create_object(NAPIEnv env, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue)JSObjectMake(env->context, NULL, NULL);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return clearLastError(env);
}

NAPIStatus napi_create_array(NAPIEnv env, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue)JSObjectMakeArray(env->context, 0, NULL, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return clearLastError(env);
}

// 本质为 const array = new Array(); array.length = length;
NAPIStatus napi_create_array_with_length(NAPIEnv env, size_t length, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    JSObjectRef objectRef = JSObjectMakeArray(env->context, 0, NULL, &env->lastException);
    CHECK_JSC(env);
    // JSObjectSetProperty 传入 NULL 会崩溃
    RETURN_STATUS_IF_FALSE(objectRef, NAPIMemoryError);

    JSStringRef lengthStringRef = JSStringCreateWithUTF8CString("length");
    // JSObjectSetProperty 传入 lengthStringRef == NULL 会崩溃
    RETURN_STATUS_IF_FALSE(lengthStringRef, NAPIMemoryError);
    JSObjectSetProperty(env->context, objectRef, lengthStringRef, JSValueMakeNumber(env->context, (double)length),
                        kJSPropertyAttributeNone, &env->lastException);
    JSStringRelease(lengthStringRef);
    CHECK_JSC(env);

    *result = (NAPIValue)objectRef;

    return clearLastError(env);
}

NAPIStatus napi_create_double(NAPIEnv env, double value, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue)JSValueMakeNumber(env->context, value);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return clearLastError(env);
}

NAPIStatus napi_create_int32(NAPIEnv env, int32_t value, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue)JSValueMakeNumber(env->context, value);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return clearLastError(env);
}

NAPIStatus napi_create_uint32(NAPIEnv env, uint32_t value, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue)JSValueMakeNumber(env->context, value);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return clearLastError(env);
}

NAPIStatus napi_create_int64(NAPIEnv env, int64_t value, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue)JSValueMakeNumber(env->context, (double)value);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return clearLastError(env);
}

// JavaScriptCore 只能接受 \0 结尾的字符串
// 传入 str NULL 则为 ""
// V8 引擎传入 NULL 直接崩溃
NAPIStatus napi_create_string_utf8(NAPIEnv env, const char *str, size_t length, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(length == NAPI_AUTO_LENGTH, NAPIInvalidArg);

    // 传入 NULL，触发 OpaqueJSString()
    JSStringRef stringRef = JSStringCreateWithUTF8CString(str);
    // stringRef == NULL，会调用 String()
    *result = (NAPIValue)JSValueMakeString(env->context, stringRef);
    if (stringRef)
    {
        JSStringRelease(stringRef);
    }
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return clearLastError(env);
}

// V8 引擎传入 NULL 返回 ""
NAPIStatus napi_create_string_utf16(NAPIEnv env, const char16_t *str, size_t length, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    static_assert(sizeof(char16_t) == sizeof(JSChar), "char16_t size not equal JSChar");
    uint16_t value = 0x0102;
    if (*((uint8_t *)&value) == 1)
    {
        // 大端
        char16_t *newStr = malloc(sizeof(char16_t) * length);
        RETURN_STATUS_IF_FALSE(str, NAPIMemoryError);
        for (size_t i = 0; i < length; ++i)
        {
            newStr[i] = ((str[i] << 8) & 0xff00) | ((str[i] >> 8) & 0x00ff);
        }
        str = newStr;
    }

    RETURN_STATUS_IF_FALSE(length != NAPI_AUTO_LENGTH, NAPIInvalidArg);
    JSStringRef stringRef = JSStringCreateWithCharacters(str, length);
    *result = (NAPIValue)JSValueMakeString(env->context, stringRef);
    if (stringRef)
    {
        JSStringRelease(stringRef);
    }
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return clearLastError(env);
}

NAPIStatus napi_create_symbol(NAPIEnv env, NAPIValue description, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    NAPIValue global, symbolFunc;
    CHECK_NAPI(napi_get_global(env, &global));
    // deployment target iOS 9
    CHECK_NAPI(napi_get_named_property(env, global, "Symbol", &symbolFunc));
    CHECK_NAPI(napi_call_function(env, global, symbolFunc, 1, &description, result));

    return clearLastError(env);
}

// Function

struct OpaqueNAPICallbackInfo
{
    JSObjectRef newTarget;
    JSObjectRef thisArg;
    const JSValueRef *argv;
    void *data;
    // 32 位情况下为 4 字节长度，放到最后
    size_t argc;
};

typedef struct
{
    NAPIEnv env;
    void *data;
    SLIST_HEAD(, Finalizer) finalizerHead;
    // napi_wrap 析构不放到 finalizerHead 中
    NAPIFinalize finalizeCallback;
    void *finalizeHint;
} ExternalInfo;

typedef struct
{
    // ExternalInfo
    ExternalInfo externalInfo;
    // FunctionInfo
    NAPICallback callback;
} FunctionInfo;

// 所有参数都会存在，返回值可以为 NULL
static JSValueRef callAsFunction(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount,
                                 const JSValueRef arguments[], JSValueRef *exception)
{
    // JSObjectSetPrototype 确实可以传入非对象 JSValue，但是会被替换为 JS
    // Null，并且可以正常取出来
    JSValueRef prototype = JSObjectGetPrototype(ctx, function);
    if (!JSValueIsObject(ctx, prototype))
    {
        // 正常不应当出现
        assert(false);

        // 返回 NULL 会被转换为 undefined
        return NULL;
    }
    JSObjectRef prototypeObjectRef = JSValueToObject(ctx, prototype, exception);
    if (*exception || !prototypeObjectRef)
    {
        // 正常不应当出现
        assert(false);

        return NULL;
    }
    FunctionInfo *functionInfo = JSObjectGetPrivate(prototypeObjectRef);
    if (!functionInfo || !functionInfo->externalInfo.env || !functionInfo->callback)
    {
        // 正常不应当出现
        assert(false);

        return NULL;
    }

    clearLastError(functionInfo->externalInfo.env);

    // JavaScriptCore 参数都不是 NULL
    struct OpaqueNAPICallbackInfo callbackInfo;
    callbackInfo.newTarget = NULL;
    callbackInfo.thisArg = thisObject;
    callbackInfo.argc = argumentCount;
    callbackInfo.argv = arguments;
    callbackInfo.data = functionInfo->externalInfo.data;

    JSValueRef returnValue = (JSValueRef)functionInfo->callback(functionInfo->externalInfo.env, &callbackInfo);

    bool isPending = false;
    if (napi_is_exception_pending(functionInfo->externalInfo.env, &isPending) != NAPIOK)
    {
        // 正常不应当出现
        assert(false);

        return NULL;
    }
    if (isPending)
    {
        // 直接提取
        if (napi_get_and_clear_last_exception(functionInfo->externalInfo.env, (NAPIValue *)exception) != NAPIOK)
        {
            // 正常不应当出现
            assert(false);
        }

        // exception 被设置后，JavaScriptCore 会忽略返回值
        return NULL;
    }

    return returnValue;
}

struct Finalizer
{
    SLIST_ENTRY(Finalizer) node;
    NAPIFinalize finalizeCallback;
    void *finalizeHint;
};

static inline void finalizeExternalInfo(ExternalInfo *info)
{
    if (info)
    {
        if (info->finalizeCallback)
        {
            info->finalizeCallback(info->env, info->data, info->finalizeHint);
        }
        struct Finalizer *finalizer, *tempFinalizer;
        SLIST_FOREACH_SAFE(finalizer, &info->finalizerHead, node, tempFinalizer)
        {
            if (finalizer->finalizeCallback)
            {
                finalizer->finalizeCallback(info->env, info->data, finalizer->finalizeHint);
            }
            free(finalizer);
        }
    }
}

static void functionFinalize(JSObjectRef object)
{
    FunctionInfo *functionInfo = JSObjectGetPrivate(object);
    finalizeExternalInfo(&functionInfo->externalInfo);
    free(functionInfo);
}

NAPIStatus napi_create_function(NAPIEnv env, const char *utf8name, size_t length, NAPICallback cb, void *data,
                                NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);
    CHECK_ARG(env, cb);

    RETURN_STATUS_IF_FALSE(length == NAPI_AUTO_LENGTH, NAPIInvalidArg);

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

NAPIStatus napi_create_error(NAPIEnv env, NAPIValue code, NAPIValue msg, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, msg);
    CHECK_ARG(env, result);

    *result = (NAPIValue)JSObjectMakeError(env->context, 1, (JSValueRef const *)&msg, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);
    CHECK_NAPI(setErrorCode(env, *result, code));

    return clearLastError(env);
}

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

NAPIStatus napi_typeof(NAPIEnv env, NAPIValue value, NAPIValueType *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    // JSC does not support BigInt
    JSType valueType = JSValueGetType(env->context, (JSValueRef)value);
    switch (valueType)
    {
    case kJSTypeUndefined:
        *result = NAPIUndefined;
        break;
    case kJSTypeNull:
        *result = NAPINull;
        break;
    case kJSTypeBoolean:
        *result = NAPIBoolean;
        break;
    case kJSTypeNumber:
        *result = NAPINumber;
        break;
    case kJSTypeString:
        *result = NAPIString;
        break;
    case kJSTypeSymbol:
        *result = NAPISymbol;
        break;
    case kJSTypeObject: {
        JSObjectRef object = JSValueToObject(env->context, (JSValueRef)value, &env->lastException);
        CHECK_JSC(env);
        RETURN_STATUS_IF_FALSE(object, NAPIMemoryError);
        if (JSObjectIsFunction(env->context, object))
        {
            *result = NAPIFunction;
        }
        else
        {
            if (JSObjectGetPrivate(object))
            {
                *result = NAPIExternal;
            }
            else
            {
                *result = NAPIObject;
            }
        }
    }
    break;
    default:
        // Should not get here unless V8 has added some new kind of value.
        assert(false);
        return setLastErrorCode(env, NAPIInvalidArg);
    }

    return clearLastError(env);
}

NAPIStatus napi_get_value_double(NAPIEnv env, NAPIValue value, double *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(JSValueIsNumber(env->context, (JSValueRef)value), NAPINumberExpected);

    *result = JSValueToNumber(env->context, (JSValueRef)value, &env->lastException);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus napi_get_value_int32(NAPIEnv env, NAPIValue value, int32_t *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(JSValueIsNumber(env->context, (JSValueRef)value), NAPINumberExpected);

    // 缺少 int64_t 作为中间转换，会固定在 -2147483648
    // TODO(ChasonTang): 虽然 ubsan 没有说明该行为是
    // UB，但是还是需要从规范上考虑是否该行为是 UB？ 但是 uint32_t 不会
    *result = (int32_t)(int64_t)JSValueToNumber(env->context, (JSValueRef)value, &env->lastException);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus napi_get_value_uint32(NAPIEnv env, NAPIValue value, uint32_t *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(JSValueIsNumber(env->context, (JSValueRef)value), NAPINumberExpected);

    *result = (uint32_t)JSValueToNumber(env->context, (JSValueRef)value, &env->lastException);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus napi_get_value_int64(NAPIEnv env, NAPIValue value, int64_t *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(JSValueIsNumber(env->context, (JSValueRef)value), NAPINumberExpected);

    double doubleValue = JSValueToNumber(env->context, (JSValueRef)value, &env->lastException);
    CHECK_JSC(env);

    if (isfinite(doubleValue))
    {
        if (isnan(doubleValue))
        {
            *result = 0;
        }
        else if (doubleValue >= (double)QUAD_MAX)
        {
            *result = QUAD_MAX;
        }
        else if (doubleValue <= (double)QUAD_MIN)
        {
            *result = QUAD_MIN;
        }
        else
        {
            *result = (int64_t)doubleValue;
        }
    }
    else
    {
        *result = 0;
    }

    return clearLastError(env);
}

NAPIStatus napi_get_value_bool(NAPIEnv env, NAPIValue value, bool *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(JSValueIsBoolean(env->context, (JSValueRef)value), NAPINumberExpected);
    *result = JSValueToBoolean(env->context, (JSValueRef)value);

    return clearLastError(env);
}

// 光计算长度实际上存在性能损耗
// Copies a JavaScript string into a UTF-8 string buffer. The result is the
// number of bytes (excluding the null terminator) copied into buf.
// A sufficient buffer size should be greater than the length of string,
// reserving space for null terminator.
// If bufsize is insufficient, the string will be truncated and null terminated.
// If buf is NULL, this method returns the length of the string (in bytes)
// via the result parameter.
// The result argument is optional unless buf is NULL.
NAPIStatus napi_get_value_string_utf8(NAPIEnv env, NAPIValue value, char *buf, size_t bufsize, size_t *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);

    RETURN_STATUS_IF_FALSE(JSValueIsString(env->context, (JSValueRef)value), NAPIStringExpected);

    // 1. buf == NULL 计算长度，这是 N-API 文档规定
    // 2. buf && bufsize 发生复制
    // 3. buf && bufsize == 0 正常情况实际上也可以计算长度，但是 N-API
    // 规定计算长度必须是 buf == NULL，因此只能返回 0 长度
    if (buf && bufsize == 0)
    {
        // Node.js N-API 这里不检查 result 指针，会导致崩溃
        if (result)
        {
            *result = 0;
        }
    }
    else
    {
        if (!buf)
        {
            CHECK_ARG(env, result);
        }
        JSStringRef stringRef = JSValueToStringCopy(env->context, (JSValueRef)value, &env->lastException);
        CHECK_JSC(env);
        RETURN_STATUS_IF_FALSE(stringRef, NAPIMemoryError);
        // NOTE: By definition, maxBytes >= 1 since the null terminator is
        // included.
        size_t length = JSStringGetMaximumUTF8CStringSize(stringRef);
        if (!buf)
        {
            // TODO(ChasonTang): 栈分配
            assert(length);
            char *buffer = malloc(sizeof(char) * length);
            if (!buffer)
            {
                // malloc 失败，无法发生复制，则无法计算实际长度
                JSStringRelease(stringRef);

                return setLastErrorCode(env, NAPIMemoryError);
            }
            // 返回值一定带 \0
            size_t copied = JSStringGetUTF8CString(stringRef, buffer, length);
            free(buffer);
            if (!copied)
            {
                // 失败
                JSStringRelease(stringRef);

                return setLastErrorCode(env, NAPIMemoryError);
            }
            *result = copied - 1;
        }
        else
        {
            // JSStringGetUTFCString 如果实际为 ASCII，则不会考虑传入的
            // bufferSize 参数，如果是 Latin1，则会报错并返回 1 长度，这是一个
            // bug 不包含 \0
            if (length > bufsize)
            {
                // TODO(ChasonTang): 栈分配
                char *buffer = malloc(sizeof(char) * length);
                if (!buffer)
                {
                    JSStringRelease(stringRef);

                    return clearLastError(env);
                }
                length = JSStringGetUTF8CString(stringRef, buffer, length);
                if (!length)
                {
                    JSStringRelease(stringRef);
                    free(buffer);

                    return setLastErrorCode(env, NAPIMemoryError);
                }
                if (JSStringGetLength(stringRef) + 1 == length)
                {
                    // ASCII
                    if (length <= bufsize)
                    {
                        memmove(buf, buffer, length);
                        if (result)
                        {
                            // JSStringGetUTF8CString returns size with null
                            // terminator.
                            *result = length - 1;
                        }
                    }
                    else
                    {
                        // 截断
                        // bufsize 不等于 0
                        buffer[bufsize - 1] = '\0';
                        // 不要使用 memcpy
                        memmove(buf, buffer, bufsize);
                        if (result)
                        {
                            // JSStringGetUTF8CString returns size with null
                            // terminator.
                            *result = bufsize - 1;
                        }
                    }
                }
                else
                {
                    bool is8Bit = true;
                    size_t i = 0;
                    while (i < length)
                    {
                        if (((uint8_t *)buffer)[i] < 0x80)
                        {
                            // ASCII
                            ++i;
                            continue;
                            ;
                        }
                        else if (((uint8_t *)buffer)[i] == 0xc2 || ((uint8_t *)buffer)[i] == 0xc3)
                        {
                            // 110xxxxx 10xxxxxx
                            // 11000010 10000000 -> 11000011 10111111
                            // 读取第二个字节
                            // 越界检查
                            if (++i >= length)
                            {
                                assert(false);
                                JSStringRelease(stringRef);
                                free(buffer);

                                return setLastErrorCode(env, NAPIMemoryError);
                            }
                            // 校验
                            if (((uint8_t *)buffer)[i] >> 6 != 2)
                            {
                                assert(false);
                                JSStringRelease(stringRef);
                                free(buffer);

                                return setLastErrorCode(env, NAPIMemoryError);
                            }
                            if (((uint8_t *)buffer)[i] < 0x80 || ((uint8_t *)buffer)[i] > 0xbf)
                            {
                                is8Bit = false;
                                break;
                            }

                            ++i;
                            continue;
                            ;
                        }
                        else
                        {
                            is8Bit = false;
                            break;
                        }
                    }
                    if (is8Bit)
                    {
                        // latin1
                        if (length <= bufsize)
                        {
                            memmove(buf, buffer, length);
                            if (result)
                            {
                                // JSStringGetUTF8CString returns size with null
                                // terminator.
                                *result = length - 1;
                            }
                        }
                        else
                        {
                            buffer[bufsize - (bufsize % 2 ? 1 : 2)] = 0;
                            // 不要使用 memcpy
                            memmove(buf, buffer, bufsize);
                            if (result)
                            {
                                // JSStringGetUTF8CString returns size with null
                                // terminator.
                                *result = bufsize - 1;
                            }
                        }
                    }
                    else if (length <= bufsize)
                    {
                        memmove(buf, buffer, length);
                        if (result)
                        {
                            // JSStringGetUTF8CString returns size with null
                            // terminator.
                            *result = length - 1;
                        }
                    }
                    else
                    {
                        // 重新复制
                        length = JSStringGetUTF8CString(stringRef, buf, bufsize);
                        if (!length)
                        {
                            JSStringRelease(stringRef);
                            free(buffer);

                            return setLastErrorCode(env, NAPIMemoryError);
                        }
                        if (result)
                        {
                            // JSStringGetUTF8CString returns size with null
                            // terminator.
                            *result = length - 1;
                        }
                    }
                }
                free(buffer);
            }
            else
            {
                size_t copied = JSStringGetUTF8CString(stringRef, buf, bufsize);
                if (!copied)
                {
                    JSStringRelease(stringRef);

                    return setLastErrorCode(env, NAPIMemoryError);
                }
                if (result)
                {
                    // JSStringGetUTF8CString returns size with null terminator.
                    *result = copied - 1;
                }
            }
        }
        JSStringRelease(stringRef);
    }

    return clearLastError(env);
}

// Copies a JavaScript string into a UTF-16 string buffer. The result is the
// number of 2-byte code units (excluding the null terminator) copied into buf.
// A sufficient buffer size should be greater than the length of string,
// reserving space for null terminator.
// If bufsize is insufficient, the string will be truncated and null terminated.
// If buf is NULL, this method returns the length of the string (in 2-byte
// code units) via the result parameter.
// The result argument is optional unless buf is NULL.
NAPIStatus napi_get_value_string_utf16(NAPIEnv env, NAPIValue value, char16_t *buf, size_t bufsize, size_t *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);

    RETURN_STATUS_IF_FALSE(JSValueIsString(env->context, (JSValueRef)value), NAPIStringExpected);

    if (buf && bufsize == 0)
    {
        if (result)
        {
            *result = 0;
        }
    }
    else
    {
        if (!buf)
        {
            CHECK_ARG(env, result);
        }
        JSStringRef stringRef = JSValueToStringCopy(env->context, (JSValueRef)value, &env->lastException);
        CHECK_JSC(env);
        RETURN_STATUS_IF_FALSE(stringRef, NAPIMemoryError);
        if (!buf)
        {
            *result = JSStringGetLength(stringRef);
        }
        else
        {
            static_assert(sizeof(char16_t) == sizeof(JSChar), "char16_t size not equal JSChar");

            size_t length = JSStringGetLength(stringRef);
            const JSChar *chars = JSStringGetCharactersPtr(stringRef);
            size_t copied = length > bufsize - 1 ? bufsize - 1 : length;
            memmove(buf, chars, copied * sizeof(char16_t));
            buf[copied] = 0;

            uint16_t checkValue = 0x0102;
            if (*((uint8_t *)&checkValue) == 1)
            {
                // 大端
                for (size_t i = 0; i < length; ++i)
                {
                    buf[i] = ((buf[i] << 8) & 0xff00) | ((buf[i] >> 8) & 0x00ff);
                }
            }

            if (result)
            {
                *result = copied;
            }
        }

        JSStringRelease(stringRef);
    }

    return clearLastError(env);
}

NAPIStatus napi_coerce_to_bool(NAPIEnv env, NAPIValue value, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    *result = (NAPIValue)JSValueMakeBoolean(env->context, JSValueToBoolean(env->context, (JSValueRef)value));
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return clearLastError(env);
}

NAPIStatus napi_coerce_to_number(NAPIEnv env, NAPIValue value, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    double doubleValue = JSValueToNumber(env->context, (JSValueRef)value, &env->lastException);
    CHECK_JSC(env);
    *result = (NAPIValue)JSValueMakeNumber(env->context, doubleValue);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return clearLastError(env);
}

NAPIStatus napi_coerce_to_object(NAPIEnv env, NAPIValue value, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->context);

    NAPIValue global, objectCtor;
    CHECK_NAPI(napi_get_global(env, &global));
    CHECK_NAPI(napi_get_named_property(env, global, "Object", &objectCtor));
    CHECK_NAPI(napi_new_instance(env, objectCtor, 1, &value, result));

    return clearLastError(env);
}

NAPIStatus napi_coerce_to_string(NAPIEnv env, NAPIValue value, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    JSStringRef stringRef = JSValueToStringCopy(env->context, (JSValueRef)value, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(stringRef, NAPIMemoryError);
    *result = (NAPIValue)JSValueMakeString(env->context, stringRef);
    if (stringRef)
    {
        JSStringRelease(stringRef);
    }
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return clearLastError(env);
}

NAPIStatus napi_get_prototype(NAPIEnv env, NAPIValue object, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)object), NAPIObjectExpected);
    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)object, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(objectRef, NAPIMemoryError);
    *result = (NAPIValue)JSObjectGetPrototype(env->context, objectRef);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return clearLastError(env);
}

NAPIStatus napi_get_property_names(NAPIEnv env, NAPIValue object, NAPIValue *result)
{
    // N-API V8 实现默认
    // 1. 包含原型链
    // 2. 可枚举，不包括 Symbol
    // 3. number 转换为 string
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    // 应当检查参数是否为对象
    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)object), NAPIObjectExpected);
    // 使用 JSObjectCopyPropertyNames 实现，符合 Object.keys
    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)object, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(objectRef, NAPIPendingException);

    JSPropertyNameArrayRef propertyNameArrayRef = JSObjectCopyPropertyNames(env->context, objectRef);

    size_t propertyCount = JSPropertyNameArrayGetCount(propertyNameArrayRef);
    *result = (NAPIValue)JSObjectMakeArray(env->context, 0, NULL, &env->lastException);
    if (env->lastException)
    {
        JSPropertyNameArrayRelease(propertyNameArrayRef);

        return setLastErrorCode(env, NAPIPendingException);
    }
    if (!*result)
    {
        JSPropertyNameArrayRelease(propertyNameArrayRef);

        return setLastErrorCode(env, NAPIMemoryError);
    }

    NAPIValue pushFunction;
    NAPIStatus status = napi_get_named_property(env, *result, "push", &pushFunction);
    if (status != NAPIOK)
    {
        JSPropertyNameArrayRelease(propertyNameArrayRef);

        return status;
    }

    if (propertyCount > 0)
    {
        for (unsigned int i = 0; i < propertyCount; ++i)
        {
            JSStringRef propertyNameStringRef = JSPropertyNameArrayGetNameAtIndex(propertyNameArrayRef, i);
            JSValueRef propertyValueRef =
                JSObjectGetProperty(env->context, objectRef, propertyNameStringRef, &env->lastException);
            if (env->lastException)
            {
                JSPropertyNameArrayRelease(propertyNameArrayRef);

                // 尽量抛出异常
                return setLastErrorCode(env, NAPIPendingException);
            }

            // 传入 NULL 参数没关系，会转换成 JS Null
            status = napi_call_function(env, *result, pushFunction, 1, (NAPIValue const *)&propertyValueRef, NULL);
            if (status != NAPIOK)
            {
                JSPropertyNameArrayRelease(propertyNameArrayRef);

                return status;
            }
        }
    }
    JSPropertyNameArrayRelease(propertyNameArrayRef);

    return clearLastError(env);
}

NAPIStatus napi_set_property(NAPIEnv env, NAPIValue object, NAPIValue key, NAPIValue value)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, key);
    CHECK_ARG(env, value);

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)object), NAPIObjectExpected);
    RETURN_STATUS_IF_FALSE(JSValueIsString(env->context, (JSValueRef)key), NAPIStringExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)object, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(objectRef, NAPIMemoryError);

    JSStringRef keyStringRef = JSValueToStringCopy(env->context, (JSValueRef)key, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(keyStringRef, NAPIMemoryError);

    JSObjectSetProperty(env->context, objectRef, keyStringRef, (JSValueRef)value, kJSPropertyAttributeNone,
                        &env->lastException);
    JSStringRelease(keyStringRef);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus napi_has_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);
    CHECK_ARG(env, key);

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)object), NAPIObjectExpected);
    RETURN_STATUS_IF_FALSE(JSValueIsString(env->context, (JSValueRef)key), NAPIStringExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)object, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(objectRef, NAPIMemoryError);

    JSStringRef keyStringRef = JSValueToStringCopy(env->context, (JSValueRef)key, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(keyStringRef, NAPIMemoryError);

    *result = JSObjectHasProperty(env->context, objectRef, keyStringRef);
    JSStringRelease(keyStringRef);

    return clearLastError(env);
}

NAPIStatus napi_get_property(NAPIEnv env, NAPIValue object, NAPIValue key, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, key);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)object), NAPIObjectExpected);
    RETURN_STATUS_IF_FALSE(JSValueIsString(env->context, (JSValueRef)key), NAPIStringExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)object, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(objectRef, NAPIMemoryError);

    JSStringRef keyStringRef = JSValueToStringCopy(env->context, (JSValueRef)key, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(keyStringRef, NAPIMemoryError);

    *result = (NAPIValue)JSObjectGetProperty(env->context, objectRef, keyStringRef, &env->lastException);
    JSStringRelease(keyStringRef);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return clearLastError(env);
}

NAPIStatus napi_delete_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, key);

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)object), NAPIObjectExpected);
    RETURN_STATUS_IF_FALSE(JSValueIsString(env->context, (JSValueRef)key), NAPIStringExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)object, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(objectRef, NAPIMemoryError);

    JSStringRef keyStringRef = JSValueToStringCopy(env->context, (JSValueRef)key, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(keyStringRef, NAPIMemoryError);

    *result = JSObjectDeleteProperty(env->context, objectRef, keyStringRef, &env->lastException);
    JSStringRelease(keyStringRef);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus napi_has_own_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result)
{
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
    *result = JSValueToBoolean(env->context, (JSValueRef)value);

    return clearLastError(env);
}

NAPIStatus napi_set_named_property(NAPIEnv env, NAPIValue object, const char *utf8name, NAPIValue value)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)object), NAPIObjectExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)object, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(objectRef, NAPIMemoryError);

    JSStringRef stringRef = JSStringCreateWithUTF8CString(utf8name);
    RETURN_STATUS_IF_FALSE(stringRef, NAPIMemoryError);

    JSObjectSetProperty(env->context, objectRef, stringRef, (JSValueRef)value, kJSPropertyAttributeNone,
                        &env->lastException);
    JSStringRelease(stringRef);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus napi_has_named_property(NAPIEnv env, NAPIValue object, const char *utf8name, bool *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)object), NAPIObjectExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)object, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(objectRef, NAPIMemoryError);

    JSStringRef stringRef = JSStringCreateWithUTF8CString(utf8name);
    RETURN_STATUS_IF_FALSE(stringRef, NAPIMemoryError);

    *result = JSObjectHasProperty(env->context, objectRef, stringRef);
    JSStringRelease(stringRef);

    return clearLastError(env);
}

NAPIStatus napi_get_named_property(NAPIEnv env, NAPIValue object, const char *utf8name, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)object), NAPIObjectExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)object, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(objectRef, NAPIMemoryError);

    JSStringRef stringRef = JSStringCreateWithUTF8CString(utf8name);
    RETURN_STATUS_IF_FALSE(stringRef, NAPIMemoryError);

    *result = (NAPIValue)JSObjectGetProperty(env->context, objectRef, stringRef, &env->lastException);
    JSStringRelease(stringRef);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return clearLastError(env);
}

NAPIStatus napi_set_element(NAPIEnv env, NAPIValue object, uint32_t index, NAPIValue value)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)object), NAPIObjectExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)object, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(objectRef, NAPIMemoryError);

    JSObjectSetPropertyAtIndex(env->context, objectRef, index, (JSValueRef)value, &env->lastException);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus napi_has_element(NAPIEnv env, NAPIValue object, uint32_t index, bool *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)object), NAPIObjectExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)object, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(objectRef, NAPIMemoryError);

    JSValueRef value = JSObjectGetPropertyAtIndex(env->context, objectRef, index, &env->lastException);
    CHECK_JSC(env);

    // 不存在的属性会返回 undefined
    *result = !JSValueIsUndefined(env->context, value);

    return clearLastError(env);
}

NAPIStatus napi_get_element(NAPIEnv env, NAPIValue object, uint32_t index, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)object), NAPIObjectExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)object, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(objectRef, NAPIMemoryError);

    *result = (NAPIValue)JSObjectGetPropertyAtIndex(env->context, objectRef, index, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return clearLastError(env);
}

NAPIStatus napi_delete_element(NAPIEnv env, NAPIValue object, uint32_t index, bool *result)
{
    NAPI_PREAMBLE(env);

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)object), NAPIObjectExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)object, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(objectRef, NAPIMemoryError);

    JSValueRef indexValue = JSValueMakeNumber(env->context, index);
    JSStringRef indexStringRef = JSValueToStringCopy(env->context, indexValue, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(indexStringRef, NAPIMemoryError);

    *result = JSObjectDeleteProperty(env->context, objectRef, indexStringRef, &env->lastException);
    JSStringRelease(indexStringRef);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus napi_define_properties(NAPIEnv env, NAPIValue object, size_t property_count,
                                  const NAPIPropertyDescriptor *properties)
{
    NAPI_PREAMBLE(env);
    if (property_count > 0)
    {
        CHECK_ARG(env, properties);
    }
    for (size_t i = 0; i < property_count; i++)
    {
        const NAPIPropertyDescriptor *p = properties + i;

        NAPIValue descriptor;
        CHECK_NAPI(napi_create_object(env, &descriptor));

        NAPIValue configurable;
        CHECK_NAPI(napi_get_boolean(env, (p->attributes & NAPIConfigurable), &configurable));
        CHECK_NAPI(napi_set_named_property(env, descriptor, "configurable", configurable));

        NAPIValue enumerable;
        CHECK_NAPI(napi_get_boolean(env, (p->attributes & NAPIEnumerable), &enumerable));
        CHECK_NAPI(napi_set_named_property(env, descriptor, "enumerable", enumerable));

        if (p->getter || p->setter)
        {
            if (p->getter)
            {
                NAPIValue getter;
                CHECK_NAPI(napi_create_function(env, p->utf8name, NAPI_AUTO_LENGTH, p->getter, p->data, &getter));
                CHECK_NAPI(napi_set_named_property(env, descriptor, "get", getter));
            }
            if (p->setter)
            {
                NAPIValue setter;
                CHECK_NAPI(napi_create_function(env, p->utf8name, NAPI_AUTO_LENGTH, p->setter, p->data, &setter));
                CHECK_NAPI(napi_set_named_property(env, descriptor, "set", setter));
            }
        }
        else if (p->method)
        {
            NAPIValue method;
            CHECK_NAPI(napi_create_function(env, p->utf8name, NAPI_AUTO_LENGTH, p->method, p->data, &method));
            CHECK_NAPI(napi_set_named_property(env, descriptor, "value", method));
        }
        else
        {
            // value
            NAPIValue writable;
            CHECK_NAPI(napi_get_boolean(env, (p->attributes & NAPIWritable), &writable));
            CHECK_NAPI(napi_set_named_property(env, descriptor, "writable", writable));

            CHECK_NAPI(napi_set_named_property(env, descriptor, "value", p->value));
        }

        NAPIValue propertyName;
        if (!p->utf8name)
        {
            propertyName = p->name;
        }
        else
        {
            CHECK_NAPI(napi_create_string_utf8(env, p->utf8name, NAPI_AUTO_LENGTH, &propertyName));
        }

        NAPIValue global, objectCtor, function;
        CHECK_NAPI(napi_get_global(env, &global));
        CHECK_NAPI(napi_get_named_property(env, global, "Object", &objectCtor));
        // deployment target iOS 6
        CHECK_NAPI(napi_get_named_property(env, objectCtor, "defineProperty", &function));

        NAPIValue args[] = {object, propertyName, descriptor};
        CHECK_NAPI(napi_call_function(env, objectCtor, function, 3, args, NULL));
    }

    return clearLastError(env);
}

NAPIStatus napi_is_array(NAPIEnv env, NAPIValue value, bool *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, (JSValueRef)value);
    CHECK_ARG(env, result);

    *result = JSValueIsArray(env->context, (JSValueRef)value);

    return clearLastError(env);
}

NAPIStatus napi_get_array_length(NAPIEnv env, NAPIValue value, uint32_t *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)value), NAPIObjectExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)value, &env->lastException);
    RETURN_STATUS_IF_FALSE(objectRef, NAPIMemoryError);

    JSStringRef stringRef = JSStringCreateWithUTF8CString("length");
    RETURN_STATUS_IF_FALSE(stringRef, NAPIMemoryError);

    JSValueRef length = JSObjectGetProperty(env->context, objectRef, stringRef, &env->lastException);
    JSStringRelease(stringRef);
    CHECK_JSC(env);

    *result = (uint32_t)JSValueToNumber(env->context, length, &env->lastException);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus napi_strict_equals(NAPIEnv env, NAPIValue lhs, NAPIValue rhs, bool *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, lhs);
    CHECK_ARG(env, rhs);
    CHECK_ARG(env, result);

    *result = JSValueIsStrictEqual(env->context, (JSValueRef)lhs, (JSValueRef)rhs);

    return clearLastError(env);
}

NAPIStatus napi_call_function(NAPIEnv env, NAPIValue recv, NAPIValue func, size_t argc, const NAPIValue *argv,
                              NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, recv);
    if (argc > 0)
    {
        CHECK_ARG(env, argv);
    }

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)func), NAPIObjectExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)func, &env->lastException);
    RETURN_STATUS_IF_FALSE(objectRef, NAPIMemoryError);
    RETURN_STATUS_IF_FALSE(JSObjectIsFunction(env->context, objectRef), NAPIFunctionExpected);

    JSObjectRef thisObjectRef = NULL;
    if (JSValueIsObject(env->context, (JSValueRef)recv))
    {
        thisObjectRef = JSValueToObject(env->context, (JSValueRef)recv, &env->lastException);
        CHECK_JSC(env);
        RETURN_STATUS_IF_FALSE(thisObjectRef, NAPIMemoryError);
    }
    JSValueRef returnValue = NULL;
    if (argc <= 8)
    {
        JSValueRef argumentArray[argc];
        for (size_t i = 0; i < argc; ++i)
        {
            argumentArray[i] = (JSValueRef)argv[i];
        }
        returnValue =
            JSObjectCallAsFunction(env->context, objectRef, thisObjectRef, argc, argumentArray, &env->lastException);
    }
    else
    {
        JSValueRef *argumentArray = malloc(sizeof(JSValueRef) * argc);
        RETURN_STATUS_IF_FALSE(argumentArray, NAPIMemoryError);
        for (size_t i = 0; i < argc; ++i)
        {
            argumentArray[i] = (JSValueRef)argv[i];
        }
        returnValue =
            JSObjectCallAsFunction(env->context, objectRef, thisObjectRef, argc, argumentArray, &env->lastException);
    }
    CHECK_JSC(env);

    if (result)
    {
        RETURN_STATUS_IF_FALSE(returnValue, NAPIMemoryError);
        *result = (NAPIValue)returnValue;
    }

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

NAPIStatus napi_instanceof(NAPIEnv env, NAPIValue object, NAPIValue constructor, bool *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, object);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)constructor), NAPIObjectExpected);
    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)constructor, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(objectRef, NAPIMemoryError);
    RETURN_STATUS_IF_FALSE(JSObjectIsConstructor(env->context, objectRef), NAPIFunctionExpected);

    *result = JSValueIsInstanceOfConstructor(env->context, (JSValueRef)object, objectRef, &env->lastException);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus napi_get_cb_info(NAPIEnv env, NAPICallbackInfo cbinfo, size_t *argc, NAPIValue *argv, NAPIValue *thisArg,
                            void **data)
{
    CHECK_ENV(env);
    CHECK_ARG(env, cbinfo);

    if (argv)
    {
        CHECK_ARG(env, argc);

        size_t i = 0;
        size_t min = *argc > cbinfo->argc ? cbinfo->argc : *argc;

        for (; i < min; i++)
        {
            argv[i] = (NAPIValue)cbinfo->argv[i];
        }

        if (i < *argc)
        {
            for (; i < *argc; i++)
            {
                argv[i] = (NAPIValue)JSValueMakeUndefined(env->context);
            }
        }
    }

    if (argc)
    {
        *argc = cbinfo->argc;
    }

    if (thisArg)
    {
        *thisArg = (NAPIValue)cbinfo->thisArg;
    }

    if (data)
    {
        *data = cbinfo->data;
    }

    return clearLastError(env);
}

NAPIStatus napi_get_new_target(NAPIEnv env, NAPICallbackInfo cbinfo, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, cbinfo);
    CHECK_ARG(env, result);

    *result = (NAPIValue)cbinfo->newTarget;

    return clearLastError(env);
}

// Constructor

typedef struct
{
    // ExternalInfo
    FunctionInfo functionInfo;
    // ConstructorInfo
    JSClassRef classRef;
} ConstructorInfo;

static void constructorFinalize(JSObjectRef object)
{
    ConstructorInfo *info = JSObjectGetPrivate(object);
    if (info)
    {
        finalizeExternalInfo(&info->functionInfo.externalInfo);
        if (info->classRef)
        {
            // JSClassRelease 不能传递 NULL
            JSClassRelease(info->classRef);
        }
    }
    free(info);
}

static void externalFinalize(JSObjectRef object)
{
    ExternalInfo *info = JSObjectGetPrivate(object);
    // 调用 finalizer
    finalizeExternalInfo(info);
    free(info);
}

static inline JSClassRef createExternalClass()
{
    JSClassDefinition classDefinition = kJSClassDefinitionEmpty;
    classDefinition.attributes = kJSClassAttributeNoAutomaticPrototype;
    classDefinition.className = "External";
    classDefinition.finalize = externalFinalize;
    JSClassRef classRef = JSClassCreate(&classDefinition);

    return classRef;
}

static inline bool createExternalInfo(NAPIEnv env, void *data, ExternalInfo **result)
{
    if (!result)
    {
        return false;
    }
    ExternalInfo *externalInfo = malloc(sizeof(ExternalInfo));
    if (!externalInfo)
    {
        return false;
    }
    externalInfo->env = env;
    externalInfo->data = data;
    SLIST_INIT(&externalInfo->finalizerHead);
    externalInfo->finalizeCallback = NULL;
    externalInfo->finalizeHint = NULL;
    *result = externalInfo;

    return true;
}

static JSObjectRef callAsConstructor(JSContextRef ctx, JSObjectRef constructor, size_t argumentCount,
                                     const JSValueRef arguments[], JSValueRef *exception)
{
    // constructor 为当初定义的 JSObjectRef，而不是子类的构造函数
    JSValueRef prototype = JSObjectGetPrototype(ctx, constructor);
    if (!JSValueIsObject(ctx, prototype))
    {
        // 正常不应当出现
        assert(false);

        return NULL;
    }
    JSObjectRef prototypeObjectRef = JSValueToObject(ctx, prototype, exception);
    if (*exception)
    {
        // 正常不应当出现
        assert(false);

        return NULL;
    }
    ConstructorInfo *constructorInfo = JSObjectGetPrivate(prototypeObjectRef);
    if (!constructorInfo || !constructorInfo->functionInfo.externalInfo.env || !constructorInfo->functionInfo.callback)
    {
        // 正常不应当出现
        assert(false);

        return NULL;
    }

    clearLastError(constructorInfo->functionInfo.externalInfo.env);

    ExternalInfo *externalInfo;
    if (!createExternalInfo(constructorInfo->functionInfo.externalInfo.env, NULL, &externalInfo) || !externalInfo)
    {
        return NULL;
    }

    JSClassRef classRef = createExternalClass();
    if (!classRef)
    {
        free(externalInfo);

        return NULL;
    }
    JSObjectRef external = JSObjectMake(ctx, classRef, externalInfo);
    JSClassRelease(classRef);
    if (!external)
    {
        free(externalInfo);

        return NULL;
    }
    // constructorInfo->classRef 不存在也没有影响
    // 默认 instance.[[prototype]] 为 CallbackObject，JSObjectGetPrivate 不是
    // NULL，所以需要创建 External 插入原型链
    JSObjectRef instance = JSObjectMake(ctx, constructorInfo->classRef, NULL);
    if (!instance)
    {
        // 正常不应当出现
        assert(false);

        return NULL;
    }
    JSObjectSetPrototype(ctx, external,
                         JSObjectGetPrototype(constructorInfo->functionInfo.externalInfo.env->context, instance));
    JSObjectSetPrototype(ctx, instance, external);

    struct OpaqueNAPICallbackInfo callbackInfo = {NULL, NULL, NULL, NULL, 0};
    callbackInfo.newTarget = constructor;
    callbackInfo.thisArg = instance;
    callbackInfo.argc = argumentCount;
    callbackInfo.argv = arguments;
    callbackInfo.data = constructorInfo->functionInfo.externalInfo.data;

    JSValueRef returnValue = (JSValueRef)constructorInfo->functionInfo.callback(
        constructorInfo->functionInfo.externalInfo.env, &callbackInfo);

    bool isPending = false;
    if (napi_is_exception_pending(constructorInfo->functionInfo.externalInfo.env, &isPending) != NAPIOK)
    {
        assert(false);

        return NULL;
    }
    if (isPending)
    {
        if (napi_get_and_clear_last_exception(constructorInfo->functionInfo.externalInfo.env, (NAPIValue *)exception) !=
            NAPIOK)
        {
            // 正常不应当出现
            assert(false);
        }

        return NULL;
    }

    if (!JSValueIsObject(ctx, returnValue))
    {
        assert(false);

        return NULL;
    }
    JSObjectRef objectRef = JSValueToObject(ctx, returnValue, exception);
    if (*exception)
    {
        // 正常不应当出现
        assert(false);

        return NULL;
    }

    return objectRef;
}

NAPIStatus napi_define_class(NAPIEnv env, const char *utf8name, size_t length, NAPICallback constructor, void *data,
                             size_t propertyCount, const NAPIPropertyDescriptor *properties, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);
    CHECK_ARG(env, constructor);

    RETURN_STATUS_IF_FALSE(length == NAPI_AUTO_LENGTH, NAPIInvalidArg);

    if (propertyCount > 0)
    {
        CHECK_ARG(env, properties);
    }
    ConstructorInfo *constructorInfo = malloc(sizeof(ConstructorInfo));
    RETURN_STATUS_IF_FALSE(constructorInfo, NAPIMemoryError);
    constructorInfo->functionInfo.externalInfo.env = env;
    constructorInfo->functionInfo.callback = constructor;
    constructorInfo->functionInfo.externalInfo.data = data;
    constructorInfo->functionInfo.externalInfo.finalizeCallback = NULL;
    constructorInfo->functionInfo.externalInfo.finalizeHint = NULL;
    SLIST_INIT(&constructorInfo->functionInfo.externalInfo.finalizerHead);

    // JavaScriptCore Function 特殊点
    // 1. Function.prototype 不存在

    JSClassDefinition classDefinition = kJSClassDefinitionEmpty;
    // 提供 className 可以让 instance 显示类名
    // 不能使用 kJSClassAttributeNoAutomaticPrototype，因为所有 instance
    // 应当共享 Constructor.prototype
    classDefinition.className = utf8name;
    constructorInfo->classRef = JSClassCreate(&classDefinition);
    if (!constructorInfo->classRef)
    {
        free(constructorInfo);

        return setLastErrorCode(env, NAPIMemoryError);
    }

    classDefinition = kJSClassDefinitionEmpty;
    classDefinition.attributes = kJSClassAttributeNoAutomaticPrototype;
    classDefinition.finalize = constructorFinalize;
    JSClassRef prototypeClassRef = JSClassCreate(&classDefinition);
    if (!prototypeClassRef)
    {
        if (constructorInfo->classRef)
        {
            // JSClassCreate 可能返回 NULL
            JSClassRelease(constructorInfo->classRef);
        }
        free(constructorInfo);

        return setLastErrorCode(env, NAPIMemoryError);
    }

    // 正常原型链见
    // https://stackoverflow.com/questions/32928810/function-prototype-is-a-function

    // JavaScriptCore Constructor 特殊点
    // 1. Constructor.prototype 为 CallbackObject，CallbackObject.[[prototype]]
    // 为 Object.prototype
    // 2. Constructor -> CallbackObject -> Object
    // 3. CallbackObject 是一个不存在的类的实例
    // 4. CallbackObject 存在 privateData，会对 wrap 造成影响
    // 5. Constructor.name 不存在

    // 构造正常的原型链，应当如下
    // 1. Constructor.prototype 应当为 Object 的一个对象
    // 2. Constructor.prototype.constructor == Constructor
    // 3. Constructor.name 为类名

    // Constructor.[[prototype]] 正常应当指向 Function.prototype
    // 但是 JavaScriptCore 实现逻辑如下
    // 1. Constructor.[[prototype]] 会指向 Object.prototype，因为 typeof
    // Constructor === 'object'
    // 2. Constructor instanceof Function === false

    // 之所以区分 callAsConstructor 和 callAsFunction，主要就是为了 newTarget
    JSObjectRef prototype = JSObjectMake(env->context, prototypeClassRef, constructorInfo);
    JSClassRelease(prototypeClassRef);
    if (!prototype)
    {
        if (constructorInfo->classRef)
        {
            // JSClassCreate 可能返回 NULL
            JSClassRelease(constructorInfo->classRef);
        }
        free(constructorInfo);

        return setLastErrorCode(env, NAPIMemoryError);
    }
    JSObjectRef function = JSObjectMakeConstructor(env->context, constructorInfo->classRef, callAsConstructor);
    RETURN_STATUS_IF_FALSE(function, NAPIMemoryError);

    JSObjectSetPrototype(env->context, prototype, JSObjectGetPrototype(env->context, function));
    JSObjectSetPrototype(env->context, function, prototype);

    // 如果传入 NULL 类名，使用 ""
    // 忽略错误
    NAPIValue nameValue;
    napi_create_string_utf8(env, utf8name, NAPI_AUTO_LENGTH, &nameValue);
    if (!checkIsExceptionPendingAndClear(env))
    {
        // { configurable: true }
        NAPIPropertyDescriptor descriptor = {"name", NULL, NULL, NULL, NULL, nameValue, NAPIConfigurable, NULL};
        napi_define_properties(env, (NAPIValue)function, 1, &descriptor);
        checkIsExceptionPendingAndClear(env);
    }

    int instancePropertyCount = 0;
    int staticPropertyCount = 0;
    for (size_t i = 0; i < propertyCount; i++)
    {
        if ((properties[i].attributes & NAPIStatic) != 0)
        {
            staticPropertyCount++;
        }
        else
        {
            instancePropertyCount++;
        }
    }

    // TODO(ChasonTang): 栈分配
    NAPIPropertyDescriptor *staticDescriptors = NULL;
    if (staticPropertyCount > 0)
    {
        staticDescriptors = malloc(sizeof(NAPIPropertyDescriptor) * staticPropertyCount);
        if (!staticDescriptors)
        {
            return setLastErrorCode(env, NAPIMemoryError);
        }
    }

    // TODO(ChasonTang): 栈分配
    NAPIPropertyDescriptor *instanceDescriptors = NULL;
    if (instancePropertyCount > 0)
    {
        instanceDescriptors = malloc(sizeof(NAPIPropertyDescriptor) * instancePropertyCount);
        if (!instanceDescriptors)
        {
            free(staticDescriptors);

            return setLastErrorCode(env, NAPIMemoryError);
        }
    }

    size_t instancePropertyIndex = 0;
    size_t staticPropertyIndex = 0;

    for (size_t i = 0; i < propertyCount; i++)
    {
        if ((properties[i].attributes & NAPIStatic) != 0)
        {
            staticDescriptors[staticPropertyIndex] = properties[i];
            staticPropertyIndex += 1;
        }
        else
        {
            instanceDescriptors[instancePropertyIndex] = properties[i];
            instancePropertyIndex += 1;
        }
    }

    if (staticPropertyCount > 0)
    {
        NAPIStatus status = napi_define_properties(env, (NAPIValue)function, staticPropertyIndex, staticDescriptors);
        if (status != NAPIOK)
        {
            free(staticDescriptors);
            free(instanceDescriptors);

            return status;
        }
    }

    free(staticDescriptors);

    if (instancePropertyCount > 0)
    {
        NAPIValue prototypeValue;
        NAPIStatus status = napi_get_named_property(env, (NAPIValue)function, "prototype", &prototypeValue);
        if (status != NAPIOK)
        {
            free(instanceDescriptors);

            return status;
        }
        status = napi_define_properties(env, prototypeValue, instancePropertyIndex, instanceDescriptors);
        if (status != NAPIOK)
        {
            free(instanceDescriptors);

            return status;
        }
    }
    free(instanceDescriptors);

    *result = (NAPIValue)function;

    return clearLastError(env);
}

// External

static inline NAPIStatus unwrap(NAPIEnv env, NAPIValue object, ExternalInfo **result)
{
    NAPI_PREAMBLE(env);

    NAPIValue prototype;
    CHECK_NAPI(napi_get_prototype(env, object, &prototype));

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)prototype), NAPIObjectExpected);
    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)prototype, &env->lastException);
    CHECK_JSC(env);

    *result = JSObjectGetPrivate(objectRef);

    return clearLastError(env);
}

static inline NAPIStatus wrap(NAPIEnv env, NAPIValue object, ExternalInfo **result)
{
    NAPI_PREAMBLE(env);

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)object), NAPIObjectExpected);
    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)object, &env->lastException);
    CHECK_JSC(env);

    ExternalInfo *info = NULL;
    CHECK_NAPI(unwrap(env, object, &info));
    if (!info)
    {
        if (!createExternalInfo(env, NULL, &info) || !info)
        {
            return setLastErrorCode(env, NAPIMemoryError);
        }

        JSClassRef classRef = createExternalClass();
        if (!classRef)
        {
            free(info);

            return setLastErrorCode(env, NAPIMemoryError);
        }
        JSObjectRef prototype = JSObjectMake(env->context, classRef, info);
        JSClassRelease(classRef);
        if (!prototype)
        {
            free(info);

            return setLastErrorCode(env, NAPIMemoryError);
        }
        JSObjectSetPrototype(env->context, prototype, JSObjectGetPrototype(env->context, objectRef));
        JSObjectSetPrototype(env->context, objectRef, prototype);
    }

    *result = info;

    return clearLastError(env);
}

static bool addFinalizer(ExternalInfo *externalInfo, NAPIFinalize finalize, void *finalizeHint)
{
    if (!externalInfo)
    {
        return false;
    }

    struct Finalizer *finalizer = malloc(sizeof(struct Finalizer));
    if (!finalizer)
    {
        return false;
    }
    finalizer->finalizeCallback = finalize;
    finalizer->finalizeHint = finalizeHint;

    SLIST_INSERT_HEAD(&externalInfo->finalizerHead, finalizer, node);

    return true;
}

NAPIStatus napi_wrap(NAPIEnv env, NAPIValue jsObject, void *nativeObject, NAPIFinalize finalizeCallback,
                     void *finalizeHint, NAPIRef *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, jsObject);

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)jsObject), NAPIObjectExpected);

    ExternalInfo *info = NULL;
    CHECK_NAPI(wrap(env, jsObject, &info));
    RETURN_STATUS_IF_FALSE(info, NAPIMemoryError);
    // If we've already wrapped this object, we error out.
    RETURN_STATUS_IF_FALSE(!info->data, NAPIInvalidArg);
    RETURN_STATUS_IF_FALSE(!info->finalizeCallback, NAPIInvalidArg);
    RETURN_STATUS_IF_FALSE(!info->finalizeHint, NAPIInvalidArg);
    info->data = nativeObject;

    if (finalizeCallback)
    {
        info->finalizeCallback = finalizeCallback;
        info->finalizeHint = finalizeHint;
    }

    // 当 napi_create_reference 失败的时候可以不回滚前面的 wrap
    // 过程，因为这实际上是两步骤
    if (result)
    {
        CHECK_NAPI(napi_create_reference(env, jsObject, 0, result));
    }

    return clearLastError(env);
}

NAPIStatus napi_unwrap(NAPIEnv env, NAPIValue jsObject, void **result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, jsObject);
    CHECK_ARG(env, result);

    ExternalInfo *info = NULL;
    CHECK_NAPI(unwrap(env, jsObject, &info));
    RETURN_STATUS_IF_FALSE(info && info->data, NAPIInvalidArg);
    *result = info->data;

    return clearLastError(env);
}

NAPIStatus napi_remove_wrap(NAPIEnv env, NAPIValue jsObject, void **result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, jsObject);
    CHECK_ARG(env, result);

    ExternalInfo *info = NULL;
    CHECK_NAPI(unwrap(env, jsObject, &info));
    RETURN_STATUS_IF_FALSE(info && info->data, NAPIInvalidArg);
    *result = info->data;
    info->data = NULL;
    // 清除回调
    info->finalizeCallback = NULL;
    info->finalizeHint = NULL;

    return clearLastError(env);
}

static inline NAPIStatus create(NAPIEnv env, void *data, NAPIFinalize finalizeCB, void *finalizeHint, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    ExternalInfo *info;
    if (!createExternalInfo(env, data, &info) || !info)
    {
        return setLastErrorCode(env, NAPIMemoryError);
    }

    JSClassRef classRef = createExternalClass();
    if (!classRef)
    {
        free(info);

        return setLastErrorCode(env, NAPIMemoryError);
    }
    if (finalizeCB)
    {
        if (!addFinalizer(info, finalizeCB, finalizeHint))
        {
            JSClassRelease(classRef);
            free(info);

            return setLastErrorCode(env, NAPIMemoryError);
        }
    }
    *result = (NAPIValue)JSObjectMake(env->context, classRef, info);
    JSClassRelease(classRef);

    return clearLastError(env);
}

NAPIStatus napi_create_external(NAPIEnv env, void *data, NAPIFinalize finalizeCB, void *finalizeHint, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    CHECK_NAPI(create(env, data, finalizeCB, finalizeHint, result));

    return clearLastError(env);
}

NAPIStatus napi_get_value_external(NAPIEnv env, NAPIValue value, void **result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)value), NAPIObjectExpected);
    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)value, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(objectRef, NAPIMemoryError);

    ExternalInfo *info = JSObjectGetPrivate(objectRef);
    *result = info ? info->data : NULL;

    return clearLastError(env);
}

typedef struct
{
    JSValueRef value;
    UT_hash_handle hh;
} ActiveReferenceValue;

static ActiveReferenceValue *activeReferenceValues;

static void referenceFinalize(__attribute((unused)) NAPIEnv env, __attribute((unused)) void *finalizeData,
                              void *finalizeHint)
{
    // 这里很可能 env 已经不存在了，不要使用 env
    ActiveReferenceValue *activeReferenceValue = NULL;
    // finalizeHint 实际上就是 JSValueRef
    // Allocation failure is possible only when adding elements to the hash
    // table (including the ADD, REPLACE, and SELECT operations). uthash_free is
    // not allowed to fail.
    HASH_FIND_PTR(activeReferenceValues, &finalizeHint, activeReferenceValue);
    if (activeReferenceValue)
    {
        HASH_DEL(activeReferenceValues, activeReferenceValue);
        free(activeReferenceValue);
    }
    else
    {
        // 正常销毁的时候应该存在
        assert(false);
    }
}

static inline void protect(NAPIEnv env, NAPIRef reference)
{
    if (!env)
    {
        assert(false);

        return;
    }
    LIST_INSERT_HEAD(&env->strongReferenceHead, reference, node);
    JSValueProtect(env->context, (reference)->value);
}

NAPIStatus napi_create_reference(NAPIEnv env, NAPIValue value, uint32_t initialRefCount, NAPIRef *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    NAPIRef reference = malloc(sizeof(struct OpaqueNAPIRef));
    RETURN_STATUS_IF_FALSE(reference, NAPIMemoryError);
    reference->value = (JSValueRef)value;
    reference->count = initialRefCount;

    ActiveReferenceValue *activeReferenceValue = NULL;
    HASH_FIND_PTR(activeReferenceValues, &value, activeReferenceValue);
    if (!activeReferenceValue)
    {
        activeReferenceValue = malloc(sizeof(ActiveReferenceValue));
        if (!activeReferenceValue)
        {
            free(reference);

            return setLastErrorCode(env, NAPIMemoryError);
        }
        activeReferenceValue->value = (JSValueRef)value;
        hashError = false;
        HASH_ADD_PTR(activeReferenceValues, value, activeReferenceValue);
        if (hashError)
        {
            hashError = false;
            free(reference);
            free(activeReferenceValue);

            return setLastErrorCode(env, NAPIMemoryError);
        }
        // wrap
        ExternalInfo *externalInfo = NULL;
        NAPIStatus status = wrap(env, value, &externalInfo);
        if (status != NAPIOK)
        {
            // 失败
            free(reference);
            HASH_DEL(activeReferenceValues, activeReferenceValue);
            free(activeReferenceValue);

            return status;
        }
        if (!addFinalizer(externalInfo, referenceFinalize, value))
        {
            // 失败
            free(reference);
            HASH_DEL(activeReferenceValues, activeReferenceValue);
            free(activeReferenceValue);

            return setLastErrorCode(env, NAPIMemoryError);
        }
    }

    if (initialRefCount)
    {
        protect(env, reference);
    }
    *result = reference;

    return clearLastError(env);
}

static inline void unprotect(NAPIEnv env, NAPIRef reference)
{
    if (!env)
    {
        assert(false);

        return;
    }
    LIST_REMOVE(reference, node);
    JSValueUnprotect(env->context, (reference)->value);
}

NAPIStatus napi_delete_reference(NAPIEnv env, NAPIRef ref)
{
    CHECK_ENV(env);
    CHECK_ARG(env, ref);

    if (ref->count)
    {
        unprotect(env, ref);
    }

    free(ref);

    return clearLastError(env);
}

NAPIStatus napi_reference_ref(NAPIEnv env, NAPIRef ref, uint32_t *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, ref);

    if (ref->count++ == 0)
    {
        protect(env, ref);
    }

    if (result)
    {
        *result = ref->count;
    }

    return clearLastError(env);
}

NAPIStatus napi_reference_unref(NAPIEnv env, NAPIRef ref, uint32_t *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, ref);

    if (--ref->count == 0)
    {
        unprotect(env, ref);
    }

    if (result)
    {
        *result = ref->count;
    }

    return clearLastError(env);
}

NAPIStatus napi_get_reference_value(NAPIEnv env, NAPIRef ref, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, ref);
    CHECK_ARG(env, result);

    ActiveReferenceValue *activeReferenceValue = NULL;
    HASH_FIND_PTR(activeReferenceValues, &ref->value, activeReferenceValue);
    if (activeReferenceValue)
    {
        *result = (NAPIValue)ref->value;
    }

    return clearLastError(env);
}

NAPIStatus napi_open_handle_scope(NAPIEnv env, NAPIHandleScope *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIHandleScope)1;

    return clearLastError(env);
}

NAPIStatus napi_close_handle_scope(NAPIEnv env, NAPIHandleScope scope)
{
    CHECK_ENV(env);
    CHECK_ARG(env, scope);

    return clearLastError(env);
}

struct OpaqueNAPIEscapableHandleScope
{
    bool escapeCalled;
};

NAPIStatus napi_open_escapable_handle_scope(NAPIEnv env, NAPIEscapableHandleScope *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = malloc(sizeof(struct OpaqueNAPIEscapableHandleScope));
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);
    (*result)->escapeCalled = false;

    return clearLastError(env);
}

NAPIStatus napi_close_escapable_handle_scope(NAPIEnv env, NAPIEscapableHandleScope scope)
{
    CHECK_ENV(env);
    CHECK_ARG(env, scope);

    free(scope);

    return clearLastError(env);
}

NAPIStatus napi_escape_handle(NAPIEnv env, NAPIEscapableHandleScope scope, NAPIValue escapee, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, scope);
    CHECK_ARG(env, escapee);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(!scope->escapeCalled, NAPIEscapeCalledTwice);
    scope->escapeCalled = true;
    *result = escapee;

    return clearLastError(env);
}

NAPIStatus napi_throw(NAPIEnv env, NAPIValue error)
{
    CHECK_ENV(env);
    CHECK_ARG(env, error);

    env->lastException = (JSValueRef)error;

    return clearLastError(env);
}

NAPIStatus napi_throw_error(NAPIEnv env, const char *code, const char *msg)
{
    CHECK_ENV(env);

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

NAPIStatus napi_throw_type_error(NAPIEnv env, const char *code, const char *msg)
{
    CHECK_ENV(env);

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
    CHECK_NAPI(napi_create_type_error(env, (NAPIValue)codeValue, (NAPIValue)msgValue, &error));

    return napi_throw(env, error);
}

NAPIStatus napi_throw_range_error(NAPIEnv env, const char *code, const char *msg)
{
    CHECK_ENV(env);

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
    CHECK_NAPI(napi_create_range_error(env, (NAPIValue)codeValue, (NAPIValue)msgValue, &error));

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

NAPIStatus napi_is_exception_pending(NAPIEnv env, bool *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = env->lastException;

    return clearLastError(env);
}

NAPIStatus napi_get_and_clear_last_exception(NAPIEnv env, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    if (!env->lastException)
    {
        return napi_get_undefined(env, result);
    }
    else
    {
        *result = (NAPIValue)env->lastException;
        env->lastException = NULL;
    }

    return clearLastError(env);
}

typedef struct
{
    JSValueRef resolve;
    JSValueRef reject;
} Wrapper;

static NAPIValue callback(__attribute((unused)) NAPIEnv env, NAPICallbackInfo callbackInfo)
{
    Wrapper *wrapper = (Wrapper *)callbackInfo->data;
    wrapper->resolve = callbackInfo->argv[0];
    wrapper->reject = callbackInfo->argv[1];

    return NULL;
}

NAPIStatus napi_create_promise(NAPIEnv env, NAPIDeferred *deferred, NAPIValue *promise)
{
    CHECK_ENV(env);
    CHECK_ARG(env, deferred);
    CHECK_ARG(env, promise);

    NAPIValue global;
    NAPIValue promiseCtor;
    CHECK_NAPI(napi_get_global(env, &global));
    // deployment target iOS 8
    CHECK_NAPI(napi_get_named_property(env, global, "Promise", &promiseCtor));

    Wrapper wrapper;

    NAPIValue executor;
    CHECK_NAPI(napi_create_function(env, "executor", NAPI_AUTO_LENGTH, callback, &wrapper, &executor));
    CHECK_NAPI(napi_new_instance(env, promiseCtor, 1, &executor, promise));

    NAPIValue deferredValue;
    CHECK_NAPI(napi_create_object(env, &deferredValue));
    CHECK_NAPI(napi_set_named_property(env, deferredValue, "resolve", (NAPIValue)wrapper.resolve));
    CHECK_NAPI(napi_set_named_property(env, deferredValue, "reject", (NAPIValue)wrapper.reject));

    NAPIRef deferredRef = NULL;
    CHECK_NAPI(napi_create_reference(env, deferredValue, 1, &deferredRef));
    *deferred = (NAPIDeferred)(deferredRef);

    return clearLastError(env);
}

NAPIStatus napi_resolve_deferred(NAPIEnv env, NAPIDeferred deferred, NAPIValue resolution)
{
    CHECK_ENV(env);
    CHECK_ARG(env, deferred);
    CHECK_ARG(env, resolution);

    NAPIRef deferredRef = (NAPIRef)deferred;
    NAPIValue undefined;
    NAPIValue deferredValue;
    NAPIValue resolve;
    CHECK_NAPI(napi_get_undefined(env, &undefined));
    CHECK_NAPI(napi_get_reference_value(env, deferredRef, &deferredValue));
    CHECK_NAPI(napi_get_named_property(env, deferredValue, "resolve", &resolve));
    CHECK_NAPI(napi_call_function(env, undefined, resolve, 1, &resolution, NULL));
    CHECK_NAPI(napi_delete_reference(env, deferredRef));

    return clearLastError(env);
}

NAPIStatus napi_reject_deferred(NAPIEnv env, NAPIDeferred deferred, NAPIValue rejection)
{
    CHECK_ENV(env);
    CHECK_ARG(env, deferred);
    CHECK_ARG(env, rejection);

    NAPIRef deferredRef = (NAPIRef)deferred;
    NAPIValue undefined, deferredValue, reject;
    CHECK_NAPI(napi_get_undefined(env, &undefined));
    CHECK_NAPI(napi_get_reference_value(env, deferredRef, &deferredValue));
    CHECK_NAPI(napi_get_named_property(env, deferredValue, "reject", &reject));
    CHECK_NAPI(napi_call_function(env, undefined, reject, 1, &rejection, NULL));
    CHECK_NAPI(napi_delete_reference(env, deferredRef));

    return clearLastError(env);
}

NAPIStatus napi_is_promise(NAPIEnv env, NAPIValue value, bool *isPromise)
{
    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, isPromise);

    NAPIValue global, promiseCtor;
    CHECK_NAPI(napi_get_global(env, &global));
    CHECK_NAPI(napi_get_named_property(env, global, "Promise", &promiseCtor));
    CHECK_NAPI(napi_instanceof(env, value, promiseCtor, isPromise));

    return clearLastError(env);
}

NAPIStatus napi_run_script(NAPIEnv env, NAPIValue script, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, script);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(JSValueIsString(env->context, (JSValueRef)script), NAPIStringExpected);

    JSStringRef script_str = JSValueToStringCopy(env->context, (JSValueRef)script, &env->lastException);
    CHECK_JSC(env);

    *result = (NAPIValue)JSEvaluateScript(env->context, script_str, NULL, NULL, 1, &env->lastException);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus NAPIRunScriptWithSourceUrl(NAPIEnv env, const char *utf8Script, const char *utf8SourceUrl, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, utf8Script);
    CHECK_ARG(env, result);

    JSStringRef scriptStringRef = JSStringCreateWithUTF8CString(utf8Script);
    JSStringRef sourceUrl = NULL;
    if (utf8SourceUrl)
    {
        sourceUrl = JSStringCreateWithUTF8CString(utf8SourceUrl);
    }

    *result = (NAPIValue)JSEvaluateScript(env->context, scriptStringRef, NULL, sourceUrl, 1, &env->lastException);
    CHECK_JSC(env);

    return clearLastError(env);
}

static JSContextGroupRef virtualMachine = NULL;

static uint8_t contextCount = 0;

NAPIStatus NAPICreateEnv(NAPIEnv *env)
{
    // *env 才是 NAPIEnv
    if (!env)
    {
        return NAPIInvalidArg;
    }

    if ((virtualMachine && !contextCount) || (!virtualMachine && contextCount) ||
        (!virtualMachine && activeReferenceValues) || (!activeReferenceValues && virtualMachine))
    {
        assert(false);

        return NAPIGenericFailure;
    }

    *env = malloc(sizeof(struct OpaqueNAPIEnv));
    if (!*env)
    {
        return NAPIMemoryError;
    }

    if (!virtualMachine)
    {
        virtualMachine = JSContextGroupCreate();
        if (!virtualMachine)
        {
            free(*env);

            return NAPIMemoryError;
        }
        activeReferenceValues = NULL;
    }
    JSGlobalContextRef globalContext = JSGlobalContextCreateInGroup(virtualMachine, NULL);
    if (!globalContext)
    {
        free(*env);
        JSContextGroupRelease(virtualMachine);
        virtualMachine = NULL;

        return NAPIMemoryError;
    }
    contextCount += 1;
    (*env)->context = globalContext;
    (*env)->lastException = NULL;
    LIST_INIT(&(*env)->strongReferenceHead);

    return NAPIOK;
}

NAPIStatus NAPIFreeEnv(NAPIEnv env)
{
    CHECK_ENV(env);

    NAPIRef reference, tempReference;
    LIST_FOREACH_SAFE(reference, &env->strongReferenceHead, node, tempReference)
    {
        LIST_REMOVE(reference, node);
        if (reference->count)
        {
            unprotect(env, reference);
        }
        free(reference);
    }
    // LIST_REMOVE 会清空 head，所以不需要 LIST_EMPTY
    //    LIST_EMPTY(&env->strongReferenceHead);

    JSGlobalContextRelease(env->context);
    if (--contextCount == 0 && virtualMachine)
    {
        // virtualMachine 不能为 NULL
        JSContextGroupRelease(virtualMachine);
        virtualMachine = NULL;

        // https://github.com/troydhanson/uthash/issues/221
        ActiveReferenceValue *first = activeReferenceValues;
        ActiveReferenceValue *element, *temp;
        HASH_ITER(hh, activeReferenceValues, element, temp)
        {
            if (element != first)
            {
                free(element);
            }
        }
        HASH_CLEAR(hh, activeReferenceValues);
        free(first);
    }

    free(env);

    return NAPIOK;
}
