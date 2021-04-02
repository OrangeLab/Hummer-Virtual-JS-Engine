//
//  js_native_api_jsc.c
//  NAPI
//
//  Created by 唐佳诚 on 2021/2/23.
//

// 如果一个函数返回的 NAPIStatus 为 NAPIOk，则没有异常被挂起，并且不需要做任何处理
// 如果 NAPIStatus 是除了 NAPIOk 或者 NAPIPendingException，为了尝试恢复并继续执行，而不是直接返回，必须调用 napi_is_exception_pending 来确定是否存在挂起的异常

// 大多数情况下，调用 N-API 函数的时候，如果已经有异常被挂起，函数会立即返回 NAPIPendingException，但是，N-API 允许一些函数被调用用来在返回 JavaScript 环境前做最小化的清理
// 在这种情况下，NAPIStatus 会反映函数的执行结果状态，而不是当前存在异常，为了避免混淆，在每次函数执行后请检查结果

// 当异常被挂起的时候，有两种方法
// 1. 进行适当的清理并返回，那么执行环境会返回到 JavaScript，作为返回到 JavaScript 的过渡的一部分，异常将在 JavaScript 调用原生方法的语句处抛出，大多数 N-API 函数在出现异常被挂起的时候，都只是简单的返回 NAPIPendingException，，所以在这种情况下，建议尽量少做事情，等待返回到 JavaScript 中处理异常
// 2. 尝试处理异常，在原生代码能捕获到异常的地方采取适当的操作，然后继续执行。只有当异常能被安全的处理的少数特定情况下推荐这样做，在这些情况下，napi_get_and_clear_last_exception 可以被用来获取并清除异常，在此之后，如果发现异常还是无法处理，可以通过 napi_throw 重新抛出错误

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <JavaScriptCore/JavaScriptCore.h>

#include <napi/js_native_api.h>

struct OpaqueNAPIEnv
{
    JSContextRef context;
    // undefined 和 null 实际上也可以当做 exception 抛出，所以异常检查只需要检查是否为 NULL
    JSValueRef lastException;
    NAPIExtendedErrorInfo lastError;
};

static inline NAPIStatus clearLastError(NAPIEnv env)
{
    if (env)
    {
        env->lastError.errorCode = NAPIOk;

        // TODO(boingoing): Should this be a callback?
        env->lastError.engineErrorCode = 0;
        env->lastError.engineReserved = NULL;
    }

    return NAPIOk;
}

// static inline NAPIStatus setException(NAPIEnv env, JSValueRef exception)
// {
//     if (env)
//     {
//         env->lastException = exception;
//     }

//     return setLastErrorCode(env, NAPIPendingException);
// }

static inline NAPIStatus setLastErrorCode(NAPIEnv env, NAPIStatus errorCode)
{
    if (env)
    {
        env->lastError.errorCode = errorCode;
    }

    return errorCode;
}

#define RETURN_STATUS_IF_FALSE(env, condition, status) \
    do                                                 \
    {                                                  \
        if (!(condition))                              \
        {                                              \
            return setLastErrorCode((env), (status));  \
        }                                              \
    } while (0)

#define CHECK_ARG(env, arg) RETURN_STATUS_IF_FALSE((env), ((arg) != NULL), NAPIInvalidArg)

// This does not call napi_set_last_error because the expression
// is assumed to be a NAPI function call that already did.
#define CHECK_NAPI(expr)            \
    do                              \
    {                               \
        NAPIStatus status = (expr); \
        if (status != NAPIOk)       \
        {                           \
            return status;          \
        }                           \
    } while (0)

#define CHECK_ENV(env)             \
    do                             \
    {                              \
        if ((env) == NULL)         \
        {                          \
            return NAPIInvalidArg; \
        }                          \
    } while (0)

#define NAPI_PREAMBLE(env)                                                                     \
    do                                                                                         \
    {                                                                                          \
        CHECK_ENV((env));                                                                      \
        RETURN_STATUS_IF_FALSE((env), (((env)->lastException) != NULL), NAPIPendingException); \
        clearLastError((env));                                                                 \
    } while (0)

#define CHECK_JSC(env)                                                             \
    do                                                                             \
    {                                                                              \
        RETURN_STATUS_IF_FALSE((env), (env->lastException), NAPIPendingException); \
    } while (0)

static inline NAPIStatus setErrorCode(NAPIEnv env, NAPIValue error, NAPIValue code, const char *codeCString)
{
    if (!code && !codeCString)
    {
        return NAPIOk;
    }
    CHECK_ENV(env);

    NAPIValue codeValue = code;
    if (codeValue == NULL)
    {
        JSStringRef stringRef = codeCString ? JSStringCreateWithUTF8CString(codeCString) : NULL;
        codeValue = (NAPIValue)JSValueMakeString(env->context, stringRef);
        if (stringRef)
        {
            JSStringRelease(stringRef);
        }
    }
    else
    {
        RETURN_STATUS_IF_FALSE(env, JSValueIsString(env->context, (JSValueRef)codeValue), NAPIStringExpected);
    }

    CHECK_NAPI(napi_set_named_property(env, error, "code", codeValue));

    return clearLastError(env);
}

// Warning: Keep in-sync with NAPIStatus enum

static const char *errorMessages[] = {
    NULL,
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
};

// The value of the constant below must be updated to reference the last
// message in the `napi_status` enum each time a new error message is added.
// We don't have a napi_status_last as this would result in an ABI
// change each time a message was added.
#define LAST_STATUS NAPIWouldDeadLock

NAPIStatus napi_get_last_error_info(NAPIEnv env, const NAPIExtendedErrorInfo **result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    static_assert(sizeof(errorMessages) / sizeof(*errorMessages) == LAST_STATUS + 1, "Count of error messages must match count of error values");
    if (env->lastError.errorCode <= LAST_STATUS)
    {
        // Wait until someone requests the last error information to fetch the error
        // message string
        env->lastError.errorMessage = errorMessages[env->lastError.errorCode];
    }
    else
    {
        // 顶多缺失 errorMessage，尽量不要 Abort
        assert(false);
    }

    *result = &(env->lastError);

    return clearLastError(env);
}

NAPIStatus napi_get_undefined(NAPIEnv env, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue)JSValueMakeUndefined(env->context);

    return clearLastError(env);
}

NAPIStatus napi_get_null(NAPIEnv env, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue)JSValueMakeNull(env->context);

    return clearLastError(env);
}

NAPIStatus napi_get_global(NAPIEnv env, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue)JSContextGetGlobalObject(env->context);

    return clearLastError(env);
}

NAPIStatus napi_get_boolean(NAPIEnv env, bool value, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue)JSValueMakeBoolean(env->context, value);

    return clearLastError(env);
}

NAPIStatus napi_create_object(NAPIEnv env, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue)JSObjectMake(env->context, NULL, NULL);

    return clearLastError(env);
}

NAPIStatus napi_create_array(NAPIEnv env, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue)JSObjectMakeArray(env->context, 0, NULL, &env->lastException);
    CHECK_JSC(env);

    return clearLastError(env);
}

// 本质为 const array = new Array(); array.length = length;
NAPIStatus napi_create_array_with_length(NAPIEnv env, size_t length, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue)JSObjectMakeArray(env->context, 0, NULL, &env->lastException);
    CHECK_JSC(env);

    JSStringRef lengthStringRef = JSStringCreateWithUTF8CString("length");
    JSObjectSetProperty(env->context, (JSObjectRef)*result, lengthStringRef, JSValueMakeNumber(env->context, length), kJSPropertyAttributeNone, &env->lastException);
    JSStringRelease(lengthStringRef);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus napi_create_double(NAPIEnv env, double value, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue)JSValueMakeNumber(env->context, value);

    return clearLastError(env);
}

NAPIStatus napi_create_int32(NAPIEnv env, int32_t value, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue)JSValueMakeNumber(env->context, value);

    return clearLastError(env);
}

NAPIStatus napi_create_uint32(NAPIEnv env, uint32_t value, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue)JSValueMakeNumber(env->context, value);

    return clearLastError(env);
}

NAPIStatus napi_create_int64(NAPIEnv env, int64_t value, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue)JSValueMakeNumber(env->context, value);

    return clearLastError(env);
}

// 未实现
NAPIStatus napi_create_string_latin1(NAPIEnv env, const char *str, size_t length, NAPIValue *result)
{
    return setLastErrorCode(env, NAPIGenericFailure);
}

// JavaScriptCore 只能接受 \0 结尾的字符串
// 传入 str NULL 则为 ""
NAPIStatus napi_create_string_utf8(NAPIEnv env, const char *str, size_t length, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env, length == NAPI_AUTO_LENGTH, NAPIInvalidArg);

    JSStringRef stringRef = JSStringCreateWithUTF8CString(str);
    *result = (NAPIValue)JSValueMakeString(env->context, stringRef);
    JSStringRelease(stringRef);

    return clearLastError(env);
}

NAPIStatus napi_create_string_utf16(NAPIEnv env, const uint_least16_t *str, size_t length, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    static_assert(sizeof(uint_least16_t) == sizeof(JSChar), "uint_least16_t size not equal JSChar");

    // Node.js 默认判断
    RETURN_STATUS_IF_FALSE(env, length <= INT_MAX && length != NAPI_AUTO_LENGTH, NAPIInvalidArg);

    JSStringRef stringRef = JSStringCreateWithCharacters(str, length);
    *result = (NAPIValue)JSValueMakeString(env->context, stringRef);
    JSStringRelease(stringRef);

    return clearLastError(env);
}

NAPIStatus napi_create_symbol(NAPIEnv env, NAPIValue description, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    NAPIValue global = NULL;
    NAPIValue symbolFunc = NULL;
    CHECK_NAPI(napi_get_global(env, &global));
    CHECK_NAPI(napi_get_named_property(env, global, "Symbol", &symbolFunc));
    CHECK_NAPI(napi_call_function(env, global, symbolFunc, 1, &description, result));

    return clearLastError(env);
}

typedef enum
{
    External,
    Function,
    Constructor,
} NativeType;

// nativeType 必须是第一个
typedef struct
{
    NAPIEnv env;
    NAPICallback callback;
    void *data;
} FunctionInfo;

static void finalize(JSObjectRef object)
{
    FunctionInfo *functionInfo = JSObjectGetPrivate(object);
    if (functionInfo)
    {
        free(functionInfo);
    }
}

struct OpaqueNAPICallbackInfo
{
    JSObjectRef newTarget;
    JSObjectRef thisArg;
    size_t argc;
    const JSValueRef *argv;
    void *data;
};

// 所有参数都会存在，返回值可以为 NULL
static JSValueRef callAsFunction(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception)
{
    JSValueRef prototype = JSObjectGetPrototype(ctx, function);
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
    FunctionInfo *functionInfo = JSObjectGetPrivate(prototypeObjectRef);
    if (!functionInfo)
    {
        // 正常不应当出现
        assert(false);

        return NULL;
    }
    struct OpaqueNAPICallbackInfo callbackInfo = {};
    callbackInfo.newTarget = NULL;
    callbackInfo.thisArg = thisObject;
    callbackInfo.argc = argumentCount;
    callbackInfo.argv = arguments;
    callbackInfo.data = functionInfo->data;

    return (JSValueRef)functionInfo->callback(functionInfo->env, &callbackInfo);
}

NAPIStatus napi_create_function(NAPIEnv env, const char *utf8name, size_t length, NAPICallback cb, void *data, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, cb);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env, length == NAPI_AUTO_LENGTH, NAPIInvalidArg);

    errno = 0;
    FunctionInfo *functionInfo = malloc(sizeof(FunctionInfo));
    if (errno == ENOMEM)
    {
        errno = 0;

        return setLastErrorCode(env, NAPIGenericFailure);
    }
    functionInfo->env = env;
    functionInfo->callback = cb;
    functionInfo->data = data;

    JSClassDefinition classDefinition = kJSClassDefinitionEmpty;
    // 使用 Object.prototype 作为实例的 [[prototype]]
    classDefinition.attributes = kJSClassAttributeNoAutomaticPrototype;
    classDefinition.finalize = finalize;
    JSClassRef classRef = JSClassCreate(&classDefinition);
    JSObjectRef prototype = JSObjectMake(env->context, classRef, functionInfo);
    JSClassRelease(classRef);

    // 函数名如果为 NULL 则为匿名函数
    JSStringRef stringRef = utf8name ? JSStringCreateWithUTF8CString(utf8name) : NULL;
    *result = (NAPIValue)JSObjectMakeFunctionWithCallback(env->context, stringRef, callAsFunction);
    if (stringRef)
    {
        JSStringRelease(stringRef);
    }

    JSObjectSetPrototype(env->context, prototype, JSObjectGetPrototype(env->context, (JSObjectRef)*result));
    JSObjectSetPrototype(env->context, (JSObjectRef)*result, prototype);

    return clearLastError(env);
}

NAPIStatus napi_create_error(NAPIEnv env, NAPIValue code, NAPIValue msg, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, msg);
    CHECK_ARG(env, result);

    *result = (NAPIValue)JSObjectMakeError(env->context, 1, (JSValueRef *)&msg, &env->lastException);
    CHECK_JSC(env);

    CHECK_NAPI(setErrorCode(env, *result, code, NULL));

    return clearLastError(env);
}

NAPIStatus napi_create_type_error(NAPIEnv env, NAPIValue code, NAPIValue msg, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, msg);
    CHECK_ARG(env, result);

    NAPIValue global = NULL;
    NAPIValue errorConstructor = NULL;
    CHECK_NAPI(napi_get_global(env, &global));
    CHECK_NAPI(napi_get_named_property(env, global, "TypeError", &errorConstructor));
    CHECK_NAPI(napi_new_instance(env, errorConstructor, 1, &msg, result));
    CHECK_NAPI(setErrorCode(env, *result, code, NULL));

    return clearLastError(env);
}

NAPIStatus napi_create_range_error(NAPIEnv env, NAPIValue code, NAPIValue msg, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, msg);
    CHECK_ARG(env, result);

    NAPIValue global = NULL;
    NAPIValue errorConstructor = NULL;
    CHECK_NAPI(napi_get_global(env, &global));
    CHECK_NAPI(napi_get_named_property(env, global, "RangeError", &errorConstructor));
    CHECK_NAPI(napi_new_instance(env, errorConstructor, 1, &msg, result));
    CHECK_NAPI(setErrorCode(env, *result, code, NULL));

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
    default:
        RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->context, (JSValueRef)value), NAPIGenericFailure);
        JSObjectRef object = JSValueToObject(env->context, (JSValueRef)value, &env->lastException);
        CHECK_JSC(env);
        if (JSObjectIsFunction(env->context, object))
        {
            *result = NAPIFunction;
        }
        else
        {
            NativeType *nativeType = JSObjectGetPrivate(object);
            if (nativeType && *nativeType == External)
            {
                *result = NAPIExternal;
            }
            else
            {
                *result = NAPIObject;
            }
        }
        break;
    }

    return clearLastError(env);
}

NAPIStatus napi_get_value_double(NAPIEnv env, NAPIValue value, double *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env, JSValueIsNumber(env->context, (JSValueRef)value), NAPINumberExpected);

    *result = JSValueToNumber(env->context, (JSValueRef)value, &env->lastException);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus napi_get_value_int32(NAPIEnv env, NAPIValue value, int32_t *result)
{
    return napi_get_value_double(env, value, (double *)result);
}

NAPIStatus napi_get_value_uint32(NAPIEnv env,
                                 NAPIValue value,
                                 uint32_t *result)
{
    return napi_get_value_double(env, value, (double *)result);
}

NAPIStatus napi_get_value_int64(NAPIEnv env, NAPIValue value, int64_t *result)
{
    return napi_get_value_double(env, value, (double *)result);
}

NAPIStatus napi_get_value_bool(NAPIEnv env, NAPIValue value, bool *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env, JSValueIsBoolean(env->context, (JSValueRef)value), NAPINumberExpected);
    *result = JSValueToBoolean(env->context, (JSValueRef)value);

    return clearLastError(env);
}

NAPIStatus napi_get_value_string_latin1(NAPIEnv env, NAPIValue value, char *buf, size_t bufsize, size_t *result)
{
    return setLastErrorCode(env, NAPIGenericFailure);
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

    RETURN_STATUS_IF_FALSE(env, JSValueIsString(env->context, (JSValueRef)value), NAPIStringExpected);

    // 1. buf == NULL 计算长度，这是 N-API 文档规定
    // 2. buf && bufsize 发生复制
    // 3. buf && bufsize == 0 正常情况实际上也可以计算长度，但是 N-API 规定计算长度必须是 buf == NULL，因此只能返回 0 长度
    if (buf && bufsize == 0)
    {
        if (result)
        {
            *result = 0;
        }
    }
    else
    {
        // !buf || bufsize != 0
        JSStringRef stringRef = JSValueToStringCopy(env->context, (JSValueRef)value, &env->lastException);
        CHECK_JSC(env);

        if (!buf)
        {
            CHECK_ARG(env, result);
            // 本质上是 UNICODE length * 3 + 1 \0
            size_t length = JSStringGetMaximumUTF8CStringSize(stringRef);
            errno = 0;
            char *buffer = malloc(sizeof(char) * length);
            if (errno == ENOMEM)
            {
                // malloc 失败，无法发生复制，则无法计算实际长度
                *result = 0;
                errno = 0;

                return clearLastError(env);
            }
            // 返回值一定带 \0
            *result = JSStringGetUTF8CString(stringRef, buffer, length) - 1;
            // 虽然是 GetUTF8CString，但实际上是复制，所以需要 free
            free(buffer);
        }
        else
        {
            size_t copied = JSStringGetUTF8CString(stringRef, buf, bufsize);
            if (result)
            {
                // JSStringGetUTF8CString returns size with null terminator.
                *result = copied - 1;
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
NAPIStatus napi_get_value_string_utf16(NAPIEnv env, NAPIValue value, uint_least16_t *buf, size_t bufsize, size_t *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);

    RETURN_STATUS_IF_FALSE(env, JSValueIsString(env->context, (JSValueRef)value), NAPIStringExpected);

    if (buf && bufsize == 0)
    {
        if (result)
        {
            *result = 0;
        }
    }
    else
    {
        JSStringRef stringRef = JSValueToStringCopy(env->context, (JSValueRef)value, &env->lastException);
        CHECK_JSC(env);

        if (!buf)
        {
            CHECK_ARG(env, result);
            *result = JSStringGetLength(stringRef);
        }
        else
        {
            size_t length = JSStringGetLength(stringRef);
            const JSChar *chars = JSStringGetCharactersPtr(stringRef);
            // 64 位
            size_t copied = fmin(length, bufsize - 1);
            memcpy(buf, chars, copied * sizeof(uint_least16_t));
            buf[copied] = 0;
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

    return clearLastError(env);
}

NAPIStatus napi_coerce_to_object(NAPIEnv env, NAPIValue value, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    *result = (NAPIValue)JSValueToObject(env->context, (JSValueRef)value, &env->lastException);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus napi_coerce_to_string(NAPIEnv env, NAPIValue value, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    JSStringRef stringRef = JSValueToStringCopy(env->context, (JSValueRef)value, &env->lastException);
    CHECK_JSC(env);
    *result = (NAPIValue)JSValueMakeString(env->context, stringRef);
    JSStringRelease(stringRef);

    return clearLastError(env);
}

NAPIStatus napi_get_prototype(NAPIEnv env, NAPIValue object, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->context, (JSValueRef)object), NAPIObjectExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)object, &env->lastException);
    CHECK_JSC(env);

    *result = (NAPIValue)JSObjectGetPrototype(env->context, objectRef);

    return clearLastError(env);
}

NAPIStatus napi_get_property_names(NAPIEnv env, NAPIValue object, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    // 应当检查参数是否为对象
    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->context, (JSValueRef)object), NAPIObjectExpected);

    NAPIValue global = NULL;
    NAPIValue objectConstructor = NULL;
    NAPIValue keysFunction = NULL;
    CHECK_NAPI(napi_get_global(env, &global));
    CHECK_NAPI(napi_get_named_property(env, global, "Object", &objectConstructor));
    CHECK_NAPI(napi_get_named_property(env, objectConstructor, "keys", &keysFunction));
    CHECK_NAPI(napi_call_function(env, objectConstructor, keysFunction, 1, &object, result));

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

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->context, (JSValueRef)constructor), NAPIObjectExpected);
    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)constructor, &env->lastException);
    CHECK_JSC(env);

    *result = (NAPIValue)JSObjectCallAsConstructor(
        env->context,
        objectRef,
        argc,
        (const JSValueRef *)argv,
        &env->lastException);
    CHECK_JSC(env);

    return clearLastError(env);
}

/*
NAPIStatus NAPIGetPropertyNames(NAPIEnv env, NAPIValue object, NAPIValue *result)
{
    // 正常应该还有 NAPIKeySkipSymbols
    return NAPIGetAllPropertyNames(env, object, NAPIKeyIncludePrototypes, NAPIKeyEnumerable, NAPIKeyNumbersToStrings, result);
}

NAPIStatus NAPISetProperty(NAPIEnv env, NAPIValue object, NAPIValue key, NAPIValue value)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, key);
    CHECK_ARG(env, value);

    CHECK_ARG(env, env->contextRef);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->contextRef, (JSValueRef)object), NAPIObjectExpected);
    JSObjectRef objectRef = JSValueToObject(env->contextRef, (JSValueRef)object, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, !env->lastException, NAPIPendingException);
    JSObjectSetPropertyForKey(env->contextRef, objectRef, (JSValueRef)key, (JSValueRef)value, kJSPropertyAttributeNone, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, !env->lastException, NAPIPendingException);

    return clearLastError(env);
}

NAPIStatus NAPIHasProperty(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);
    CHECK_ARG(env, key);

    CHECK_ARG(env, env->contextRef);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->contextRef, (JSValueRef)object), NAPIObjectExpected);
    JSObjectRef objectRef = JSValueToObject(env->contextRef, (JSValueRef)object, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, !env->lastException, NAPIPendingException);
    *result = JSObjectHasPropertyForKey(env->contextRef, objectRef, (JSValueRef)key, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, !env->lastException, NAPIPendingException);

    return clearLastError(env);
}

NAPIStatus NAPIGetProperty(NAPIEnv env, NAPIValue object, NAPIValue key, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);
    CHECK_ARG(env, key);

    CHECK_ARG(env, env->contextRef);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->contextRef, (JSValueRef)object), NAPIObjectExpected);
    JSObjectRef objectRef = JSValueToObject(env->contextRef, (JSValueRef)object, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, !env->lastException, NAPIPendingException);
    *result = (NAPIValue)JSObjectGetPropertyForKey(env->contextRef, objectRef, (JSValueRef)key, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, !env->lastException, NAPIPendingException);

    return clearLastError(env);
}

NAPIStatus NAPIDeleteProperty(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);
    CHECK_ARG(env, key);

    CHECK_ARG(env, env->contextRef);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->contextRef, (JSValueRef)object), NAPIObjectExpected);
    JSObjectRef objectRef = JSValueToObject(env->contextRef, (JSValueRef)object, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, !env->lastException, NAPIPendingException);
    *result = JSObjectDeletePropertyForKey(env->contextRef, objectRef, (JSValueRef)key, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, !env->lastException, NAPIPendingException);

    return clearLastError(env);
}

NAPIStatus NAPISetNamedProperty(NAPIEnv env, NAPIValue object, const char *utf8name, NAPIValue value)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);

    CHECK_ARG(env, env->contextRef);

    RETURN_STATUS_IF_FALSE(env, utf8name, NAPIStringExpected);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->contextRef, (JSValueRef)object), NAPIObjectExpected);
    JSObjectRef objectRef = JSValueToObject(env->contextRef, (JSValueRef)object, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, !env->lastException, NAPIPendingException);
    JSStringRef stringRef = JSStringCreateWithUTF8CString(utf8name);
    JSObjectSetProperty(env->contextRef, objectRef, stringRef, (JSValueRef)value, kJSPropertyAttributeNone, &env->lastException);
    JSStringRelease(stringRef);
    RETURN_STATUS_IF_FALSE(env, !env->lastException, NAPIPendingException);

    return clearLastError(env);
}

NAPIStatus NAPIHasNamedProperty(NAPIEnv env, NAPIValue object, const char *utf8name, bool *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->contextRef);

    RETURN_STATUS_IF_FALSE(env, utf8name, NAPIStringExpected);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->contextRef, (JSValueRef)object), NAPIObjectExpected);
    JSObjectRef objectRef = JSValueToObject(env->contextRef, (JSValueRef)object, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, !env->lastException, NAPIPendingException);
    JSStringRef stringRef = JSStringCreateWithUTF8CString(utf8name);
    *result = JSObjectHasProperty(env->contextRef, objectRef, stringRef);
    JSStringRelease(stringRef);

    return clearLastError(env);
}

NAPIStatus NAPIGetNamedProperty(NAPIEnv env, NAPIValue object, const char *utf8name, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->contextRef);

    RETURN_STATUS_IF_FALSE(env, utf8name, NAPIStringExpected);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->contextRef, (JSValueRef)object), NAPIObjectExpected);
    JSObjectRef objectRef = JSValueToObject(env->contextRef, (JSValueRef)object, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, !env->lastException, NAPIPendingException);
    JSStringRef stringRef = JSStringCreateWithUTF8CString(utf8name);
    *result = (NAPIValue)JSObjectGetProperty(env->contextRef, objectRef, stringRef, &env->lastException);
    JSStringRelease(stringRef);
    RETURN_STATUS_IF_FALSE(env, !env->lastException, NAPIPendingException);

    return clearLastError(env);
}

NAPIStatus NAPISetElement(NAPIEnv env, NAPIValue object, uint32_t index, NAPIValue value)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);

    CHECK_ARG(env, env->contextRef);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->contextRef, (JSValueRef)object), NAPIObjectExpected);
    JSObjectRef objectRef = JSValueToObject(env->contextRef, (JSValueRef)object, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, !env->lastException, NAPIPendingException);
    JSObjectSetPropertyAtIndex(env->contextRef, objectRef, index, (JSValueRef)value, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, !env->lastException, NAPIPendingException);

    return clearLastError(env);
}

NAPIStatus NAPIGetElement(NAPIEnv env, NAPIValue object, uint32_t index, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->contextRef);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->contextRef, (JSValueRef)object), NAPIObjectExpected);
    JSObjectRef objectRef = JSValueToObject(env->contextRef, (JSValueRef)object, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, !env->lastException, NAPIPendingException);
    *result = (NAPIValue)JSObjectGetPropertyAtIndex(env->contextRef, objectRef, index, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, !env->lastException, NAPIPendingException);

    return clearLastError(env);
}

NAPIStatus NAPIIsArray(NAPIEnv env, NAPIValue value, bool *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->contextRef);

    JSObjectRef globalObject = JSContextGetGlobalObject(env->contextRef);
    JSStringRef arrayStringRef = JSStringCreateWithUTF8CString("Array");
    JSValueRef arrayConstructorValue = JSObjectGetProperty(env->contextRef, globalObject, arrayStringRef, &env->lastException);
    JSStringRelease(arrayStringRef);
    RETURN_STATUS_IF_FALSE(env, !env->lastException, NAPIPendingException);
    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->contextRef, arrayConstructorValue), NAPIGenericFailure);
    JSObjectRef arrayConstructorObjectRef = JSValueToObject(env->contextRef, arrayConstructorValue, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, !env->lastException, NAPIPendingException);
    JSStringRef isArrayString = JSStringCreateWithUTF8CString("isArray");
    JSValueRef isArrayValue = JSObjectGetProperty(env->contextRef, arrayConstructorObjectRef, isArrayString, &env->lastException);
    JSStringRelease(isArrayString);
    RETURN_STATUS_IF_FALSE(env, !env->lastException, NAPIPendingException);
    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->contextRef, isArrayValue), NAPIGenericFailure);
    JSObjectRef isArray = JSValueToObject(env->contextRef, isArrayValue, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, !env->lastException, NAPIPendingException);
    JSValueRef result = JSObjectCallAsFunction(env->contextRef, isArray, NULL, 1, &value, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, !env->lastException, NAPIPendingException);
    RETURN_STATUS_IF_FALSE(env, JSValueIsBoolean(env->contextRef, result), NAPIGenericFailure);

    *result = JSValueToBoolean(env->contextRef, result);

    return NAPIOk;
}

NAPIStatus NAPIGetArrayLength(NAPIEnv env, NAPIValue value, uint32_t *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->contextRef);

    bool isArray = false;
    NAPIStatus apiStatus = NAPIIsArray(env, value, &isArray);
    RETURN_STATUS_IF_FALSE(env, apiStatus == NAPIOk, NAPIGenericFailure);
    RETURN_STATUS_IF_FALSE(env, isArray, NAPIArrayExpected);
    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->contextRef, (JSValueRef)value), NAPIObjectExpected);
    JSObjectRef objectRef = JSValueToObject(env->contextRef, (JSValueRef)value, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, !env->lastException, NAPIPendingException);
    JSStringRef lengthPropertyName = JSStringCreateWithUTF8CString("length");
    JSValueRef lengthValueRef = JSObjectGetProperty(env->contextRef, objectRef, lengthPropertyName, &env->lastException);
    JSStringRelease(lengthPropertyName);
    RETURN_STATUS_IF_FALSE(env, !env->lastException, NAPIPendingException);
    RETURN_STATUS_IF_FALSE(env, JSValueIsNumber(env->contextRef, lengthValueRef), NAPIGenericFailure);
    *result = JSValueToNumber(env->contextRef, lengthValueRef, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, !env->lastException, NAPIPendingException);

    return NAPIOk;
}

// 传入 NULL 会被转换为 JS null
NAPIStatus NAPIStrictEquals(NAPIEnv env, NAPIValue lhs, NAPIValue rhs, bool *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->contextRef);

    *result = JSValueIsStrictEqual(env->contextRef, lhs, rhs);

    return NAPIOk;
}

NAPIStatus NAPICallFunction(NAPIEnv env, NAPIValue recv, NAPIValue func, size_t argc, const NAPIValue *argv, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, func);

    CHECK_ARG(env, env->contextRef);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->contextRef, func), NAPIObjectExpected);
    JSObjectRef objectRef = JSValueToObject(env->contextRef, func, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, !env->lastException, NAPIPendingException);
    RETURN_STATUS_IF_FALSE(env, JSObjectIsFunction(env->contextRef, objectRef), NAPIFunctionExpected);

    JSObjectRef thisObject = NULL;
    if (recv && JSValueIsObject(env->contextRef, recv))
    {
        thisObject = JSValueToObject(env->contextRef, recv, &env->lastException);
    }

    if (argc > 0)
    {
        CHECK_ARG(env, argv);
    }
    *result = JSObjectCallAsFunction(env->contextRef, objectRef, thisObject, argc, argv, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, !env->lastException, NAPIPendingException);

    return NAPIOk;
}

NAPIStatus NAPIInstanceOf(NAPIEnv env, NAPIValue object, NAPIValue constructor, bool *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, constructor);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->contextRef);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->contextRef, constructor), NAPIObjectExpected);
    JSObjectRef objectRef = JSValueToObject(env->contextRef, constructor, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, env->lastException, NAPIPendingException);
    *result = JSValueIsInstanceOfConstructor(env->contextRef, object, objectRef, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, env->lastException, NAPIPendingException);

    return NAPIOk;
}

NAPIStatus NAPIGetCBInfo(
    NAPIEnv env,             // [in] NAPI environment handle
    NAPICallbackInfo cbinfo, // [in] Opaque callback-info handle
    size_t *argc,            // [in-out] Specifies the size of the provided argv array and receives the actual count of args.
    NAPIValue *argv,         // [out] Array of values
    NAPIValue *thisArg,      // [out] Receives the JS 'this' arg for the call
    void **data)
{
    // TODO(ChasonTang): 处理
    return NAPIOk;
}

NAPIStatus NAPICreateExternal(NAPIEnv env, void *data, NAPIFinalize finalizeCB, void *finalizeHint, NAPIValue *result)
{
    // TODO(ChasonTang): 处理
    return NAPIOk;
}

NAPIStatus NAPIGetValueExternal(NAPIEnv env, NAPIValue value, void **result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->contextRef);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->contextRef, value), NAPIObjectExpected);
    JSObjectRef objectRef = JSValueToObject(env->contextRef, value, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->contextRef, value), NAPIObjectExpected);
    *result = JSObjectGetPrivate(objectRef);

    return NAPIOk;
}

NAPIStatus NAPICreateReference(NAPIEnv env, NAPIValue value, uint32_t initialRefCount, NAPIRef *result);

NAPIStatus NAPIDeleteReference(NAPIEnv env, NAPIRef ref);

static JSContextGroupRef contextGroupRef = NULL;

static UInt8 contextCount = 0;

NAPIStatus NAPICreateJSEngine(NAPIEnv env)
{
    CHECK_ENV(env);

    errno = 0;
    env = malloc(sizeof(struct OpaqueNAPIEnv));
    if (errno == ENOMEM)
    {
        return NAPIGenericFailure;
    }
    if (!contextGroupRef)
    {
        assert(contextCount == 0);
        contextGroupRef = JSContextGroupCreate();
    }
    env->contextRef = JSGlobalContextCreateInGroup(contextGroupRef, NULL);
    contextCount += 1;

    return NAPIOk;
}

NAPIStatus NAPIFreeJSEngine(NAPIEnv env)
{
    CHECK_ENV(env);

    CHECK_ARG(env, env->contextRef);

    JSGlobalContextRelease(env->contextRef);
    free(env);
    if (contextCount > 1)
    {
        contextCount -= 1;
    }
    else if (contextGroupRef)
    {
        JSContextGroupRelease(contextGroupRef);
        contextCount = 0;
        contextGroupRef = NULL;
    }

    return NAPIOk;
}
*/