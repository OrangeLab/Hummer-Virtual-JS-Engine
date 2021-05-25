// 所有函数都可能抛出 NAPIInvalidArg
// 当返回值为 NAPIOK 时候，必须保证 result 可用，但是发生错误的时候，result 也可能会被修改，所以建议不要对 result 做
// NULL 初始化

#include <assert.h>
#include <napi/js_native_api.h>
//#include <quickjs-libc.h>
#include <quickjs.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#define CHECK_ENV(env)                                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(env))                                                                                                    \
        {                                                                                                              \
            return NAPIInvalidArg;                                                                                     \
        }                                                                                                              \
    } while (0)

static inline NAPIStatus setLastErrorCode(NAPIEnv env, NAPIStatus errorCode);

#define RETURN_STATUS_IF_FALSE(env, condition, status)                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            return setLastErrorCode(env, status);                                                                      \
        }                                                                                                              \
    } while (0)

// This does not call napi_set_last_error because the expression
// is assumed to be a NAPI function call that already did.
#define CHECK_NAPI(expr)                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        NAPIStatus status = expr;                                                                                      \
        if (status != NAPIOK)                                                                                          \
        {                                                                                                              \
            return status;                                                                                             \
        }                                                                                                              \
    } while (0)

static inline NAPIStatus clearLastError(NAPIEnv env);

#define CHECK_ARG(env, arg) RETURN_STATUS_IF_FALSE(env, arg, NAPIInvalidArg)

// JS_GetException 会转移所有权
// JS_Throw 会获取所有权
// 所以如果存在异常，先获取所有权，再交还所有权，最终所有权依旧在
// JSContext 中，QuickJS 目前 JS null 代表正常，未来考虑写成这样
// !JS_IsUndefined(exceptionValue) && !JS_IsNull(exceptionValue)
// NAPI_PREAMBLE 同时会检查 env->context
#define NAPI_PREAMBLE(env)                                                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        CHECK_ENV(env);                                                                                                \
        CHECK_ARG(env, (env)->context);                                                                                \
        JSValue exceptionValue = JS_GetException((env)->context);                                                      \
        if (!JS_IsNull(exceptionValue))                                                                                \
        {                                                                                                              \
            JS_Throw((env)->context, exceptionValue);                                                                  \
                                                                                                                       \
            return setLastErrorCode(env, NAPIPendingException);                                                        \
        }                                                                                                              \
        clearLastError(env);                                                                                           \
    } while (0)

struct Handle
{
    JSValue value; // 64/128
    SLIST_ENTRY(Handle)
    node; // 32/64
};

struct OpaqueNAPIHandleScope
{
    SLIST_ENTRY(OpaqueNAPIHandleScope)
    node; // 32/64
    SLIST_HEAD(, Handle)
    handleList; // 32/64
};

// typedef struct
//{
//     const char *errorMessage; // 32/64
//     void *engineReserved; // 32/64
//     uint32_t engineErrorCode; // 32
//     NAPIStatus errorCode; // enum，有需要的话根据 iOS 或 Android 的 clang
//     情况做单独处理
// } NAPIExtendedErrorInfo;
struct OpaqueNAPIEnv
{
    JSContext *context; // 32/64
    SLIST_HEAD(, OpaqueNAPIHandleScope)
    handleScopeList; // 32/64
    NAPIExtendedErrorInfo lastError;
};

// static inline
NAPIStatus setLastErrorCode(NAPIEnv env, NAPIStatus errorCode)
{
    CHECK_ENV(env);

    env->lastError.errorCode = errorCode;
#ifndef NDEBUG
    if (errorCode == NAPIPendingException)
    {
        CHECK_ARG(env, env->context);
        JSValue exceptionValue = JS_GetException(env->context);
        if (JS_IsNull(exceptionValue))
        {
            assert(false);
        }
        else
        {
            JS_Throw(env->context, exceptionValue);
        }
    }
#endif

    return errorCode;
}

// static inline
NAPIStatus clearLastError(NAPIEnv env)
{
    // env 空指针检查
    CHECK_ENV(env);

    env->lastError.errorCode = NAPIOK;
    // TODO(boingoing): Should this be a callback?
    env->lastError.engineErrorCode = 0;
    env->lastError.engineReserved = NULL;

    return NAPIOK;
}

// 这个函数不会修改引用计数和所有权
// NAPIHandleScopeMismatch/NAPIMemoryError
static inline NAPIStatus addValueToHandleScope(NAPIEnv env, JSValue value, struct Handle **result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env, !SLIST_EMPTY(&(*env).handleScopeList), NAPIHandleScopeMismatch);
    *result = malloc(sizeof(struct Handle));
    RETURN_STATUS_IF_FALSE(env, *result, NAPIMemoryError);
    (*result)->value = value;
    NAPIHandleScope handleScope = SLIST_FIRST(&(env)->handleScopeList);
    SLIST_INSERT_HEAD(&handleScope->handleList, *result, node);

    return clearLastError(env);
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

// + addValueToHandleScope
NAPIStatus napi_get_undefined(NAPIEnv env, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    JSValue value = JS_UNDEFINED;
    struct Handle *handle;
    CHECK_NAPI(addValueToHandleScope(env, value, &handle));
    *result = (NAPIValue)&handle->value;

    return clearLastError(env);
}

// + addValueToHandleScope
NAPIStatus napi_get_null(NAPIEnv env, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    JSValue value = JS_NULL;
    struct Handle *handle;
    CHECK_NAPI(addValueToHandleScope(env, value, &handle));
    *result = (NAPIValue)&handle->value;

    return clearLastError(env);
}

// + addValueToHandleScope
NAPIStatus napi_get_global(NAPIEnv env, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    // JS_GetGlobalObject 不能传入 NULL
    CHECK_ARG(env, env->context);

    // JS_GetGlobalObject 返回已经引用计数 +1
    // 实际上 globalValue 可能为 JS_EXCEPTION
    JSValue globalValue = JS_GetGlobalObject(env->context);
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(globalValue), NAPIPendingException);
    //    RETURN_STATUS_IF_FALSE(env, !JS_IsException(globalValue), NAPIMemoryError);
    struct Handle *globalHandle;
    NAPIStatus status = addValueToHandleScope(env, globalValue, &globalHandle);
    if (status != NAPIOK)
    {
        JS_FreeValue(env->context, globalValue);

        return setLastErrorCode(env, status);
    }
    *result = (NAPIValue)&globalHandle->value;

    return clearLastError(env);
}

// + addValueToHandleScope
NAPIStatus napi_get_boolean(NAPIEnv env, bool value, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->context);

    JSValue jsValue = JS_NewBool(env->context, value);
    struct Handle *handle;
    CHECK_NAPI(addValueToHandleScope(env, jsValue, &handle));
    *result = (NAPIValue)&handle->value;

    return clearLastError(env);
}

// + addValueToHandleScope
NAPIStatus napi_create_object(NAPIEnv env, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    // JS_NewObject 不能传入 NULL
    CHECK_ARG(env, env->context);

    // 初始引用计数为 1
    JSValue objectValue = JS_NewObject(env->context);
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(objectValue), NAPIPendingException);
    struct Handle *objectHandle;
    NAPIStatus status = addValueToHandleScope(env, objectValue, &objectHandle);
    if (status != NAPIOK)
    {
        JS_FreeValue(env->context, objectValue);

        return setLastErrorCode(env, status);
    }
    *result = (NAPIValue)&objectHandle->value;

    return clearLastError(env);
}

// + addValueToHandleScope
NAPIStatus napi_create_array(NAPIEnv env, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->context);

    JSValue arrayValue = JS_NewArray(env->context);
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(arrayValue), NAPIPendingException);
    struct Handle *arrayHandle;
    NAPIStatus status = addValueToHandleScope(env, arrayValue, &arrayHandle);
    if (status != NAPIOK)
    {
        JS_FreeValue(env->context, arrayValue);

        return setLastErrorCode(env, status);
    }
    *result = (NAPIValue)&arrayHandle->value;

    return clearLastError(env);
}

// 本质为 const array = new Array(); array.length = length;
// NAPIPendingException/NAPIGenericFailure + addValueToHandleScope
NAPIStatus napi_create_array_with_length(NAPIEnv env, size_t length, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    JSValue arrayValue = JS_NewArray(env->context);
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(arrayValue), NAPIPendingException);

    int returnValue = JS_SetPropertyStr(env->context, arrayValue, "length",
                                        JS_DupValue(env->context, JS_NewInt64(env->context, (int64_t)length)));
    RETURN_STATUS_IF_FALSE(env, returnValue != -1, NAPIPendingException);
    RETURN_STATUS_IF_FALSE(env, returnValue != false, NAPIGenericFailure);

    struct Handle *arrayHandle;
    NAPIStatus status = addValueToHandleScope(env, arrayValue, &arrayHandle);
    if (status != NAPIOK)
    {
        JS_FreeValue(env->context, arrayValue);

        return setLastErrorCode(env, status);
    }
    *result = (NAPIValue)&arrayHandle->value;

    return clearLastError(env);
}

// + addValueToHandleScope
NAPIStatus napi_create_double(NAPIEnv env, double value, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    // JS_NewFloat64 实际上不关心 env->context
    JSValue jsValue = JS_NewFloat64(env->context, value);
    struct Handle *handle;
    NAPIStatus status = addValueToHandleScope(env, jsValue, &handle);
    RETURN_STATUS_IF_FALSE(env, status == NAPIOK, status);
    *result = (NAPIValue)&handle->value;

    return clearLastError(env);
}

// + addValueToHandleScope
NAPIStatus napi_create_int32(NAPIEnv env, int32_t value, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    // JS_NewFloat64 实际上不关心 env->context
    JSValue jsValue = JS_NewInt32(env->context, value);
    struct Handle *handle;
    NAPIStatus status = addValueToHandleScope(env, jsValue, &handle);
    RETURN_STATUS_IF_FALSE(env, status == NAPIOK, status);
    *result = (NAPIValue)&handle->value;

    return clearLastError(env);
}

// + addValueToHandleScope
NAPIStatus napi_create_uint32(NAPIEnv env, uint32_t value, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    // JS_NewFloat64 实际上不关心 env->context
    JSValue jsValue = JS_NewInt64(env->context, value);
    struct Handle *handle;
    NAPIStatus status = addValueToHandleScope(env, jsValue, &handle);
    RETURN_STATUS_IF_FALSE(env, status == NAPIOK, status);
    *result = (NAPIValue)&handle->value;

    return clearLastError(env);
}

// + addValueToHandleScope
NAPIStatus napi_create_int64(NAPIEnv env, int64_t value, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    // JS_NewFloat64 实际上不关心 env->context
    JSValue jsValue = JS_NewInt64(env->context, value);
    struct Handle *handle;
    NAPIStatus status = addValueToHandleScope(env, jsValue, &handle);
    RETURN_STATUS_IF_FALSE(env, status == NAPIOK, status);
    *result = (NAPIValue)&handle->value;

    return clearLastError(env);
}

// V8 引擎传入 NULL 直接崩溃
// NAPIPendingException/NAPIMemoryError + addValueToHandleScope
NAPIStatus napi_create_string_utf8(NAPIEnv env, const char *str, size_t length, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->context);

    if (length == NAPI_AUTO_LENGTH)
    {
        if (str)
        {
            length = strlen(str);
        }
        else
        {
            // str 依旧保持 NULL
            length = 0;
        }
    }
    // length == 0 的情况下会返回 ""
    JSValue stringValue = JS_NewStringLen(env->context, str, length);
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(stringValue), NAPIPendingException);
    struct Handle *stringHandle;
    CHECK_NAPI(addValueToHandleScope(env, stringValue, &stringHandle));
    *result = (NAPIValue)&stringHandle->value;

    return clearLastError(env);
}

// V8 引擎传入 NULL 返回 ""
// NAPIPendingException/NAPIMemoryError/NAPIGenericFailure + addValueToHandleScope
NAPIStatus napi_create_string_utf16(NAPIEnv env, const char16_t *str, size_t length, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    if (length == NAPI_AUTO_LENGTH)
    {
        if (str)
        {
            length = strlen((const char *)str);
            RETURN_STATUS_IF_FALSE(env, !(length % 2), NAPIGenericFailure);
            // 0, 2, 4...
            if (length)
            {
                length /= 2;
            }
        }
        else
        {
            length = 0;
        }
    }
    uint8_t *utf8String = NULL;
    size_t utf8StringLength = 0;
    if (length)
    {
        uint16_t value = 0x0102;
        if (*((uint8_t *)&value) == 1)
        {
            // 大端
            uint16_t *newStr = malloc(sizeof(uint16_t) * length);
            RETURN_STATUS_IF_FALSE(env, str, NAPIMemoryError);
            for (size_t i = 0; i < length; ++i)
            {
                // >>, << 优先级比 | 高
                newStr[i] = str[i] << 8 | str[i] >> 8;
            }
            str = newStr;
        }
        // 这里不需要考虑 \0
        // 0x0 -> 0x10ffff UTF-32 UTF-16 包含的 Unicode，UTF-8 正常应当可以扩展至 6 字节
        utf8String = malloc(sizeof(char) * 4 * length);
        if (!utf8String)
        {
            if (*((uint8_t *)&value) == 1)
            {
                // 大端
                free((void *)str);
            }

            return setLastErrorCode(env, NAPIMemoryError);
        }
        // UTF-16 -> UTF-8
        int i = 0;
        int j = 0;
        uint32_t unicodeValue = 0;
        if (str[i] >= 0xd800 && str[i] <= 0xdbff)
        {
            // 代理平面
            if (i + 1 == length)
            {
                // 只有 head，缺少 trail，UTF-16 字符串不完整
                free(utf8String);
                if (*((uint8_t *)&value) == 1)
                {
                    // 大端
                    free((void *)str);
                }

                return setLastErrorCode(env, NAPIGenericFailure);
            }
            uint16_t head = str[i] & 0x400;
            // 读取 tail
            uint16_t trail = str[++i];
            if (trail < 0xdc00 || trail > 0xdfff)
            {
                // tail 非法
                free(utf8String);
                if (*((uint8_t *)&value) == 1)
                {
                    // 大端
                    free((void *)str);
                }

                return setLastErrorCode(env, NAPIGenericFailure);
            }
            trail = trail & 0x400;
            unicodeValue = ((uint32_t)head) << 10 | trail | 0x10000;
        }
        else
        {
            // Unicode
            unicodeValue = str[i];
        }
        uint8_t bytesToWrite = 0;
        if (unicodeValue < 0x80)
        {
            // 0xxxxxxx -> 7
            bytesToWrite = 1;
//            utf8String[j++] = unicodeValue;
        } else if (unicodeValue < 0x800) {
            // 110xxxxx 10xxxxxx -> 11
            bytesToWrite = 2;
//            utf8String[j++] = (0x7c0 & unicodeValue) >> 6 | 0xc0;
//            utf8String[j++] = 0x3f & unicodeValue | 0x80;
        } else if (unicodeValue < 0x10000) {
            // 1110xxxx 10xxxxxx 10xxxxxx
            bytesToWrite = 3;
//            utf8String[j++] = (0xf000 & unicodeValue) >> 12 | 0xf;
//            utf8String[j++] = (0xfc0 & unicodeValue) >> 6 | 0x80;
//            utf8String[j++] = 0x3f & unicodeValue | 0x80;
        } else if (unicodeValue < 0x110000) {
            // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            bytesToWrite = 4;
//            utf8String[j++] = (0xf000 & unicodeValue) >> 18 | 0xf;
//            utf8String[j++] = (0xfc0 & unicodeValue) >> 6 | 0x80;
//            utf8String[j++] = 0x3f & unicodeValue | 0x80;
        } else {
            free(utf8String);
            if (*((uint8_t *)&value) == 1)
            {
                // 大端
                free((void *)str);
            }

            return setLastErrorCode(env, NAPIGenericFailure);
        }
        j += bytesToWrite - 1;
        uint8_t *target = &utf8String[j - 1];
        switch (bytesToWrite)
        {
        case 4:
        case 3:
        case 2:
        case 1:

        }

        if (*((uint8_t *)&value) == 1)
        {
            // 大端
            free((void *)str);
        }
    }

    JSValue stringValue = JS_NewStringLen(env->context, (char *)utf8String, utf8StringLength);
    free(utf8String);
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(stringValue), NAPIPendingException);
    struct Handle *stringHandle;
    CHECK_NAPI(addValueToHandleScope(env, stringValue, &stringHandle));
    *result = (NAPIValue)&stringHandle->value;

    return clearLastError(env);
}

NAPIStatus napi_typeof(NAPIEnv env, NAPIValue value, NAPIValueType *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->context);

    // TODO(ChasonTang): NAPIExternal after NAPIFunction before NAPIObject
    JSValue jsValue = *((JSValue *)value);
    if (JS_IsNumber(jsValue))
    {
        *result = NAPINumber;
    }
    else if (JS_IsString(jsValue))
    {
        *result = NAPIString;
    }
    else if (JS_IsFunction(env->context, jsValue))
    {
        *result = NAPIFunction;
    }
    else if (JS_IsObject(jsValue))
    {
        *result = NAPIObject;
    }
    else if (JS_IsBool(jsValue))
    {
        *result = NAPIBoolean;
    }
    else if (JS_IsUndefined(jsValue))
    {
        *result = NAPIUndefined;
    }
    else if (JS_IsNull(jsValue))
    {
        *result = NAPINull;
    }
    else
    {
        // Should not get here unless V8 has added some new kind of value.
        return setLastErrorCode(env, NAPIInvalidArg);
    }

    return clearLastError(env);
}

// NAPINumberExpected
NAPIStatus napi_get_value_double(NAPIEnv env, NAPIValue value, double *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    JSValue jsValue = *((JSValue *)value);
    int tag = JS_VALUE_GET_TAG(jsValue);
    if (tag == JS_TAG_INT)
    {
        *result = JS_VALUE_GET_INT(jsValue);
    }
    else if (JS_TAG_IS_FLOAT64(tag))
    {
        *result = JS_VALUE_GET_FLOAT64(jsValue);
    }
    else
    {
        return setLastErrorCode(env, NAPINumberExpected);
    }

    return clearLastError(env);
}

NAPIStatus napi_get_value_int32(NAPIEnv env, NAPIValue value, int32_t *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    JSValue jsValue = *((JSValue *)value);
    int tag = JS_VALUE_GET_TAG(jsValue);
    if (tag == JS_TAG_INT)
    {
        *result = JS_VALUE_GET_INT(jsValue);
    }
    else if (JS_TAG_IS_FLOAT64(tag))
    {
        *result = JS_VALUE_GET_FLOAT64(jsValue);
    }
    else
    {
        return setLastErrorCode(env, NAPINumberExpected);
    }

    return clearLastError(env);
}

NAPIStatus napi_set_named_property(NAPIEnv env, NAPIValue object, const char *utf8name, NAPIValue value)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);

    CHECK_ARG(env, env->context);

    if (!utf8name)
    {
        utf8name = "";
    }

    RETURN_STATUS_IF_FALSE(env, JS_IsObject(*((JSValue *)object)), NAPIObjectExpected);
    // utf8name 传入 NULL 会崩溃
    // JS_SetPropertyStr 会获取所有权，因此需要
    // JS_DupValue，并且最好使用返回值传递
    int returnValue =
        JS_SetPropertyStr(env->context, *((JSValue *)object), utf8name, JS_DupValue(env->context, *((JSValue *)value)));
    RETURN_STATUS_IF_FALSE(env, returnValue != -1, NAPIPendingException);
    RETURN_STATUS_IF_FALSE(env, returnValue != false, NAPIGenericFailure);

    return clearLastError(env);
}

NAPIStatus napi_open_handle_scope(NAPIEnv env, NAPIHandleScope *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = malloc(sizeof(struct OpaqueNAPIHandleScope));
    if (!*result)
    {
        return NAPIMemoryError;
    }
    SLIST_INIT(&(*result)->handleList);
    SLIST_INSERT_HEAD(&env->handleScopeList, *result, node);

    return clearLastError(env);
}

NAPIStatus napi_close_handle_scope(NAPIEnv env, NAPIHandleScope scope)
{
    CHECK_ENV(env);
    CHECK_ARG(env, scope);

    // JS_FreeValue 不能传入 NULL
    CHECK_ARG(env, env->context);

    RETURN_STATUS_IF_FALSE(env, SLIST_FIRST(&env->handleScopeList) == scope, NAPIHandleScopeMismatch);
    struct Handle *handle, *tempHandle;
    SLIST_FOREACH_SAFE(handle, &scope->handleList, node, tempHandle)
    {
        JS_FreeValue(env->context, handle->value);
        free(handle);
    }
    SLIST_REMOVE_HEAD(&env->handleScopeList, node);
    free(scope);

    return clearLastError(env);
}

NAPIStatus napi_get_and_clear_last_exception(NAPIEnv env, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    // JS_GetException
    CHECK_ARG(env, env->context);

    JSValue exceptionValue = JS_GetException(env->context);
    if (JS_IsNull(exceptionValue))
    {
        exceptionValue = JS_UNDEFINED;
    }
    struct Handle *exceptionHandle;
    NAPIStatus status = addValueToHandleScope(env, exceptionValue, &exceptionHandle);
    if (status != NAPIOK)
    {
        JS_FreeValue(env->context, exceptionValue);

        return setLastErrorCode(env, status);
    }
    *result = (NAPIValue)&exceptionHandle->value;

    return clearLastError(env);
}

NAPIStatus NAPIRunScriptWithSourceUrl(NAPIEnv env, const char *utf8Script, const char *utf8SourceUrl, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, utf8Script);
    CHECK_ARG(env, result);

    // JS_Eval/JS_GetRuntime 不能传入 NULL
    CHECK_ARG(env, env->context);

    // utf8SourceUrl 传入 NULL 会导致崩溃，JS_NewAtom 调用 strlen 不能传入 NULL
    if (!utf8SourceUrl)
    {
        utf8SourceUrl = "";
    }

    JSValue returnValue = JS_Eval(env->context, utf8Script, strlen(utf8Script), utf8SourceUrl, JS_EVAL_TYPE_GLOBAL);
    for (int err = 1; err > 0;)
    {
        JSContext *context;
        err = JS_ExecutePendingJob(JS_GetRuntime(env->context), &context);
        if (err)
        {
            // 正常情况下 JS_ExecutePendingJob 返回 -1
            // 代表引擎内部异常，比如内存分配失败等，未来如果真产生了 JS
            // 异常，也直接吃掉
            JSValue inlineExceptionValue = JS_GetException(context);
            JS_FreeValue(context, inlineExceptionValue);
        }
    }
    //    if (JS_IsException(returnValue))
    //    {
    //        js_std_dump_error(env->context);
    //
    //        return setLastErrorCode(env, NAPIPendingException);
    //    }
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(returnValue), NAPIPendingException);
    struct Handle *returnHandle;
    NAPIStatus status = addValueToHandleScope(env, returnValue, &returnHandle);
    if (status != NAPIOK)
    {
        JS_FreeValue(env->context, returnValue);

        return setLastErrorCode(env, status);
    }
    *result = (NAPIValue)&returnHandle->value;

    return clearLastError(env);
}

static JSRuntime *runtime = NULL;

static uint8_t contextCount = 0;

NAPIStatus NAPICreateEnv(NAPIEnv *env)
{
    if (!env)
    {
        return NAPIInvalidArg;
    }
    if ((runtime && !contextCount) || (!runtime && contextCount))
    {
        assert(false);

        return NAPIGenericFailure;
    }
    *env = malloc(sizeof(struct OpaqueNAPIEnv));
    if (!*env)
    {
        return NAPIMemoryError;
    }
    if (!runtime)
    {
        runtime = JS_NewRuntime();
        if (!runtime)
        {
            free(*env);

            return NAPIMemoryError;
        }
    }
    JSContext *context = JS_NewContext(runtime);
    if (!context)
    {
        free(*env);
        JS_FreeRuntime(runtime);
        runtime = NULL;

        return NAPIMemoryError;
    }
    contextCount += 1;
    (*env)->context = context;
    (*env)->lastError.engineErrorCode = 0;
    (*env)->lastError.engineReserved = NULL;
    (*env)->lastError.errorMessage = NULL;
    (*env)->lastError.errorCode = NAPIOK;
    SLIST_INIT(&(*env)->handleScopeList);

    return NAPIOK;
}

NAPIStatus NAPIFreeEnv(NAPIEnv env)
{
    CHECK_ENV(env);

    NAPIHandleScope handleScope, tempHandleScope;
    SLIST_FOREACH_SAFE(handleScope, &(*env).handleScopeList, node, tempHandleScope)
    {
        if (napi_close_handle_scope(env, handleScope) != NAPIOK)
        {
            assert(false);
        }
    }

    JS_FreeContext(env->context);
    if (--contextCount == 0 && runtime)
    {
        // virtualMachine 不能为 NULL
        JS_FreeRuntime(runtime);
        runtime = NULL;
    }
    free(env);

    return NAPIOK;
}