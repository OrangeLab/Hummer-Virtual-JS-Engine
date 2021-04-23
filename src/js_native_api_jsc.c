//
//  js_native_api_jsc.c
//  NAPI
//
//  Created by 唐佳诚 on 2021/2/23.
//

// 如果一个函数返回的 NAPIStatus 为 NAPIOK，则没有异常被挂起，并且不需要做任何处理
// 如果 NAPIStatus 是除了 NAPIOK 或者 NAPIPendingException，为了尝试恢复并继续执行，而不是直接返回，必须调用 napi_is_exception_pending 来确定是否存在挂起的异常

// 大多数情况下，调用 N-API 函数的时候，如果已经有异常被挂起，函数会立即返回 NAPIPendingException，但是，N-API 允许一些函数被调用用来在返回 JavaScript 环境前做最小化的清理
// 在这种情况下，NAPIStatus 会反映函数的执行结果状态，而不是当前存在异常，为了避免混淆，在每次函数执行后请检查结果

// 当异常被挂起的时候，有两种方法
// 1. 进行适当的清理并返回，那么执行环境会返回到 JavaScript，作为返回到 JavaScript 的过渡的一部分，异常将在 JavaScript 调用原生方法的语句处抛出，大多数 N-API 函数在出现异常被挂起的时候，都只是简单的返回 NAPIPendingException，，所以在这种情况下，建议尽量少做事情，等待返回到 JavaScript 中处理异常
// 2. 尝试处理异常，在原生代码能捕获到异常的地方采取适当的操作，然后继续执行。只有当异常能被安全的处理的少数特定情况下推荐这样做，在这些情况下，napi_get_and_clear_last_exception 可以被用来获取并清除异常，在此之后，如果发现异常还是无法处理，可以通过 napi_throw 重新抛出错误

#include <napi/js_native_api.h>

#include <sys/queue.h>

#define RETURN_STATUS_IF_FALSE(env, condition, status) \
    do                                                 \
    {                                                  \
        if (!(condition))                              \
        {                                              \
            return setLastErrorCode(env, status);  \
        }                                              \
    } while (0)

#define CHECK_ARG(env, arg) RETURN_STATUS_IF_FALSE(env, arg, NAPIInvalidArg)

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

#define CHECK_ENV(env)             \
    do                             \
    {                              \
        if (!(env))         \
        {                          \
            return NAPIInvalidArg; \
        }                          \
    } while (0)

#define NAPI_PREAMBLE(env)                                                                     \
    do                                                                                         \
    {                                                                                          \
        CHECK_ENV(env);                                                                      \
        RETURN_STATUS_IF_FALSE(env, !(env)->lastException, NAPIPendingException); \
        clearLastError(env);                                                                 \
    } while (0)

#define CHECK_JSC(env)                                                             \
    do                                                                             \
    {                                                                              \
        RETURN_STATUS_IF_FALSE(env, !((env)->lastException), NAPIPendingException); \
    } while (0)

#include <napi/js_native_api_types.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <JavaScriptCore/JavaScriptCore.h>

#define HASH_NONFATAL_OOM 1

static bool hashError = false;

#define uthash_nonfatal_oom(elt) hashError = true;

#include <uthash.h>

struct OpaqueNAPIRef {
    LIST_ENTRY(OpaqueNAPIRef) node;
    JSValueRef value;
    uint32_t count;
};

typedef struct {
    JSValueRef value;
    UT_hash_handle hh;
} ActiveReferenceValue;

struct OpaqueNAPIEnv {
    JSGlobalContextRef context;
    // undefined 和 null 实际上也可以当做 exception 抛出，所以异常检查只需要检查是否为 C NULL
    JSValueRef lastException;
    NAPIExtendedErrorInfo lastError;
    LIST_HEAD(, OpaqueNAPIRef) strongReferenceHead;
    ActiveReferenceValue *activeReferenceValues;
};

static inline NAPIStatus clearLastError(NAPIEnv env) {
    // env 空指针检查
    CHECK_ENV(env);
    env->lastError.errorCode = NAPIOK;

    // TODO(boingoing): Should this be a callback?
    env->lastError.engineErrorCode = 0;
    env->lastError.engineReserved = NULL;

    return NAPIOK;
}

static inline NAPIStatus setLastErrorCode(NAPIEnv env, NAPIStatus errorCode) {
    CHECK_ENV(env);
    env->lastError.errorCode = errorCode;

    return errorCode;
}

static inline NAPIStatus setErrorCode(NAPIEnv env, NAPIValue error, NAPIValue code) {
    CHECK_ENV(env);

    // JSValueIsString 传入 NULL context，返回 false，传入 NULL value，转化为 JS null
    RETURN_STATUS_IF_FALSE(env, JSValueIsString(env->context, (JSValueRef) code), NAPIStringExpected);
    CHECK_NAPI(napi_set_named_property(env, error, "code", code));

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
        "Memory allocation error"
};

// The value of the constant below must be updated to reference the last
// message in the `napi_status` enum each time a new error message is added.
// We don't have a napi_status_last as this would result in an ABI
// change each time a message was added.
#define LAST_STATUS NAPIMemoryError

NAPIStatus napi_get_last_error_info(NAPIEnv env, const NAPIExtendedErrorInfo **result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    static_assert(sizeof(errorMessages) / sizeof(*errorMessages) == LAST_STATUS + 1,
                  "Count of error messages must match count of error values");

    if (env->lastError.errorCode <= LAST_STATUS) {
        // Wait until someone requests the last error information to fetch the error
        // message string
        env->lastError.errorMessage = errorMessages[env->lastError.errorCode];
    } else {
        // 顶多缺失 errorMessage，尽量不要 Abort
        assert(false);
    }

    *result = &(env->lastError);

    // 这里不能使用 clearLastError(env);
    return NAPIOK;
}

NAPIStatus napi_get_undefined(NAPIEnv env, NAPIValue *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue) JSValueMakeUndefined(env->context);

    return clearLastError(env);
}

NAPIStatus napi_get_null(NAPIEnv env, NAPIValue *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue) JSValueMakeNull(env->context);

    return clearLastError(env);
}

NAPIStatus napi_get_global(NAPIEnv env, NAPIValue *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue) JSContextGetGlobalObject(env->context);

    return clearLastError(env);
}

NAPIStatus napi_get_boolean(NAPIEnv env, bool value, NAPIValue *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue) JSValueMakeBoolean(env->context, value);

    return clearLastError(env);
}

NAPIStatus napi_create_object(NAPIEnv env, NAPIValue *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue) JSObjectMake(env->context, NULL, NULL);

    return clearLastError(env);
}

NAPIStatus napi_create_array(NAPIEnv env, NAPIValue *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue) JSObjectMakeArray(env->context, 0, NULL, &env->lastException);
    CHECK_JSC(env);

    return clearLastError(env);
}

// 本质为 const array = new Array(); array.length = length;
NAPIStatus napi_create_array_with_length(NAPIEnv env, size_t length, NAPIValue *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue) JSObjectMakeArray(env->context, 0, NULL, &env->lastException);
    CHECK_JSC(env);

    JSStringRef lengthStringRef = JSStringCreateWithUTF8CString("length");
    JSObjectSetProperty(env->context, (JSObjectRef) *result, lengthStringRef,
                        JSValueMakeNumber(env->context, (double) length),
                        kJSPropertyAttributeNone, &env->lastException);
    JSStringRelease(lengthStringRef);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus napi_create_double(NAPIEnv env, double value, NAPIValue *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue) JSValueMakeNumber(env->context, value);

    return clearLastError(env);
}

NAPIStatus napi_create_int32(NAPIEnv env, int32_t value, NAPIValue *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue) JSValueMakeNumber(env->context, value);

    return clearLastError(env);
}

NAPIStatus napi_create_uint32(NAPIEnv env, uint32_t value, NAPIValue *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue) JSValueMakeNumber(env->context, value);

    return clearLastError(env);
}

NAPIStatus napi_create_int64(NAPIEnv env, int64_t value, NAPIValue *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIValue) JSValueMakeNumber(env->context, (double) value);

    return clearLastError(env);
}

// 未实现
//NAPIStatus napi_create_string_latin1(NAPIEnv env, const char *str, size_t length, NAPIValue *result) {
//    CHECK_ENV(env);
//
//    return setLastErrorCode(env, NAPINotImplemented);
//}

// JavaScriptCore 只能接受 \0 结尾的字符串
// 传入 str NULL 则为 ""
NAPIStatus napi_create_string_utf8(NAPIEnv env, const char *str, size_t length, NAPIValue *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env, length == NAPI_AUTO_LENGTH, NAPIInvalidArg);

    JSStringRef stringRef = JSStringCreateWithUTF8CString(str);
    *result = (NAPIValue) JSValueMakeString(env->context, stringRef);
    JSStringRelease(stringRef);

    return clearLastError(env);
}

NAPIStatus napi_create_string_utf16(NAPIEnv env, const char16_t *str, size_t length, NAPIValue *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    static_assert(sizeof(char16_t) == sizeof(JSChar), "char16_t size not equal JSChar");

    RETURN_STATUS_IF_FALSE(env, length != NAPI_AUTO_LENGTH, NAPIInvalidArg);

    JSStringRef stringRef = JSStringCreateWithCharacters(str, length);
    *result = (NAPIValue) JSValueMakeString(env->context, stringRef);
    JSStringRelease(stringRef);

    return clearLastError(env);
}

NAPIStatus napi_create_symbol(NAPIEnv env, NAPIValue description, NAPIValue *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    NAPIValue global, symbolFunc;
    CHECK_NAPI(napi_get_global(env, &global));
    // iOS 9
    CHECK_NAPI(napi_get_named_property(env, global, "Symbol", &symbolFunc));
    CHECK_NAPI(napi_call_function(env, global, symbolFunc, 1, &description, result));

    return clearLastError(env);
}

// Function

struct OpaqueNAPICallbackInfo {
    JSObjectRef newTarget;
    JSObjectRef thisArg;
    size_t argc;
    const JSValueRef *argv;
    void *data;
};

typedef struct {
    NAPIEnv env;
    NAPICallback callback;
    void *data;
} FunctionInfo;

// 所有参数都会存在，返回值可以为 NULL
static JSValueRef callAsFunction(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount,
                                 const JSValueRef arguments[], JSValueRef *exception) {
    JSValueRef prototype = JSObjectGetPrototype(ctx, function);
    if (!JSValueIsObject(ctx, prototype)) {
        // 正常不应当出现
        assert(false);

        return NULL;
    }
    JSObjectRef prototypeObjectRef = JSValueToObject(ctx, prototype, exception);
    if (*exception) {
        // 正常不应当出现
        assert(false);

        return NULL;
    }
    FunctionInfo *functionInfo = JSObjectGetPrivate(prototypeObjectRef);
    if (!functionInfo) {
        // 正常不应当出现
        assert(false);

        return NULL;
    }

    // Make sure any errors encountered last time we were in N-API are gone.
    clearLastError(functionInfo->env);

    struct OpaqueNAPICallbackInfo callbackInfo;
    callbackInfo.newTarget = NULL;
    callbackInfo.thisArg = thisObject;
    callbackInfo.argc = argumentCount;
    callbackInfo.argv = arguments;
    callbackInfo.data = functionInfo->data;

    return (JSValueRef) functionInfo->callback(functionInfo->env, &callbackInfo);
}

static void functionFinalize(JSObjectRef object) {
    free(JSObjectGetPrivate(object));
}

NAPIStatus
napi_create_function(NAPIEnv env, const char *utf8name, size_t length, NAPICallback cb, void *data, NAPIValue *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);
    CHECK_ARG(env, cb);

    RETURN_STATUS_IF_FALSE(env, length == NAPI_AUTO_LENGTH, NAPIInvalidArg);

    errno = 0;
    // malloc(0) 依然会分配 16 字节
    FunctionInfo *functionInfo = malloc(sizeof(FunctionInfo));
    if (errno == ENOMEM) {
        errno = 0;

        return setLastErrorCode(env, NAPIMemoryError);
    }
    functionInfo->env = env;
    functionInfo->callback = cb;
    functionInfo->data = data;

    JSClassDefinition classDefinition = kJSClassDefinitionEmpty;
    // 使用 Object.prototype 作为实例的 [[prototype]]
    classDefinition.attributes = kJSClassAttributeNoAutomaticPrototype;
    classDefinition.finalize = functionFinalize;
    JSClassRef classRef = JSClassCreate(&classDefinition);
    JSObjectRef prototype = JSObjectMake(env->context, classRef, functionInfo);
    JSClassRelease(classRef);

    // 函数名如果为 NULL 则为匿名函数
    JSStringRef stringRef = utf8name ? JSStringCreateWithUTF8CString(utf8name) : NULL;
    JSObjectRef functionObjectRef = JSObjectMakeFunctionWithCallback(env->context, stringRef, callAsFunction);
    *result = (NAPIValue) functionObjectRef;
    if (stringRef) {
        JSStringRelease(stringRef);
    }

    JSObjectSetPrototype(env->context, prototype, JSObjectGetPrototype(env->context, functionObjectRef));
    JSObjectSetPrototype(env->context, functionObjectRef, prototype);

    return clearLastError(env);
}

NAPIStatus napi_create_error(NAPIEnv env, NAPIValue code, NAPIValue msg, NAPIValue *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, msg);
    CHECK_ARG(env, result);

    *result = (NAPIValue) JSObjectMakeError(env->context, 1, (JSValueRef *) &msg, &env->lastException);
    CHECK_JSC(env);

    CHECK_NAPI(setErrorCode(env, *result, code));

    return clearLastError(env);
}

NAPIStatus napi_create_type_error(NAPIEnv env, NAPIValue code, NAPIValue msg, NAPIValue *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, msg);
    CHECK_ARG(env, result);

    NAPIValue global = NULL;
    NAPIValue errorConstructor = NULL;
    CHECK_NAPI(napi_get_global(env, &global));
    CHECK_NAPI(napi_get_named_property(env, global, "TypeError", &errorConstructor));
    CHECK_NAPI(napi_new_instance(env, errorConstructor, 1, &msg, result));
    CHECK_NAPI(setErrorCode(env, *result, code));

    return clearLastError(env);
}

NAPIStatus napi_create_range_error(NAPIEnv env, NAPIValue code, NAPIValue msg, NAPIValue *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, msg);
    CHECK_ARG(env, result);

    NAPIValue global = NULL;
    NAPIValue errorConstructor = NULL;
    CHECK_NAPI(napi_get_global(env, &global));
    CHECK_NAPI(napi_get_named_property(env, global, "RangeError", &errorConstructor));
    CHECK_NAPI(napi_new_instance(env, errorConstructor, 1, &msg, result));
    CHECK_NAPI(setErrorCode(env, *result, code));

    return clearLastError(env);
}

NAPIStatus napi_typeof(NAPIEnv env, NAPIValue value, NAPIValueType *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    // JSC does not support BigInt
    JSType valueType = JSValueGetType(env->context, (JSValueRef) value);
    switch (valueType) {
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
            RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->context, (JSValueRef) value), NAPIGenericFailure);
            JSObjectRef object = JSValueToObject(env->context, (JSValueRef) value, &env->lastException);
            CHECK_JSC(env);
            if (JSObjectIsFunction(env->context, object)) {
                *result = NAPIFunction;
            } else {
                if (JSObjectGetPrivate(object)) {
                    *result = NAPIExternal;
                } else {
                    *result = NAPIObject;
                }
            }
            break;
    }

    return clearLastError(env);
}

NAPIStatus napi_get_value_double(NAPIEnv env, NAPIValue value, double *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env, JSValueIsNumber(env->context, (JSValueRef) value), NAPINumberExpected);

    *result = JSValueToNumber(env->context, (JSValueRef) value, &env->lastException);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus napi_get_value_int32(NAPIEnv env, NAPIValue value, int32_t *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env, JSValueIsNumber(env->context, (JSValueRef) value), NAPINumberExpected);

    *result = (int32_t) JSValueToNumber(env->context, (JSValueRef) value, &env->lastException);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus napi_get_value_uint32(NAPIEnv env,
                                 NAPIValue value,
                                 uint32_t *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env, JSValueIsNumber(env->context, (JSValueRef) value), NAPINumberExpected);

    *result = (uint32_t) JSValueToNumber(env->context, (JSValueRef) value, &env->lastException);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus napi_get_value_int64(NAPIEnv env, NAPIValue value, int64_t *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env, JSValueIsNumber(env->context, (JSValueRef) value), NAPINumberExpected);

    *result = (int64_t) JSValueToNumber(env->context, (JSValueRef) value, &env->lastException);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus napi_get_value_bool(NAPIEnv env, NAPIValue value, bool *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env, JSValueIsBoolean(env->context, (JSValueRef) value), NAPINumberExpected);
    *result = JSValueToBoolean(env->context, (JSValueRef) value);

    return clearLastError(env);
}

//NAPIStatus napi_get_value_string_latin1(NAPIEnv env, NAPIValue value,
//                                        char *buf, size_t bufsize,
//                                        size_t *result) {
//    return setLastErrorCode(env, NAPINotImplemented);
//}

// 光计算长度实际上存在性能损耗
// Copies a JavaScript string into a UTF-8 string buffer. The result is the
// number of bytes (excluding the null terminator) copied into buf.
// A sufficient buffer size should be greater than the length of string,
// reserving space for null terminator.
// If bufsize is insufficient, the string will be truncated and null terminated.
// If buf is NULL, this method returns the length of the string (in bytes)
// via the result parameter.
// The result argument is optional unless buf is NULL.
NAPIStatus napi_get_value_string_utf8(NAPIEnv env, NAPIValue value, char *buf, size_t bufsize, size_t *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);

    RETURN_STATUS_IF_FALSE(env, JSValueIsString(env->context, (JSValueRef) value), NAPIStringExpected);

    // 1. buf == NULL 计算长度，这是 N-API 文档规定
    // 2. buf && bufsize 发生复制
    // 3. buf && bufsize == 0 正常情况实际上也可以计算长度，但是 N-API 规定计算长度必须是 buf == NULL，因此只能返回 0 长度
    if (buf && bufsize == 0) {
        if (result) {
            *result = 0;
        }
    } else {
        // !buf || bufsize != 0
        if (!buf) {
            CHECK_ARG(env, result);

            JSStringRef stringRef = JSValueToStringCopy(env->context, (JSValueRef) value, &env->lastException);
            CHECK_JSC(env);

            // 本质上是 UNICODE length * 3 + 1 \0
            size_t length = JSStringGetMaximumUTF8CStringSize(stringRef);
            if (!length) {
                assert(false);

                JSStringRelease(stringRef);

                return setLastErrorCode(env, NAPIGenericFailure);
            }
            errno = 0;
            char *buffer = malloc(sizeof(char) * length);
            if (errno == ENOMEM) {
                // malloc 失败，无法发生复制，则无法计算实际长度
                *result = 0;
                errno = 0;
                JSStringRelease(stringRef);

                return clearLastError(env);
            }
            // 返回值一定带 \0
            *result = JSStringGetUTF8CString(stringRef, buffer, length) - 1;
            // 虽然是 GetUTF8CString，但实际上是复制，所以需要 free
            free(buffer);
            JSStringRelease(stringRef);
        } else {
            JSStringRef stringRef = JSValueToStringCopy(env->context, (JSValueRef) value, &env->lastException);
            CHECK_JSC(env);
            size_t copied = JSStringGetUTF8CString(stringRef, buf, bufsize);
            if (result) {
                // JSStringGetUTF8CString returns size with null terminator.
                *result = copied - 1;
            }
            JSStringRelease(stringRef);
        }
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
NAPIStatus
napi_get_value_string_utf16(NAPIEnv env, NAPIValue value, char16_t *buf, size_t bufsize, size_t *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);

    RETURN_STATUS_IF_FALSE(env, JSValueIsString(env->context, (JSValueRef) value), NAPIStringExpected);

    if (buf && bufsize == 0) {
        if (result) {
            *result = 0;
        }
    } else {
        if (!buf) {
            CHECK_ARG(env, result);

            JSStringRef stringRef = JSValueToStringCopy(env->context, (JSValueRef) value, &env->lastException);
            CHECK_JSC(env);

            *result = JSStringGetLength(stringRef);
            JSStringRelease(stringRef);
        } else {
            static_assert(sizeof(char16_t) == sizeof(JSChar), "char16_t size not equal JSChar");

            JSStringRef stringRef = JSValueToStringCopy(env->context, (JSValueRef) value, &env->lastException);
            CHECK_JSC(env);

            size_t length = JSStringGetLength(stringRef);
            const JSChar *chars = JSStringGetCharactersPtr(stringRef);
            // 64 位
            size_t copied = length > bufsize - 1 ? bufsize - 1 : length;
            memcpy(buf, chars, copied * sizeof(char16_t));
            buf[copied] = 0;
            if (result) {
                *result = copied;
            }
            JSStringRelease(stringRef);
        }
    }

    return clearLastError(env);
}

NAPIStatus napi_coerce_to_bool(NAPIEnv env, NAPIValue value, NAPIValue *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    *result = (NAPIValue) JSValueMakeBoolean(env->context, JSValueToBoolean(env->context, (JSValueRef) value));

    return clearLastError(env);
}

NAPIStatus napi_coerce_to_number(NAPIEnv env, NAPIValue value, NAPIValue *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    double doubleValue = JSValueToNumber(env->context, (JSValueRef) value, &env->lastException);
    CHECK_JSC(env);

    *result = (NAPIValue) JSValueMakeNumber(env->context, doubleValue);

    return clearLastError(env);
}

NAPIStatus napi_coerce_to_object(NAPIEnv env, NAPIValue value, NAPIValue *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    *result = (NAPIValue) JSValueToObject(env->context, (JSValueRef) value, &env->lastException);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus napi_coerce_to_string(NAPIEnv env, NAPIValue value, NAPIValue *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    JSStringRef stringRef = JSValueToStringCopy(env->context, (JSValueRef) value, &env->lastException);
    CHECK_JSC(env);
    *result = (NAPIValue) JSValueMakeString(env->context, stringRef);
    JSStringRelease(stringRef);

    return clearLastError(env);
}

NAPIStatus napi_get_prototype(NAPIEnv env, NAPIValue object, NAPIValue *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->context, (JSValueRef) object), NAPIObjectExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef) object, &env->lastException);
    CHECK_JSC(env);

    *result = (NAPIValue) JSObjectGetPrototype(env->context, objectRef);

    return clearLastError(env);
}

NAPIStatus napi_get_property_names(NAPIEnv env, NAPIValue object, NAPIValue *result) {
    // N-API V8 实现默认
    // 1. 包含原型链
    // 2. 可枚举，不包括 Symbol
    // 3. number 转换为 string
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    // 应当检查参数是否为对象
    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->context, (JSValueRef) object), NAPIObjectExpected);

    // 使用 JSObjectCopyPropertyNames 实现，符合 Object.keys
    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef) object, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, !env->lastException, NAPIPendingException);

    JSPropertyNameArrayRef propertyNameArrayRef = JSObjectCopyPropertyNames(env->context, objectRef);

    size_t propertyCount = JSPropertyNameArrayGetCount(propertyNameArrayRef);

    if (propertyCount > 0) {
        JSObjectRef arrayObjectRef = JSObjectMakeArray(env->context, 0, NULL, &env->lastException);
        if (env->lastException) {
            JSPropertyNameArrayRelease(propertyNameArrayRef);

            return setLastErrorCode(env, NAPIPendingException);
        }

        for (unsigned int i = 0; i < propertyCount; ++i) {
            JSStringRef propertyNameStringRef = JSPropertyNameArrayGetNameAtIndex(propertyNameArrayRef, i);
            JSValueRef propertyValueRef = JSObjectGetProperty(env->context, objectRef, propertyNameStringRef,
                                                              &env->lastException);
            if (env->lastException) {
                JSPropertyNameArrayRelease(propertyNameArrayRef);

                return setLastErrorCode(env, NAPIPendingException);
            }
            JSObjectSetPropertyAtIndex(env->context, arrayObjectRef, i, propertyValueRef, &env->lastException);
            if (env->lastException) {
                JSPropertyNameArrayRelease(propertyNameArrayRef);

                return setLastErrorCode(env, NAPIPendingException);
            }
        }
        *result = (NAPIValue) arrayObjectRef;
    }
    JSPropertyNameArrayRelease(propertyNameArrayRef);

    return clearLastError(env);
}

NAPIStatus napi_set_property(NAPIEnv env, NAPIValue object, NAPIValue key, NAPIValue value) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, key);
    CHECK_ARG(env, value);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->context, (JSValueRef) object), NAPIObjectExpected);
    RETURN_STATUS_IF_FALSE(env, JSValueIsString(env->context, (JSValueRef) key), NAPIStringExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef) object, &env->lastException);
    CHECK_JSC(env);

    JSStringRef keyStringRef = JSValueToStringCopy(env->context, (JSValueRef) key, &env->lastException);
    CHECK_JSC(env);

    JSObjectSetProperty(
            env->context,
            objectRef,
            keyStringRef,
            (JSValueRef) value,
            kJSPropertyAttributeNone,
            &env->lastException);
    JSStringRelease(keyStringRef);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus napi_has_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);
    CHECK_ARG(env, key);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->context, (JSValueRef) object), NAPIObjectExpected);
    RETURN_STATUS_IF_FALSE(env, JSValueIsString(env->context, (JSValueRef) key), NAPIStringExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef) object, &env->lastException);
    CHECK_JSC(env);

    JSStringRef keyStringRef = JSValueToStringCopy(env->context, (JSValueRef) key, &env->lastException);
    CHECK_JSC(env);

    *result = JSObjectHasProperty(
            env->context,
            objectRef,
            keyStringRef);
    JSStringRelease(keyStringRef);

    return clearLastError(env);
}

NAPIStatus napi_get_property(NAPIEnv env, NAPIValue object, NAPIValue key, NAPIValue *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, key);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->context, (JSValueRef) object), NAPIObjectExpected);
    RETURN_STATUS_IF_FALSE(env, JSValueIsString(env->context, (JSValueRef) key), NAPIStringExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef) object, &env->lastException);
    CHECK_JSC(env);

    JSStringRef keyStringRef = JSValueToStringCopy(env->context, (JSValueRef) key, &env->lastException);
    CHECK_JSC(env);

    *result = (NAPIValue) JSObjectGetProperty(
            env->context,
            objectRef,
            keyStringRef,
            &env->lastException);
    JSStringRelease(keyStringRef);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus napi_delete_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, key);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->context, (JSValueRef) object), NAPIObjectExpected);
    RETURN_STATUS_IF_FALSE(env, JSValueIsString(env->context, (JSValueRef) key), NAPIStringExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef) object, &env->lastException);
    CHECK_JSC(env);

    JSStringRef keyStringRef = JSValueToStringCopy(env->context, (JSValueRef) key, &env->lastException);
    CHECK_JSC(env);

    *result = JSObjectDeleteProperty(
            env->context,
            objectRef,
            keyStringRef,
            &env->lastException);
    JSStringRelease(keyStringRef);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus napi_has_own_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, key);
    CHECK_ARG(env, result);

    NAPIValue global, objectCtor, function, value;
    CHECK_NAPI(napi_get_global(env, &global));
    CHECK_NAPI(napi_get_named_property(env, global, "Object", &objectCtor));
    CHECK_NAPI(napi_get_named_property(env, objectCtor, "hasOwnProperty", &function));
    CHECK_NAPI(napi_call_function(env, object, function, 1, &key, &value));
    *result = (JSValueRef) JSValueToBoolean(env->context, (JSValueRef) value);

    return clearLastError(env);
}

NAPIStatus napi_set_named_property(NAPIEnv env, NAPIValue object, const char *utf8name, NAPIValue value) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->context, (JSValueRef) object), NAPIObjectExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef) object, &env->lastException);
    CHECK_JSC(env);

    JSStringRef stringRef = JSStringCreateWithUTF8CString(utf8name);

    JSObjectSetProperty(
            env->context,
            objectRef,
            stringRef,
            (JSValueRef) value,
            kJSPropertyAttributeNone,
            &env->lastException);
    JSStringRelease(stringRef);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus napi_has_named_property(NAPIEnv env, NAPIValue object, const char *utf8name, bool *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->context, (JSValueRef) object), NAPIObjectExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef) object, &env->lastException);
    CHECK_JSC(env);

    JSStringRef stringRef = JSStringCreateWithUTF8CString(utf8name);

    *result = JSObjectHasProperty(
            env->context,
            objectRef,
            stringRef);
    JSStringRelease(stringRef);

    return clearLastError(env);
}

NAPIStatus napi_get_named_property(NAPIEnv env, NAPIValue object, const char *utf8name, NAPIValue *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->context, (JSValueRef) object), NAPIObjectExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef) object, &env->lastException);
    CHECK_JSC(env);

    JSStringRef stringRef = JSStringCreateWithUTF8CString(utf8name);

    *result = (NAPIValue) JSObjectGetProperty(
            env->context,
            objectRef,
            stringRef,
            &env->lastException);
    JSStringRelease(stringRef);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus napi_set_element(NAPIEnv env, NAPIValue object, uint32_t index, NAPIValue value) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->context, (JSValueRef) object), NAPIObjectExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef) object, &env->lastException);
    CHECK_JSC(env);

    JSObjectSetPropertyAtIndex(
            env->context,
            objectRef,
            index,
            (JSValueRef) value,
            &env->lastException);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus napi_has_element(NAPIEnv env, NAPIValue object, uint32_t index, bool *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->context, (JSValueRef) object), NAPIObjectExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef) object, &env->lastException);
    CHECK_JSC(env);

    JSValueRef value = JSObjectGetPropertyAtIndex(
            env->context,
            objectRef,
            index,
            &env->lastException);
    CHECK_JSC(env);

    *result = !JSValueIsUndefined(env->context, value);

    return clearLastError(env);
}

NAPIStatus napi_get_element(NAPIEnv env, NAPIValue object, uint32_t index, NAPIValue *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->context, (JSValueRef) object), NAPIObjectExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef) object, &env->lastException);
    CHECK_JSC(env);

    *result = (NAPIValue) JSObjectGetPropertyAtIndex(
            env->context,
            objectRef,
            index,
            &env->lastException);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus napi_delete_element(NAPIEnv env, NAPIValue object, uint32_t index, bool *result) {
    NAPI_PREAMBLE(env);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->context, (JSValueRef) object), NAPIObjectExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef) object, &env->lastException);
    CHECK_JSC(env);

    JSValueRef indexValue = JSValueMakeNumber(env->context, index);
    JSStringRef indexStringRef = JSValueToStringCopy(env->context, indexValue, &env->lastException);
    CHECK_JSC(env);

    *result = JSObjectDeleteProperty(
            env->context,
            objectRef,
            indexStringRef,
            &env->lastException);
    JSStringRelease(indexStringRef);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus
napi_define_properties(NAPIEnv env, NAPIValue object, size_t property_count, const NAPIPropertyDescriptor *properties) {
    NAPI_PREAMBLE(env);
    if (property_count > 0) {
        CHECK_ARG(env, properties);
    }
    for (size_t i = 0; i < property_count; i++) {
        const NAPIPropertyDescriptor *p = properties + i;

        NAPIValue descriptor;
        CHECK_NAPI(napi_create_object(env, &descriptor));

        NAPIValue configurable;
        CHECK_NAPI(napi_get_boolean(env, (p->attributes & NAPIConfigurable), &configurable));
        CHECK_NAPI(napi_set_named_property(env, descriptor, "configurable", configurable));

        NAPIValue enumerable;
        CHECK_NAPI(napi_get_boolean(env, (p->attributes & NAPIEnumerable), &enumerable));
        CHECK_NAPI(napi_set_named_property(env, descriptor, "enumerable", enumerable));

        if (p->getter || p->setter) {
            if (p->getter) {
                NAPIValue getter;
                CHECK_NAPI(napi_create_function(env, p->utf8name, NAPI_AUTO_LENGTH, p->getter, p->data, &getter));
                CHECK_NAPI(napi_set_named_property(env, descriptor, "get", getter));
            }
            if (p->setter) {
                NAPIValue setter;
                CHECK_NAPI(napi_create_function(env, p->utf8name, NAPI_AUTO_LENGTH, p->setter, p->data, &setter));
                CHECK_NAPI(napi_set_named_property(env, descriptor, "set", setter));
            }
        } else if (p->method) {
            NAPIValue method;
            CHECK_NAPI(napi_create_function(env, p->utf8name, NAPI_AUTO_LENGTH, p->method, p->data, &method));
            CHECK_NAPI(napi_set_named_property(env, descriptor, "value", method));
        } else {
            RETURN_STATUS_IF_FALSE(env, p->value, NAPIInvalidArg);

            NAPIValue writable;
            CHECK_NAPI(napi_get_boolean(env, (p->attributes & NAPIWritable), &writable));
            CHECK_NAPI(napi_set_named_property(env, descriptor, "writable", writable));

            CHECK_NAPI(napi_set_named_property(env, descriptor, "value", p->value));
        }

        NAPIValue propertyName;
        if (!p->utf8name) {
            propertyName = p->name;
        } else {
            CHECK_NAPI(napi_create_string_utf8(env, p->utf8name, NAPI_AUTO_LENGTH, &propertyName));
        }

        NAPIValue global, objectCtor, function;
        CHECK_NAPI(napi_get_global(env, &global));
        CHECK_NAPI(napi_get_named_property(env, global, "Object", &objectCtor));
        CHECK_NAPI(napi_get_named_property(env, objectCtor, "defineProperty", &function));

        NAPIValue args[] = {object, propertyName, descriptor};
        CHECK_NAPI(napi_call_function(env, objectCtor, function, 3, args, NULL));
    }

    return clearLastError(env);
}

NAPIStatus napi_is_array(NAPIEnv env, NAPIValue value, bool *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    *result = JSValueIsArray(
            env->context,
            (JSValueRef) value);

    return clearLastError(env);
}

NAPIStatus napi_get_array_length(NAPIEnv env, NAPIValue value, uint32_t *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->context, (JSValueRef) value), NAPIObjectExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef) value, &env->lastException);

    JSStringRef stringRef = JSStringCreateWithUTF8CString("length");

    JSValueRef length = JSObjectGetProperty(
            env->context,
            objectRef,
            stringRef,
            &env->lastException);
    JSStringRelease(stringRef);
    CHECK_JSC(env);

    *result = (uint32_t) JSValueToNumber(env->context, length, &env->lastException);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus napi_strict_equals(NAPIEnv env, NAPIValue lhs, NAPIValue rhs, bool *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, lhs);
    CHECK_ARG(env, rhs);
    CHECK_ARG(env, result);

    *result = JSValueIsStrictEqual(
            env->context,
            (JSValueRef) lhs,
            (JSValueRef) rhs);

    return clearLastError(env);
}

NAPIStatus
napi_call_function(NAPIEnv env, NAPIValue recv, NAPIValue func, size_t argc, const NAPIValue *argv, NAPIValue *napi_define_properties
) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, recv);
    if (argc > 0) {
        CHECK_ARG(env, argv);
    }

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->context, (JSValueRef) func), NAPIObjectExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef) func, &env->lastException);
    JSObjectRef thisObjectRef = NULL;
    if (JSValueIsObject(env->context, (JSValueRef) recv)) {
        thisObjectRef = JSValueToObject(env->context, (JSValueRef) recv, &env->lastException);
        CHECK_JSC(env);
    }

    JSValueRef returnValue = JSObjectCallAsFunction(
            env->context,
            objectRef,
            thisObjectRef,
            argc,
            (JSValueRef const *) argv,
            &env->lastException);
    CHECK_JSC(env);

    if (result) {
        *result = (NAPIValue) returnValue;
    }

    return clearLastError(env);
}

NAPIStatus
napi_new_instance(NAPIEnv env, NAPIValue constructor, size_t argc, const NAPIValue *argv, NAPIValue *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, constructor);
    if (argc > 0) {
        CHECK_ARG(env, argv);
    }
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->context, (JSValueRef) constructor), NAPIObjectExpected);
    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef) constructor, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(env, JSObjectIsConstructor(env->context, objectRef), NAPIFunctionExpected);

    *result = (NAPIValue) JSObjectCallAsConstructor(
            env->context,
            objectRef,
            argc,
            (const JSValueRef *) argv,
            &env->lastException);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus napi_instanceof(NAPIEnv env, NAPIValue object, NAPIValue constructor, bool *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, object);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->context, (JSValueRef) constructor), NAPIObjectExpected);
    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef) constructor, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(env, JSObjectIsConstructor(env->context, objectRef), NAPIFunctionExpected);

    *result = JSValueIsInstanceOfConstructor(
            env->context,
            (JSValueRef) object,
            objectRef,
            &env->lastException);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus
napi_get_cb_info(NAPIEnv env, NAPICallbackInfo cbinfo, size_t *argc, NAPIValue *argv, NAPIValue *thisArg, void **data) {
    CHECK_ENV(env);
    CHECK_ARG(env, cbinfo);

    if (argv) {
        CHECK_ARG(env, argc);

        size_t i = 0;
        size_t min = *argc > cbinfo->argc ? cbinfo->argc : *argc;

        for (; i < min; i++) {
            argv[i] = (NAPIValue) cbinfo->argv[i];
        }

        if (i < *argc) {
            for (; i < *argc; i++) {
                argv[i] = (NAPIValue) JSValueMakeUndefined(env->context);
            }
        }
    }

    if (argc) {
        *argc = cbinfo->argc;
    }

    if (thisArg) {
        *thisArg = (NAPIValue) cbinfo->thisArg;
    }

    if (data) {
        *data = cbinfo->data;
    }

    return clearLastError(env);
}

NAPIStatus napi_get_new_target(NAPIEnv env, NAPICallbackInfo cbinfo, NAPIValue *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, cbinfo);
    CHECK_ARG(env, result);

    *result = (NAPIValue) cbinfo->newTarget;

    return clearLastError(env);
}

// Constructor

typedef struct {
    NAPIEnv env;
    JSClassRef classRef;
    NAPICallback callback;
    void *data;
} ConstructorInfo;

static void constructorFinalize(JSObjectRef object) {
    ConstructorInfo *info = JSObjectGetPrivate(object);
    if (info && info->classRef) {
        JSClassRelease(info->classRef);
    }
    free(info);
}

static JSObjectRef callAsConstructor(JSContextRef ctx,
                                     JSObjectRef constructor,
                                     size_t argumentCount,
                                     const JSValueRef arguments[],
                                     JSValueRef *exception) {
    JSValueRef prototype = JSObjectGetPrototype(ctx, constructor);
    if (!JSValueIsObject(ctx, prototype)) {
        // 正常不应当出现
        assert(false);

        return NULL;
    }
    JSObjectRef prototypeObjectRef = JSValueToObject(ctx, prototype, exception);
    if (*exception) {
        // 正常不应当出现
        assert(false);

        return NULL;
    }
    ConstructorInfo *constructorInfo = JSObjectGetPrivate(prototypeObjectRef);
    if (!constructorInfo) {
        // 正常不应当出现
        assert(false);

        return NULL;
    }

    // Make sure any errors encountered last time we were in N-API are gone.
    clearLastError(constructorInfo->env);

    JSObjectRef instance = JSObjectMake(ctx, constructorInfo->classRef, NULL);

    struct OpaqueNAPICallbackInfo callbackInfo = {
            NULL,
            NULL,
            0,
            NULL,
            NULL
    };
    callbackInfo.newTarget = constructor;
    callbackInfo.thisArg = instance;
    callbackInfo.argc = argumentCount;
    callbackInfo.argv = arguments;
    callbackInfo.data = constructorInfo->data;

    JSValueRef returnValue = (JSValueRef) constructorInfo->callback(constructorInfo->env, &callbackInfo);
    if (JSValueIsObject(ctx, returnValue)) {
        assert(false);

        return NULL;
    }
    JSObjectRef objectRef = JSValueToObject(ctx, returnValue, exception);
    if (*exception) {
        // 正常不应当出现
        assert(false);

        return NULL;
    }

    return objectRef;
}

NAPIStatus napi_define_class(NAPIEnv env, const char *utf8name, size_t length, NAPICallback constructor, void *data,
                             size_t propertyCount, const NAPIPropertyDescriptor *properties, NAPIValue *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);
    CHECK_ARG(env, constructor);

    RETURN_STATUS_IF_FALSE(env, length == NAPI_AUTO_LENGTH, NAPIInvalidArg);

    if (propertyCount > 0) {
        CHECK_ARG(env, properties);
    }
    errno = 0;
    ConstructorInfo *constructorInfo = malloc(sizeof(ConstructorInfo));
    if (errno == ENOMEM) {
        errno = 0;

        return setLastErrorCode(env, NAPIGenericFailure);
    }
    constructorInfo->env = env;
    constructorInfo->callback = constructor;
    constructorInfo->data = data;

    JSClassDefinition classDefinition = kJSClassDefinitionEmpty;
    classDefinition.className = utf8name;
    constructorInfo->classRef = JSClassCreate(&classDefinition);

    JSClassDefinition prototypeClassDefinition = kJSClassDefinitionEmpty;
    prototypeClassDefinition.attributes = kJSClassAttributeNoAutomaticPrototype;
    prototypeClassDefinition.className = utf8name;
    prototypeClassDefinition.finalize = constructorFinalize;
    JSClassRef prototypeClassRef = JSClassCreate(&prototypeClassDefinition);

    JSObjectRef function = JSObjectMakeConstructor(env->context, constructorInfo->classRef, callAsConstructor);
    JSObjectRef prototype = JSObjectMake(env->context, prototypeClassRef, constructorInfo);
    JSClassRelease(prototypeClassRef);
    JSObjectSetPrototype(env->context, prototype, JSObjectGetPrototype(env->context, function));
    JSObjectSetPrototype(env->context, function, prototype);

    JSStringRef stringRef = JSStringCreateWithUTF8CString("constructor");
    JSObjectSetProperty(env->context, prototype, stringRef, function,
                        kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete, &env->lastException);
    JSStringRelease(stringRef);
    if (env->lastException) {
        JSClassRelease(constructorInfo->classRef);
        free(constructorInfo);

        return setLastErrorCode(env, NAPIPendingException);
    }

    int instancePropertyCount = 0;
    int staticPropertyCount = 0;
    for (size_t i = 0; i < propertyCount; i++) {
        if ((properties[i].attributes & NAPIStatic) != 0) {
            staticPropertyCount++;
        } else {
            instancePropertyCount++;
        }
    }

    NAPIPropertyDescriptor *staticDescriptors = NULL;
    if (staticPropertyCount > 0) {
        errno = 0;
        staticDescriptors = malloc(
                sizeof(NAPIPropertyDescriptor) * staticPropertyCount);
        if (errno == ENOMEM) {
            errno = 0;
            JSClassRelease(constructorInfo->classRef);
            free(constructorInfo);

            return NAPIMemoryError;
        }
    }

    NAPIPropertyDescriptor *instanceDescriptors = NULL;
    if (instancePropertyCount > 0) {
        errno = 0;
        instanceDescriptors = malloc(
                sizeof(NAPIPropertyDescriptor) * instancePropertyCount);
        if (errno == ENOMEM) {
            errno = 0;
            JSClassRelease(constructorInfo->classRef);
            free(constructorInfo);
            free(staticDescriptors);

            return NAPIMemoryError;
        }
    }

    size_t instancePropertyIndex = 0;
    size_t staticPropertyIndex = 0;

    for (size_t i = 0; i < propertyCount; i++) {
        if ((properties[i].attributes & NAPIStatic) != 0) {
            staticDescriptors[staticPropertyIndex] = properties[i];
            staticPropertyIndex += 1;
        } else {
            instanceDescriptors[instancePropertyIndex] = properties[i];
            instancePropertyIndex += 1;
        }
    }

    if (staticPropertyCount > 0) {
        NAPIStatus status = napi_define_properties(env, (NAPIValue) function, staticPropertyIndex, staticDescriptors);
        if (status != NAPIOK) {
            JSClassRelease(constructorInfo->classRef);
            free(constructorInfo);
            free(staticDescriptors);
            free(instanceDescriptors);

            return setLastErrorCode(env, status);
        }
    }

    if (instancePropertyCount > 0) {
        NAPIValue prototypeValue;
        NAPIStatus status = napi_get_named_property(env, (NAPIValue) function, "prototype", &prototypeValue);
        if (status != NAPIOK) {
            JSClassRelease(constructorInfo->classRef);
            free(constructorInfo);
            free(staticDescriptors);
            free(instanceDescriptors);

            return setLastErrorCode(env, status);
        }
        status = napi_define_properties(env, prototypeValue, instancePropertyIndex, instanceDescriptors);
        if (status != NAPIOK) {
            JSClassRelease(constructorInfo->classRef);
            free(constructorInfo);
            free(staticDescriptors);
            free(instanceDescriptors);

            return setLastErrorCode(env, status);
        }
    }
    free(staticDescriptors);
    free(instanceDescriptors);

    *result = (NAPIValue) function;

    return clearLastError(env);
}

// External

struct Finalizer {
    SLIST_ENTRY(Finalizer) node;
    NAPIFinalize finalizeCallback;
    void *finalizeHint;
};

typedef struct {
    NAPIEnv env;
    void *data;
    SLIST_HEAD(, Finalizer) finalizerHead;
} ExternalInfo;

static inline NAPIStatus unwrap(NAPIEnv env, NAPIValue object, ExternalInfo **result) {
    NAPIValue prototype = NULL;
    CHECK_NAPI(napi_get_prototype(env, object, &prototype));

    NAPI_PREAMBLE(env);
    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->context, (JSValueRef) prototype), NAPIObjectExpected);
    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef) prototype, &env->lastException);
    CHECK_JSC(env);

    *result = JSObjectGetPrivate(objectRef);

    return clearLastError(env);
}

static void ExternalFinalize(JSObjectRef object) {
    ExternalInfo *info = JSObjectGetPrivate(object);
    // 调用 finalizer
    if (info) {
        struct Finalizer *finalizer, *tempFinalizer;
        SLIST_FOREACH_SAFE(finalizer, &info->finalizerHead, node, tempFinalizer) {
            if (finalizer->finalizeCallback) {
                finalizer->finalizeCallback(info->env, info->data, finalizer->finalizeHint);
            }
            free(finalizer);
        }
    }
    free(info);
}

static inline JSClassRef createExternalClass() {
    JSClassDefinition classDefinition = kJSClassDefinitionEmpty;
    classDefinition.attributes = kJSClassAttributeNoAutomaticPrototype;
    classDefinition.className = "External";
    classDefinition.finalize = ExternalFinalize;
    JSClassRef classRef = JSClassCreate(&classDefinition);

    return classRef;
}

static inline NAPIStatus wrap(NAPIEnv env, NAPIValue object, ExternalInfo **result) {
    NAPI_PREAMBLE(env);
    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->context, (JSValueRef) object), NAPIGenericFailure);
    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef) object, &env->lastException);
    CHECK_JSC(env);

    ExternalInfo *info = NULL;
    CHECK_NAPI(unwrap(env, object, &info));
    if (!info) {
        errno = 0;
        info = malloc(sizeof(ExternalInfo));
        if (errno == ENOMEM) {
            errno = 0;

            return setLastErrorCode(env, NAPIMemoryError);
        }
        info->env = env;
        info->data = NULL;
        SLIST_INIT(&info->finalizerHead);

        JSClassRef classRef = createExternalClass();

        JSObjectRef prototype = JSObjectMake(env->context, classRef, info);
        JSObjectSetPrototype(env->context, prototype, JSObjectGetPrototype(env->context, objectRef));
        JSObjectSetPrototype(env->context, objectRef, prototype);
    }

    *result = info;

    return clearLastError(env);
}

static bool addFinalizer(ExternalInfo *externalInfo, NAPIFinalize finalize, void *finalizeHint) {
    if (!externalInfo) {
        return false;
    }

    errno = 0;
    struct Finalizer *finalizer = malloc(sizeof(struct Finalizer));
    if (errno == ENOMEM) {
        return false;
    }
    finalizer->finalizeCallback = finalize;
    finalizer->finalizeHint = finalizeHint;

    SLIST_INSERT_HEAD(&externalInfo->finalizerHead, finalizer, node);

    return true;
}

NAPIStatus
napi_wrap(NAPIEnv env, NAPIValue jsObject, void *nativeObject, NAPIFinalize finalizeCallback, void *finalizeHint,
          NAPIRef *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, jsObject);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->context, (JSValueRef) jsObject), NAPIInvalidArg);

    ExternalInfo *info = NULL;
    CHECK_NAPI(wrap(env, jsObject, &info));
    RETURN_STATUS_IF_FALSE(env, !info->data, NAPIInvalidArg);
    if (finalizeCallback) {
        RETURN_STATUS_IF_FALSE(env, addFinalizer(info, finalizeCallback, finalizeHint), NAPIMemoryError);
    }
    info->data = nativeObject;

    // 当 napi_create_reference 失败的时候可以不回滚前面的 wrap 过程，因为这实际上是两步骤
    if (result) {
        CHECK_NAPI(napi_create_reference(env, jsObject, 0, result));
    }

    return clearLastError(env);
}

NAPIStatus napi_unwrap(NAPIEnv env, NAPIValue jsObject, void **result) {
    CHECK_ENV(env);
    CHECK_ARG(env, jsObject);
    CHECK_ARG(env, result);

    ExternalInfo *info = NULL;
    CHECK_NAPI(unwrap(env, jsObject, &info));
    RETURN_STATUS_IF_FALSE(env, info && info->data, NAPIInvalidArg);
    *result = info->data;

    return clearLastError(env);
}

NAPIStatus napi_remove_wrap(NAPIEnv env, NAPIValue jsObject, void **result) {
    CHECK_ENV(env);
    CHECK_ARG(env, jsObject);
    CHECK_ARG(env, result);

    // Once an object is wrapped, it stays wrapped in order to support finalizer callbacks.
    ExternalInfo *info = NULL;
    CHECK_NAPI(unwrap(env, jsObject, &info));
    RETURN_STATUS_IF_FALSE(env, info && info->data, NAPIInvalidArg);
    *result = info->data;
    info->data = NULL;

    return clearLastError(env);
}

static inline NAPIStatus create(NAPIEnv env,
                                void *data,
                                NAPIFinalize finalizeCB,
                                void *finalizeHint,
                                NAPIValue *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    errno = 0;
    ExternalInfo *info = malloc(sizeof(ExternalInfo));
    if (errno == ENOMEM) {
        errno = 0;

        return setLastErrorCode(env, NAPIMemoryError);
    }
    info->env = env;
    info->data = data;
    SLIST_INIT(&info->finalizerHead);

    JSClassRef classRef = createExternalClass();
    if (finalizeCB) {
        if (!addFinalizer(info, finalizeCB, finalizeHint)) {
            JSClassRelease(classRef);
            free(info);

            return setLastErrorCode(env, NAPIGenericFailure);
        }
    }
    *result = (NAPIValue) JSObjectMake(env->context, classRef, info);
    JSClassRelease(classRef);

    return clearLastError(env);
}

NAPIStatus
napi_create_external(NAPIEnv env, void *data, NAPIFinalize finalizeCB, void *finalizeHint, NAPIValue *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    CHECK_NAPI(create(env, data, finalizeCB, finalizeHint, result));

    return clearLastError(env);
}

NAPIStatus napi_get_value_external(NAPIEnv env, NAPIValue value, void **result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->context, (JSValueRef) value), NAPIObjectExpected);
    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef) value, &env->lastException);
    CHECK_JSC(env);

    ExternalInfo *info = JSObjectGetPrivate(objectRef);
    if (info) {
        *result = info->data;
    }

    return clearLastError(env);
}

static void referenceFinalize(NAPIEnv env, __attribute((unused)) void *finalizeData, void *finalizeHint) {
    ActiveReferenceValue *activeReferenceValue = NULL;
    // finalizeHint 实际上就是 JSValueRef
    // Allocation failure is possible only when adding elements to the hash table (including the ADD, REPLACE, and SELECT operations). uthash_free is not allowed to fail.
    HASH_FIND_PTR(env->activeReferenceValues, &finalizeHint, activeReferenceValue);
    if (activeReferenceValue) {
        HASH_DEL(env->activeReferenceValues, activeReferenceValue);
        free(activeReferenceValue);
    } else {
        // 正常销毁的时候应该存在
        assert(false);
    }
}

#define protect(reference) \
    do {                   \
        LIST_INSERT_HEAD(&env->strongReferenceHead, reference, node); \
        JSValueProtect(env->context, (reference)->value);  \
    } while (0)

NAPIStatus napi_create_reference(NAPIEnv env, NAPIValue value, uint32_t initialRefCount, NAPIRef *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    errno = 0;
    NAPIRef reference = malloc(sizeof(struct OpaqueNAPIRef));
    if (errno == ENOMEM) {
        errno = 0;

        return setLastErrorCode(env, NAPIMemoryError);
    }
    reference->value = (JSValueRef) value;
    reference->count = initialRefCount;

    ActiveReferenceValue *activeReferenceValue = NULL;
    HASH_FIND_PTR(env->activeReferenceValues, &value, activeReferenceValue);
    if (!activeReferenceValue) {
        errno = 0;
        activeReferenceValue = malloc(sizeof(ActiveReferenceValue));
        if (errno == ENOMEM) {
            errno = 0;
            free(reference);

            return setLastErrorCode(env, NAPIMemoryError);
        }
        hashError = false;
        HASH_ADD_PTR(env->activeReferenceValues, value, activeReferenceValue);
        if (hashError) {
            hashError = false;
            free(reference);
            free(activeReferenceValue);

            return setLastErrorCode(env, NAPIMemoryError);
        }
        // wrap
        ExternalInfo *externalInfo = NULL;
        CHECK_NAPI(wrap(env, value, &externalInfo));
        if (!addFinalizer(externalInfo, referenceFinalize, value)) {
            // 失败
            free(reference);
            HASH_DEL(env->activeReferenceValues, activeReferenceValue);
            free(activeReferenceValue);

            return setLastErrorCode(env, NAPIMemoryError);
        }
    }

    if (initialRefCount) {
        protect(reference);
    }

    return clearLastError(env);
}

#define unprotect(reference) \
    do { \
        LIST_REMOVE(reference, node); \
        JSValueProtect(env->context, (reference)->value); \
    } while (0)

NAPIStatus napi_delete_reference(NAPIEnv env, NAPIRef ref) {
    CHECK_ENV(env);
    CHECK_ARG(env, ref);

    if (ref->count) {
        unprotect(ref);
    }

    free(ref);

    return clearLastError(env);
}

NAPIStatus napi_reference_ref(NAPIEnv env, NAPIRef ref, uint32_t *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, ref);

    if (ref->count++ == 0) {
        protect(ref);
    }

    if (result) {
        *result = ref->count;
    }

    return clearLastError(env);
}

NAPIStatus napi_reference_unref(NAPIEnv env, NAPIRef ref, uint32_t *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, ref);

    if (--ref->count == 0) {
        unprotect(ref);
    }

    if (result) {
        *result = ref->count;
    }

    return clearLastError(env);
}

NAPIStatus napi_get_reference_value(NAPIEnv env, NAPIRef ref, NAPIValue *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, ref);
    CHECK_ARG(env, result);

    *result = NULL;
    ActiveReferenceValue *activeReferenceValue = NULL;
    HASH_FIND_PTR(env->activeReferenceValues, &ref->value, activeReferenceValue);
    if (activeReferenceValue) {
        *result = (NAPIValue) ref->value;
    }

    return clearLastError(env);
}

NAPIStatus napi_open_handle_scope(NAPIEnv env, NAPIHandleScope *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIHandleScope) 1;

    return clearLastError(env);
}

NAPIStatus napi_close_handle_scope(NAPIEnv env, NAPIHandleScope scope) {
    CHECK_ENV(env);
    CHECK_ARG(env, scope);

    return clearLastError(env);
}

NAPIStatus napi_open_escapable_handle_scope(NAPIEnv env, NAPIEscapableHandleScope *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = (NAPIEscapableHandleScope) 1;

    return clearLastError(env);
}

NAPIStatus napi_close_escapable_handle_scope(NAPIEnv env, NAPIEscapableHandleScope scope) {
    CHECK_ENV(env);
    CHECK_ARG(env, scope);

    return clearLastError(env);
}

NAPIStatus napi_escape_handle(NAPIEnv env, NAPIEscapableHandleScope scope, NAPIValue escapee, NAPIValue *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, scope);
    CHECK_ARG(env, escapee);
    CHECK_ARG(env, result);

    *result = escapee;

    return clearLastError(env);
}

NAPIStatus napi_throw(NAPIEnv env, NAPIValue error) {
    CHECK_ENV(env);
    CHECK_ARG(env, error);

    env->lastException = (JSValueRef) error;

    return clearLastError(env);
}

NAPIStatus napi_throw_error(NAPIEnv env, const char *code, const char *msg) {
    CHECK_ENV(env);

    JSStringRef stringRef = JSStringCreateWithUTF8CString(code);
    JSValueRef codeValue = JSValueMakeString(env->context, stringRef);
    JSStringRelease(stringRef);
    stringRef = JSStringCreateWithUTF8CString(msg);
    JSValueRef msgValue = JSValueMakeString(env->context, stringRef);
    JSStringRelease(stringRef);
    NAPIValue error = NULL;
    CHECK_NAPI(napi_create_error(env, (NAPIValue) codeValue, (NAPIValue) msgValue, &error));

    return napi_throw(env, error);
}

NAPIStatus napi_throw_type_error(NAPIEnv env, const char *code, const char *msg) {
    CHECK_ENV(env);

    JSStringRef stringRef = JSStringCreateWithUTF8CString(code);
    JSValueRef codeValue = JSValueMakeString(env->context, stringRef);
    JSStringRelease(stringRef);
    stringRef = JSStringCreateWithUTF8CString(msg);
    JSValueRef msgValue = JSValueMakeString(env->context, stringRef);
    JSStringRelease(stringRef);
    NAPIValue error = NULL;
    CHECK_NAPI(napi_create_type_error(env, (NAPIValue) codeValue, (NAPIValue) msgValue, &error));

    return napi_throw(env, error);
}

NAPIStatus napi_throw_range_error(NAPIEnv env, const char *code, const char *msg) {
    CHECK_ENV(env);

    JSStringRef stringRef = JSStringCreateWithUTF8CString(code);
    JSValueRef codeValue = JSValueMakeString(env->context, stringRef);
    JSStringRelease(stringRef);
    stringRef = JSStringCreateWithUTF8CString(msg);
    JSValueRef msgValue = JSValueMakeString(env->context, stringRef);
    JSStringRelease(stringRef);
    NAPIValue error = NULL;
    CHECK_NAPI(napi_create_range_error(env, (NAPIValue) codeValue, (NAPIValue) msgValue, &error));

    return napi_throw(env, error);
}

NAPIStatus napi_is_error(NAPIEnv env, NAPIValue value, bool *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    NAPIValue global = NULL;
    NAPIValue errorCtor = NULL;
    CHECK_NAPI(napi_get_global(env, &global));
    CHECK_NAPI(napi_get_named_property(env, global, "Error", &errorCtor));
    CHECK_NAPI(napi_instanceof(env, value, errorCtor, result));

    return clearLastError(env);
}

NAPIStatus napi_is_exception_pending(NAPIEnv env, bool *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = env->lastException;

    return clearLastError(env);
}

NAPIStatus napi_get_and_clear_last_exception(NAPIEnv env, NAPIValue *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    if (!env->lastException) {
        return napi_get_undefined(env, result);
    } else {
        *result = (NAPIValue) env->lastException;
        env->lastException = NULL;
    }

    return clearLastError(env);
}

typedef struct {
    NAPIValue resolve;
    NAPIValue reject;
} Wrapper;

static NAPIValue callback(__attribute((unused)) NAPIEnv env, NAPICallbackInfo callbackInfo) {
    Wrapper *wrapper = (Wrapper *) callbackInfo->data;
    wrapper->resolve = (NAPIValue) callbackInfo->argv[0];
    wrapper->reject = (NAPIValue) callbackInfo->argv[1];

    return NULL;
}

NAPIStatus napi_create_promise(NAPIEnv env,
                               NAPIDeferred *deferred,
                               NAPIValue *promise) {
    CHECK_ENV(env);
    CHECK_ARG(env, deferred);
    CHECK_ARG(env, promise);

    NAPIValue global = NULL;
    NAPIValue promiseCtor = NULL;
    CHECK_NAPI(napi_get_global(env, &global));
    CHECK_NAPI(napi_get_named_property(env, global, "Promise", &promiseCtor));

    Wrapper wrapper;

    NAPIValue executor = NULL;
    CHECK_NAPI(napi_create_function(env, "executor", NAPI_AUTO_LENGTH, callback, &wrapper, &executor));
    CHECK_NAPI(napi_new_instance(env, promiseCtor, 1, &executor, promise));

    NAPIValue deferredValue = NULL;
    CHECK_NAPI(napi_create_object(env, &deferredValue));
    CHECK_NAPI(napi_set_named_property(env, deferredValue, "resolve", wrapper.resolve));
    CHECK_NAPI(napi_set_named_property(env, deferredValue, "reject", wrapper.reject));

    NAPIRef deferredRef = NULL;
    CHECK_NAPI(napi_create_reference(env, deferredValue, 1, &deferredRef));
    *deferred = (NAPIDeferred) (deferredRef);

    return clearLastError(env);
}

NAPIStatus napi_resolve_deferred(NAPIEnv env,
                                 NAPIDeferred deferred,
                                 NAPIValue resolution) {
    CHECK_ENV(env);
    CHECK_ARG(env, deferred);
    CHECK_ARG(env, resolution);

    NAPIRef deferredRef = (NAPIRef) deferred;
    NAPIValue undefined = NULL;
    NAPIValue deferredValue = NULL;
    NAPIValue resolve = NULL;
    CHECK_NAPI(napi_get_undefined(env, &undefined));
    CHECK_NAPI(napi_get_reference_value(env, deferredRef, &deferredValue));
    CHECK_NAPI(napi_get_named_property(env, deferredValue, "resolve", &resolve));
    CHECK_NAPI(napi_call_function(env, undefined, resolve, 1, &resolution, NULL));
    CHECK_NAPI(napi_delete_reference(env, deferredRef));

    return clearLastError(env);
}

NAPIStatus napi_reject_deferred(NAPIEnv env,
                                NAPIDeferred deferred,
                                NAPIValue rejection) {
    CHECK_ENV(env);
    CHECK_ARG(env, deferred);

    NAPIRef deferredRef = (NAPIRef) deferred;
    NAPIValue undefined, deferredValue, reject;
    CHECK_NAPI(napi_get_undefined(env, &undefined));
    CHECK_NAPI(napi_get_reference_value(env, deferredRef, &deferredValue));
    CHECK_NAPI(napi_get_named_property(env, deferredValue, "reject", &reject));
    CHECK_NAPI(napi_call_function(env, undefined, reject, 1, &rejection, NULL));
    CHECK_NAPI(napi_delete_reference(env, deferredRef));

    return clearLastError(env);
}

NAPIStatus napi_is_promise(NAPIEnv env,
                           NAPIValue value,
                           bool *isPromise) {
    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, isPromise);

    NAPIValue global, promiseCtor;
    CHECK_NAPI(napi_get_global(env, &global));
    CHECK_NAPI(napi_get_named_property(env, global, "Promise", &promiseCtor));
    CHECK_NAPI(napi_instanceof(env, value, promiseCtor, isPromise));

    return clearLastError(env);
}

NAPIStatus napi_run_script(NAPIEnv env,
                           NAPIValue script,
                           NAPIValue *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, script);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env, JSValueIsString(env->context, (JSValueRef) script), NAPIStringExpected);

    JSStringRef script_str = JSValueToStringCopy(env->context, (JSValueRef) script, &env->lastException);
    CHECK_JSC(env);

    *result = (NAPIValue) JSEvaluateScript(
            env->context, script_str, NULL, NULL, 1, &env->lastException);
    CHECK_JSC(env);

    return clearLastError(env);
}

NAPIStatus
NAPIRunScriptWithSourceUrl(NAPIEnv env, const char *utf8Script, const char *utf8SourceUrl, NAPIValue *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, utf8Script);
    CHECK_ARG(env, result);

    JSStringRef scriptStringRef = JSStringCreateWithUTF8CString(utf8Script);
    JSStringRef sourceUrl = NULL;
    if (utf8SourceUrl) {
        sourceUrl = JSStringCreateWithUTF8CString(utf8SourceUrl);
    }

    *result = (NAPIValue) JSEvaluateScript(env->context, scriptStringRef, NULL, sourceUrl, 1, &env->lastException);
    CHECK_JSC(env);

    return clearLastError(env);
}

static JSContextGroupRef virtualMachine = NULL;

static uint32_t contextCount = 0;

NAPIStatus NAPICreateEnv(NAPIEnv *env) {
    // *env 才是 NAPIEnv
    CHECK_ENV(env);

    errno = 0;
    *env = malloc(sizeof(struct OpaqueNAPIEnv));
    if (errno == ENOMEM) {
        errno = 0;

        return NAPIMemoryError;
    }

    if ((virtualMachine && !contextCount) || (!virtualMachine && contextCount)) {
        assert(false);

        return NAPIGenericFailure;
    }

    if (!virtualMachine) {
        virtualMachine = JSContextGroupCreate();
    }
    contextCount += 1;
    (*env)->context = JSGlobalContextCreateInGroup(virtualMachine, NULL);
    (*env)->lastException = NULL;
    (*env)->lastError.errorCode = NAPIOK;
    (*env)->lastError.errorMessage = NULL;
    (*env)->lastError.engineReserved = NULL;
    (*env)->lastError.engineErrorCode = 0;
    LIST_INIT(&(*env)->strongReferenceHead);
    (*env)->activeReferenceValues = NULL;

    return NAPIOK;
}

NAPIStatus NAPIFreeEnv(NAPIEnv env) {
    CHECK_ENV(env);

    ActiveReferenceValue *element, *temp;
    HASH_ITER(hh, env->activeReferenceValues, element, temp) {
        HASH_DEL(env->activeReferenceValues, element);
        free(element);
    }

    NAPIRef reference, tempReference;

    LIST_FOREACH_SAFE(reference, &env->strongReferenceHead, node, tempReference) {
        LIST_REMOVE(reference, node);
        if (reference->count) {
            unprotect(reference);
        }
        free(reference);
    }
    JSGlobalContextRelease(env->context);
    if (--contextCount == 0 && virtualMachine) {
        // virtualMachine 不能为 NULL
        JSContextGroupRelease(virtualMachine);
        virtualMachine = NULL;
    }
    free(env);

    return NAPIOK;
}
