// 所有函数都可能抛出 NAPIInvalidArg NAPIOK
// 当返回值为 NAPIOK 时候，必须保证 result 可用，但是发生错误的时候，result 也可能会被修改，所以建议不要对 result 做
// NULL 初始化

#include <assert.h>
#include <napi/js_native_api.h>
//#include <quickjs-libc.h>
#include <cutils.h>
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

static NAPIStatus setLastErrorCode(NAPIEnv env, NAPIStatus errorCode);

#define RETURN_STATUS_IF_FALSE(env, condition, status)                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            return setLastErrorCode(env, status);                                                                      \
        }                                                                                                              \
    } while (0)

#define CHECK_NAPI(expr)                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        NAPIStatus status = expr;                                                                                      \
        if (status != NAPIOK)                                                                                          \
        {                                                                                                              \
            return status;                                                                                             \
        }                                                                                                              \
    } while (0)

#define CHECK_ARG(env, arg) RETURN_STATUS_IF_FALSE(env, arg, NAPIInvalidArg)

static NAPIStatus clearLastError(NAPIEnv env);

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
    JSValue value;            // 64/128
    SLIST_ENTRY(Handle) node; // size_t
};

struct OpaqueNAPIHandleScope
{
    SLIST_ENTRY(OpaqueNAPIHandleScope) node; // size_t
    SLIST_HEAD(, Handle) handleList;         // size_t
};

// 1. external -> 不透明指针 + finalizer + 调用一个回调
// 2. Function -> JSValue data 数组 + finalizer
// 3. Constructor -> .[[prototype]] = new Object() + finalizer
// 4. ReferenceCount -> Symbol('referenceCount'): new Object() + finalizer + 调用多个回调

// Symbol('strictEqual'): (op1, op2) => op1 === op2;

// typedef struct
// {
//     const char *errorMessage; // size_t
//     NAPIStatus errorCode;     // enum，有需要的话根据 iOS 或 Android 的 clang 情况做单独处理
// } NAPIExtendedErrorInfo;
struct OpaqueNAPIEnv
{
    JSValue strictEqualSymbolValue;                      // 64/128
    JSValue referenceCountSymbolValue;                   // 64/128
    JSContext *context;                                  // size_t
    SLIST_HEAD(, OpaqueNAPIHandleScope) handleScopeList; // size_t
    NAPIExtendedErrorInfo lastError;
};

// static
NAPIStatus setLastErrorCode(NAPIEnv env, NAPIStatus errorCode)
{
    CHECK_ENV(env);

    env->lastError.errorCode = errorCode;
    // assert 可能导致静态分析出现假阳性
    //#ifndef NDEBUG
    //    if (errorCode == NAPIPendingException)
    //    {
    //        CHECK_ARG(env, env->context);
    //        JSValue exceptionValue = JS_GetException(env->context);
    //        if (JS_IsNull(exceptionValue))
    //        {
    //            assert(false);
    //        }
    //        else
    //        {
    //            JS_Throw(env->context, exceptionValue);
    //        }
    //    }
    //#endif

    return errorCode;
}

// static
NAPIStatus clearLastError(NAPIEnv env)
{
    // env 空指针检查
    CHECK_ENV(env);

    env->lastError.errorCode = NAPIOK;

    return NAPIOK;
}

// 这个函数不会修改引用计数和所有权
// NAPIHandleScopeMismatch/NAPIMemoryError
static NAPIStatus addValueToHandleScope(NAPIEnv env, JSValue value, struct Handle **result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env, !SLIST_EMPTY(&(*env).handleScopeList), NAPIHandleScopeMismatch);
    *result = malloc(sizeof(struct Handle));
    RETURN_STATUS_IF_FALSE(env, *result, NAPIMemoryError);
    (*result)->value = value;
    SLIST_INSERT_HEAD(&(SLIST_FIRST(&(env)->handleScopeList))->handleList, *result, node);

    return clearLastError(env);
}

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

// NAPIPendingException + addValueToHandleScope
NAPIStatus napi_get_global(NAPIEnv env, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    // JS_GetGlobalObject 返回已经引用计数 +1
    // 实际上 globalValue 可能为 JS_EXCEPTION
    JSValue globalValue = JS_GetGlobalObject(env->context);
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(globalValue), NAPIPendingException);
    struct Handle *globalHandle;
    NAPIStatus status = addValueToHandleScope(env, globalValue, &globalHandle);
    if (status != NAPIOK)
    {
        JS_FreeValue(env->context, globalValue);

        return status;
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
NAPIStatus napi_create_double(NAPIEnv env, double value, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->context);

    JSValue jsValue = JS_NewFloat64(env->context, value);
    struct Handle *handle;
    CHECK_NAPI(addValueToHandleScope(env, jsValue, &handle));
    *result = (NAPIValue)&handle->value;

    return clearLastError(env);
}

// + addValueToHandleScope
NAPIStatus napi_create_int32(NAPIEnv env, int32_t value, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->context);

    JSValue jsValue = JS_NewInt32(env->context, value);
    struct Handle *handle;
    CHECK_NAPI(addValueToHandleScope(env, jsValue, &handle));
    *result = (NAPIValue)&handle->value;

    return clearLastError(env);
}

// + addValueToHandleScope
NAPIStatus napi_create_uint32(NAPIEnv env, uint32_t value, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->context);

    // uint32_t 需要用 int64_t 承载
    JSValue jsValue = JS_NewInt64(env->context, value);
    struct Handle *handle;
    CHECK_NAPI(addValueToHandleScope(env, jsValue, &handle));
    *result = (NAPIValue)&handle->value;

    return clearLastError(env);
}

// + addValueToHandleScope
NAPIStatus napi_create_int64(NAPIEnv env, int64_t value, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->context);

    JSValue jsValue = JS_NewInt64(env->context, value);
    struct Handle *handle;
    CHECK_NAPI(addValueToHandleScope(env, jsValue, &handle));
    *result = (NAPIValue)&handle->value;

    return clearLastError(env);
}

// NAPIPendingException + addValueToHandleScope
NAPIStatus napi_create_string_utf8(NAPIEnv env, const char *str, size_t length, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

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
    NAPIStatus status = addValueToHandleScope(env, stringValue, &stringHandle);
    if (status != NAPIOK)
    {
        JS_FreeValue(env->context, stringValue);

        return status;
    }
    *result = (NAPIValue)&stringHandle->value;

    return clearLastError(env);
}

typedef struct
{
    NAPIEnv env; // size_t
    void *data;  // size_t
} BaseInfo;

typedef struct
{
    BaseInfo baseInfo;
    NAPICallback callback; // size_t
} FunctionInfo;

static JSClassID functionClassId;

struct OpaqueNAPICallbackInfo
{
    JSValueConst newTarget; // 128/64
    JSValueConst thisArg;   // 128/64
    JSValueConst *argv;     // 128/64
    void *data;             // size_t
    int argc;
};

static JSValue callAsFunction(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv, int magic,
                              JSValue *funcData)
{
    // 固定 1 参数
    FunctionInfo *functionInfo = JS_GetOpaque(funcData[0], functionClassId);
    if (!functionInfo || !functionInfo->callback || !functionInfo->baseInfo.env)
    {
        assert(false);

        return JS_UNDEFINED;
    }
    struct OpaqueNAPICallbackInfo callbackInfo = {JS_UNDEFINED, thisVal, argv, functionInfo->baseInfo.data, argc};
    NAPIHandleScope handleScope;
    NAPIStatus status = napi_open_handle_scope(functionInfo->baseInfo.env, &handleScope);
    if (status != NAPIOK)
    {
        assert(false);

        return JS_UNDEFINED;
    }
    JSValue returnValue =
        JS_DupValue(ctx, *((JSValue *)functionInfo->callback(functionInfo->baseInfo.env, &callbackInfo)));
    JSValue exceptionValue = JS_GetException(ctx);
    status = napi_close_handle_scope(functionInfo->baseInfo.env, handleScope);
    if (status != NAPIOK)
    {
        assert(false);
        JS_FreeValue(ctx, returnValue);

        return JS_UNDEFINED;
    }
    if (!JS_IsNull(exceptionValue))
    {
        JS_Throw(ctx, exceptionValue);
        JS_FreeValue(ctx, returnValue);

        return JS_EXCEPTION;
    }

    return returnValue;
}

// NAPIMemoryError/NAPIPendingException + addValueToHandleScope
NAPIStatus napi_create_function(NAPIEnv env, const char *utf8name, size_t length, NAPICallback cb, void *data,
                                NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);
    CHECK_ARG(env, cb);

    // malloc
    FunctionInfo *functionInfo = malloc(sizeof(FunctionInfo));
    RETURN_STATUS_IF_FALSE(env, functionInfo, NAPIMemoryError);
    functionInfo->baseInfo.env = env;
    functionInfo->baseInfo.data = data;
    functionInfo->callback = cb;

    // rc: 1
    JSValue fakePrototypeValue = JS_NewObjectClass(env->context, (int)functionClassId);
    if (JS_IsException(fakePrototypeValue))
    {
        free(functionInfo);

        return setLastErrorCode(env, NAPIPendingException);
    }
    // functionInfo 生命周期被 JSValue 托管
    JS_SetOpaque(fakePrototypeValue, functionInfo);

    // fakePrototypeValue 引用计数 +1 => rc: 2
    JSValue functionValue = JS_NewCFunctionData(env->context, callAsFunction, 0, 0, 1, &fakePrototypeValue);
    // 转移所有权
    JS_FreeValue(env->context, fakePrototypeValue);
    // 如果 functionValue 创建失败，fakePrototypeValue rc 不会被 +1，上面的引用计数 -1 => rc 为
    // 0，发生垃圾回收，FunctionInfo 结构体也被回收
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(functionValue), NAPIPendingException);
    struct Handle *functionHandle;
    NAPIStatus addStatus = addValueToHandleScope(env, functionValue, &functionHandle);
    if (addStatus != NAPIOK)
    {
        // 由于 fakePrototypeValue 所有权被 functionValue 持有，所以只需要对 functionValue 做引用计数 -1
        JS_FreeValue(env->context, functionValue);

        return addStatus;
    }
    *result = (NAPIValue)&functionHandle->value;

    return clearLastError(env);
}

static JSClassID externalClassId;

NAPIStatus napi_typeof(NAPIEnv env, NAPIValue value, NAPIValueType *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->context);

    JSValue jsValue = *((JSValue *)value);
    if (JS_IsUndefined(jsValue))
    {
        *result = NAPIUndefined;
    }
    else if (JS_IsNull(jsValue))
    {
        *result = NAPINull;
    }
    else if (JS_IsNumber(jsValue))
    {
        *result = NAPINumber;
    }
    else if (JS_IsBool(jsValue))
    {
        *result = NAPIBoolean;
    }
    else if (JS_IsString(jsValue))
    {
        *result = NAPIString;
    }
    else if (JS_IsFunction(env->context, jsValue))
    {
        *result = NAPIFunction;
    }
    else if (JS_GetOpaque(jsValue, externalClassId))
    {
        // JS_GetOpaque 会检查 classId
        *result = NAPIExternal;
    }
    else if (JS_IsObject(jsValue))
    {
        *result = NAPIObject;
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

// NAPINumberExpected
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

// NAPINumberExpected
NAPIStatus napi_get_value_uint32(NAPIEnv env, NAPIValue value, uint32_t *result)
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

// NAPINumberExpected
NAPIStatus napi_get_value_int64(NAPIEnv env, NAPIValue value, int64_t *result)
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

// NAPIBooleanExpected
NAPIStatus napi_get_value_bool(NAPIEnv env, NAPIValue value, bool *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    JSValue jsValue = *((JSValue *)value);
    RETURN_STATUS_IF_FALSE(env, JS_IsBool(jsValue), NAPIBooleanExpected);
    *result = JS_VALUE_GET_BOOL(jsValue);

    return clearLastError(env);
}

static bool getUTF8CharacterBytesLength(uint8_t codePoint, uint8_t *bytesToWrite)
{
    if (!bytesToWrite)
    {
        return false;
    }
    if (codePoint >> 7 == 0)
    {
        // 0xxxxxxx
        *bytesToWrite = 1;
    }
    else if (codePoint >> 5 == 6)
    {
        // 110xxxxx
        *bytesToWrite = 2;
    }
    else if (codePoint >> 4 == 0xe)
    {
        // 1110xxxx
        *bytesToWrite = 3;
    }
    else if (codePoint >> 3 == 0x1e)
    {
        // 11110xxx
        *bytesToWrite = 4;
    }
    else if (codePoint >> 2 == 0x3e)
    {
        // 111110xx
        *bytesToWrite = 5;
    }
    else if (codePoint >> 1 == 0x7e)
    {
        // 1111110x
        *bytesToWrite = 6;
    }
    else
    {
        return false;
    }

    return true;
}

// NAPIStringExpected/NAPIPendingException/NAPIGenericFailure
NAPIStatus napi_get_value_string_utf8(NAPIEnv env, NAPIValue value, char *buf, size_t bufsize, size_t *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);

    RETURN_STATUS_IF_FALSE(env, JS_IsString(*((JSValue *)value)), NAPIStringExpected);
    if (buf && !bufsize)
    {
        if (result)
        {
            *result = 0;
        }

        return clearLastError(env);
    }
    if (bufsize == 1)
    {
        buf[0] = '\0';

        return clearLastError(env);
    }
    size_t length;
    const char *cString = JS_ToCStringLen(env->context, &length, *((JSValue *)value));
    RETURN_STATUS_IF_FALSE(env, cString || length, NAPIPendingException);
    if (!buf)
    {
        JS_FreeCString(env->context, cString);
        CHECK_ARG(env, result);
        *result = length;

        return clearLastError(env);
    }
    if (length <= bufsize - 1)
    {
        memmove(buf, cString, length);
        buf[length] = '\0';
        JS_FreeCString(env->context, cString);

        return clearLastError(env);
    }
    // 截断
    memmove(buf, cString, bufsize - 1);
    // bufsize >= 2
    size_t initialIndex = bufsize - 2;
    size_t index = initialIndex;
    uint8_t bytesToWrite = 0;
    while (index >= 0)
    {
        uint8_t codePoint = cString[index];
        if (codePoint >> 6 == 2)
        {
            // 10xxxxxx
            if (index != 0)
            {
                --index;
            }
            else
            {
                JS_FreeCString(env->context, cString);

                return setLastErrorCode(env, NAPIGenericFailure);
            }
        }
        else if (codePoint >> 7 == 0 || codePoint >> 6 == 3)
        {
            // 0xxxxxxx 11xxxxxx
            if (!getUTF8CharacterBytesLength(codePoint, &bytesToWrite))
            {
                JS_FreeCString(env->context, cString);

                return setLastErrorCode(env, NAPIGenericFailure);
            }
            break;
        }
        else
        {
            JS_FreeCString(env->context, cString);

            return setLastErrorCode(env, NAPIGenericFailure);
        }
    }
    // getUTF8CharacterBytesLength 返回 bytesToWrite >= 1
    if (index + bytesToWrite - 1 < initialIndex)
    {
        JS_FreeCString(env->context, cString);

        return setLastErrorCode(env, NAPIGenericFailure);
    }
    else if (index + bytesToWrite - 1 > initialIndex)
    {
        buf[index] = '\0';
    }
    else
    {
        buf[bufsize - 1] = '\0';
    }

    JS_FreeCString(env->context, cString);

    return clearLastError(env);
}

// NAPIPendingException + addValueToHandleScope
NAPIStatus napi_coerce_to_bool(NAPIEnv env, NAPIValue value, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    int boolStatus = JS_ToBool(env->context, *((JSValue *)value));
    RETURN_STATUS_IF_FALSE(env, boolStatus != -1, NAPIPendingException);
    JSValue boolValue = JS_NewBool(env->context, boolStatus);
    struct Handle *boolHandle;
    CHECK_NAPI(addValueToHandleScope(env, boolValue, &boolHandle));
    *result = (NAPIValue)&boolHandle->value;

    return clearLastError(env);
}

// NAPIPendingException + addValueToHandleScope
NAPIStatus napi_coerce_to_number(NAPIEnv env, NAPIValue value, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    double doubleValue;
    // JS_ToFloat64 只有 -1 0 两种
    int floatStatus = JS_ToFloat64(env->context, &doubleValue, *((JSValue *)value));
    RETURN_STATUS_IF_FALSE(env, floatStatus != -1, NAPIPendingException);
    JSValue floatValue = JS_NewFloat64(env->context, doubleValue);
    struct Handle *floatHandle;
    CHECK_NAPI(addValueToHandleScope(env, floatValue, &floatHandle));
    *result = (NAPIValue)&floatHandle->value;

    return clearLastError(env);
}

// NAPIPendingException + addValueToHandleScope
NAPIStatus napi_coerce_to_string(NAPIEnv env, NAPIValue value, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    JSValue stringValue = JS_ToString(env->context, *((JSValue *)value));
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(stringValue), NAPIPendingException);
    struct Handle *stringHandle;
    NAPIStatus status = addValueToHandleScope(env, stringValue, &stringHandle);
    if (status != NAPIOK)
    {
        JS_FreeValue(env->context, stringValue);

        return status;
    }
    *result = (NAPIValue)&stringHandle->value;

    return clearLastError(env);
}

// NAPIPendingException/NAPIGenericFailure
NAPIStatus napi_set_property(NAPIEnv env, NAPIValue object, NAPIValue key, NAPIValue value)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, object);
    CHECK_ARG(env, key);
    CHECK_ARG(env, value);

    JSAtom atom = JS_ValueToAtom(env->context, *((JSValue *)key));
    RETURN_STATUS_IF_FALSE(env, atom != JS_ATOM_NULL, NAPIPendingException);
    int status =
        JS_SetProperty(env->context, *((JSValue *)object), atom, JS_DupValue(env->context, *((JSValue *)value)));
    JS_FreeAtom(env->context, atom);
    RETURN_STATUS_IF_FALSE(env, status != -1, NAPIPendingException);
    RETURN_STATUS_IF_FALSE(env, status, NAPIGenericFailure);

    return clearLastError(env);
}

// NAPIPendingException
NAPIStatus napi_has_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, object);
    CHECK_ARG(env, key);
    CHECK_ARG(env, result);

    JSAtom atom = JS_ValueToAtom(env->context, *((JSValue *)key));
    RETURN_STATUS_IF_FALSE(env, atom != JS_ATOM_NULL, NAPIPendingException);
    int status = JS_HasProperty(env->context, *((JSValue *)object), atom);
    JS_FreeAtom(env->context, atom);
    RETURN_STATUS_IF_FALSE(env, status != -1, NAPIPendingException);
    *result = status;

    return clearLastError(env);
}

// NAPIPendingException + addValueToHandleScope
NAPIStatus napi_get_property(NAPIEnv env, NAPIValue object, NAPIValue key, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, object);
    CHECK_ARG(env, key);
    CHECK_ARG(env, result);

    JSAtom atom = JS_ValueToAtom(env->context, *((JSValue *)key));
    RETURN_STATUS_IF_FALSE(env, atom != JS_ATOM_NULL, NAPIPendingException);
    JSValue value = JS_GetProperty(env->context, *((JSValue *)object), atom);
    JS_FreeAtom(env->context, atom);
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(value), NAPIPendingException);
    struct Handle *handle;
    NAPIStatus status = addValueToHandleScope(env, value, &handle);
    if (status != NAPIOK)
    {
        JS_FreeValue(env->context, value);

        return status;
    }
    *result = (NAPIValue)&handle->value;

    return clearLastError(env);
}

// NAPIPendingException
NAPIStatus napi_delete_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, object);
    CHECK_ARG(env, key);

    JSAtom atom = JS_ValueToAtom(env->context, *((JSValue *)key));
    RETURN_STATUS_IF_FALSE(env, atom != JS_ATOM_NULL, NAPIPendingException);
    // 不抛出异常
    int status = JS_DeleteProperty(env->context, *((JSValue *)object), atom, 0);
    JS_FreeAtom(env->context, atom);
    RETURN_STATUS_IF_FALSE(env, status != -1, NAPIPendingException);
    if (result)
    {
        *result = status;
    }

    return clearLastError(env);
}

// NAPIGenericFailure + napi_get_global + napi_get_property + napi_typeof + napi_get_undefined + napi_call_function +
// napi_get_value_bool
NAPIStatus napi_strict_equals(NAPIEnv env, NAPIValue lhs, NAPIValue rhs, bool *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, lhs);
    CHECK_ARG(env, rhs);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env, JS_IsSymbol(env->strictEqualSymbolValue), NAPIGenericFailure);

    NAPIValue globalValue, undefinedValue, strictEqualValue, isStrictEqualValue;
    CHECK_NAPI(napi_get_global(env, &globalValue));
    CHECK_NAPI(napi_get_property(env, globalValue, (NAPIValue)&env->strictEqualSymbolValue, &strictEqualValue));
    NAPIValueType valueType;
    CHECK_NAPI(napi_typeof(env, strictEqualValue, &valueType));
    RETURN_STATUS_IF_FALSE(env, valueType == NAPIFunction, NAPIGenericFailure);
    // 这里已经拿到 globalValue，所以不需要 JS undefined
    NAPIValue argv[] = {lhs, rhs};
    CHECK_NAPI(napi_get_undefined(env, &undefinedValue));
    CHECK_NAPI(napi_call_function(env, undefinedValue, strictEqualValue, 2, argv, &isStrictEqualValue));
    CHECK_NAPI(napi_typeof(env, isStrictEqualValue, &valueType));
    RETURN_STATUS_IF_FALSE(env, valueType == NAPIBoolean, NAPIGenericFailure);
    CHECK_NAPI(napi_get_value_bool(env, isStrictEqualValue, result));

    return clearLastError(env);
}

// NAPIMemoryError/NAPIPendingException + addValueToHandleScope
NAPIStatus napi_call_function(NAPIEnv env, NAPIValue thisValue, NAPIValue func, size_t argc, const NAPIValue *argv,
                              NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, thisValue);
    CHECK_ARG(env, func);

    JSValue *internalArgv = NULL;
    if (argc > 0)
    {
        CHECK_ARG(env, argv);
        internalArgv = malloc(sizeof(JSValue) * argc);
        RETURN_STATUS_IF_FALSE(env, internalArgv, NAPIMemoryError);
        for (size_t i = 0; i < argc; ++i)
        {
            internalArgv[i] = *((JSValue *)argv[i]);
        }
    }
    JSValue returnValue = JS_Call(env->context, *((JSValue *)func), *((JSValue *)thisValue), (int)argc, internalArgv);
    free(internalArgv);
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(returnValue), NAPIPendingException);
    struct Handle *handle;
    NAPIStatus status = addValueToHandleScope(env, returnValue, &handle);
    if (status != NAPIOK)
    {
        JS_FreeValue(env->context, returnValue);

        return status;
    }
    if (result)
    {
        *result = (NAPIValue)&handle->value;
    }

    return clearLastError(env);
}

// NAPIPendingException/NAPIMemoryError
NAPIStatus napi_new_instance(NAPIEnv env, NAPIValue constructor, size_t argc, const NAPIValue *argv, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, constructor);
    CHECK_ARG(env, result);
    JSValue *internalArgv = NULL;
    if (argc > 0)
    {
        CHECK_ARG(env, argv);
        internalArgv = malloc(sizeof(JSValue) * argc);
        RETURN_STATUS_IF_FALSE(env, internalArgv, NAPIMemoryError);
        for (size_t i = 0; i < argc; ++i)
        {
            internalArgv[i] = *((JSValue *)argv[i]);
        }
    }

    JSValue returnValue = JS_CallConstructor(env->context, *((JSValue *)constructor), (int)argc, internalArgv);
    free(internalArgv);
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(returnValue), NAPIPendingException);
    struct Handle *handle;
    NAPIStatus status = addValueToHandleScope(env, returnValue, &handle);
    if (status != NAPIOK)
    {
        JS_FreeValue(env->context, returnValue);

        return status;
    }
    *result = (NAPIValue)&handle->value;

    return clearLastError(env);
}

// NAPIPendingException
NAPIStatus napi_instanceof(NAPIEnv env, NAPIValue object, NAPIValue constructor, bool *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, object);
    CHECK_ARG(env, constructor);
    CHECK_ARG(env, result);

    int status = JS_IsInstanceOf(env->context, *((JSValue *)object), *((JSValue *)constructor));
    RETURN_STATUS_IF_FALSE(env, status != -1, NAPIPendingException);
    *result = status;

    return clearLastError(env);
}

// napi_get_undefined
NAPIStatus napi_get_cb_info(NAPIEnv env, NAPICallbackInfo callbackInfo, size_t *argc, NAPIValue *argv,
                            NAPIValue *thisArg, void **data)
{
    CHECK_ENV(env);
    CHECK_ARG(env, callbackInfo);

    if (argv)
    {
        CHECK_ARG(env, argc);
        size_t i = 0;
        size_t min = *argc > callbackInfo->argc ? callbackInfo->argc : *argc;
        for (; i < min; ++i)
        {
            argv[i] = (NAPIValue)&callbackInfo->argv[i];
        }
        if (i < *argc)
        {
            NAPIValue undefinedValue;
            CHECK_NAPI(napi_get_undefined(env, &undefinedValue));
            for (; i < *argc; ++i)
            {
                argv[i] = undefinedValue;
            }
        }
    }
    if (argc)
    {
        *argc = callbackInfo->argc;
    }
    if (thisArg)
    {
        *thisArg = (NAPIValue)&callbackInfo->thisArg;
    }
    if (data)
    {
        *data = callbackInfo->data;
    }

    return clearLastError(env);
}

NAPIStatus napi_get_new_target(NAPIEnv env, NAPICallbackInfo callbackInfo, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, callbackInfo);
    CHECK_ARG(env, result);

    *result = (NAPIValue)&callbackInfo->newTarget;

    return clearLastError(env);
}

typedef struct
{
    BaseInfo baseInfo;
    void *finalizeHint;            // size_t
    NAPIFinalize finalizeCallback; // size_t
} ExternalInfo;

// NAPIMemoryError/NAPIPendingException + addValueToHandleScope
NAPIStatus napi_create_external(NAPIEnv env, void *data, NAPIFinalize finalizeCB, void *finalizeHint, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    ExternalInfo *externalInfo = malloc(sizeof(ExternalInfo));
    RETURN_STATUS_IF_FALSE(env, externalInfo, NAPIMemoryError);
    externalInfo->baseInfo.env = env;
    externalInfo->baseInfo.data = data;
    externalInfo->finalizeCallback = finalizeCB;
    externalInfo->finalizeHint = finalizeHint;
    JSValue object = JS_NewObjectClass(env->context, (int)externalClassId);
    if (JS_IsException(object))
    {
        free(externalInfo);

        return setLastErrorCode(env, NAPIPendingException);
    }
    JS_SetOpaque(object, externalInfo);
    struct Handle *handle;
    NAPIStatus status = addValueToHandleScope(env, object, &handle);
    if (status != NAPIOK)
    {
        JS_FreeValue(env->context, object);

        return status;
    }
    *result = (NAPIValue)&handle->value;

    return clearLastError(env);
}

NAPIStatus napi_get_value_external(NAPIEnv env, NAPIValue value, void **result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    ExternalInfo *externalInfo = JS_GetOpaque(*((JSValue *)value), externalClassId);
    *result = externalInfo ? externalInfo->baseInfo.data : NULL;

    return clearLastError(env);
}

// NAPIMemoryError
NAPIStatus napi_open_handle_scope(NAPIEnv env, NAPIHandleScope *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = malloc(sizeof(struct OpaqueNAPIHandleScope));
    RETURN_STATUS_IF_FALSE(env, *result, NAPIMemoryError);
    SLIST_INIT(&(*result)->handleList);
    SLIST_INSERT_HEAD(&env->handleScopeList, *result, node);

    return clearLastError(env);
}

// NAPIHandleScopeMismatch
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

struct OpaqueNAPIEscapableHandleScope
{
    struct OpaqueNAPIHandleScope handleScope;
    bool escapeCalled;
};

// NAPIMemoryError
NAPIStatus napi_open_escapable_handle_scope(NAPIEnv env, NAPIEscapableHandleScope *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    *result = malloc(sizeof(struct OpaqueNAPIEscapableHandleScope));
    RETURN_STATUS_IF_FALSE(env, *result, NAPIMemoryError);
    (*result)->escapeCalled = false;
    SLIST_INIT(&(*result)->handleScope.handleList);
    SLIST_INSERT_HEAD(&env->handleScopeList, &((*result)->handleScope), node);

    return clearLastError(env);
}

// NAPIHandleScopeMismatch
NAPIStatus napi_close_escapable_handle_scope(NAPIEnv env, NAPIEscapableHandleScope scope)
{
    CHECK_ENV(env);
    CHECK_ARG(env, scope);

    // JS_FreeValue 不能传入 NULL
    CHECK_ARG(env, env->context);

    RETURN_STATUS_IF_FALSE(env, SLIST_FIRST(&env->handleScopeList) == &scope->handleScope, NAPIHandleScopeMismatch);
    struct Handle *handle, *tempHandle;
    SLIST_FOREACH_SAFE(handle, &(&scope->handleScope)->handleList, node, tempHandle)
    {
        JS_FreeValue(env->context, handle->value);
        free(handle);
    }
    SLIST_REMOVE_HEAD(&env->handleScopeList, node);
    free(scope);

    return clearLastError(env);
}

// NAPIMemoryError/NAPIEscapeCalledTwice/NAPIHandleScopeMismatch
NAPIStatus napi_escape_handle(NAPIEnv env, NAPIEscapableHandleScope scope, NAPIValue escapee, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, scope);
    CHECK_ARG(env, escapee);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env, !scope->escapeCalled, NAPIEscapeCalledTwice);

    NAPIHandleScope handleScope = SLIST_NEXT(&scope->handleScope, node);
    RETURN_STATUS_IF_FALSE(env, handleScope, NAPIHandleScopeMismatch);
    struct Handle *handle = malloc(sizeof(struct Handle));
    RETURN_STATUS_IF_FALSE(env, *result, NAPIMemoryError);
    scope->escapeCalled = true;
    handle->value = *((JSValue *)escapee);
    SLIST_INSERT_HEAD(&handleScope->handleList, handle, node);
    *result = (NAPIValue)&handle->value;

    return clearLastError(env);
}

NAPIStatus napi_throw(NAPIEnv env, NAPIValue error)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, error);

    JS_Throw(env->context, *((JSValue *)error));

    return clearLastError(env);
}

// + addValueToHandleScope
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

        return status;
    }
    *result = (NAPIValue)&exceptionHandle->value;

    return clearLastError(env);
}

// NAPIPendingException + addValueToHandleScope
NAPIStatus NAPIRunScript(NAPIEnv env, const char *script, const char *sourceUrl, NAPIValue *result)
{
    NAPI_PREAMBLE(env);

    // script 传入 NULL 会崩
    // utf8SourceUrl 传入 NULL 会导致崩溃，JS_NewAtom 调用 strlen 不能传入 NULL
    if (!sourceUrl)
    {
        sourceUrl = "";
    }
    JSValue returnValue = JS_Eval(env->context, script, script ? strlen(script) : 0, sourceUrl, JS_EVAL_TYPE_GLOBAL);
    for (int err = 1; err > 0;)
    {
        JSContext *context;
        err = JS_ExecutePendingJob(JS_GetRuntime(env->context), &context);
        if (err == -1)
        {
            // 正常情况下 JS_ExecutePendingJob 返回 -1
            // 代表引擎内部异常，比如内存分配失败等，未来如果真产生了 JS
            // 异常，也直接吃掉
            JSValue inlineExceptionValue = JS_GetException(context);
            JS_FreeValue(context, inlineExceptionValue);
        }
    }
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(returnValue), NAPIPendingException);
    struct Handle *returnHandle;
    NAPIStatus status = addValueToHandleScope(env, returnValue, &returnHandle);
    if (status != NAPIOK)
    {
        JS_FreeValue(env->context, returnValue);

        return status;
    }
    *result = (NAPIValue)&returnHandle->value;

    return clearLastError(env);
}

static uint8_t contextCount = 0;

static void functionFinalizer(JSRuntime *rt, JSValue val)
{
    FunctionInfo *functionInfo = JS_GetOpaque(val, functionClassId);
    free(functionInfo);
}

static void externalFinalizer(JSRuntime *rt, JSValue val)
{
    ExternalInfo *externalInfo = JS_GetOpaque(val, functionClassId);
    if (externalInfo && externalInfo->finalizeCallback)
    {
        externalInfo->finalizeCallback(externalInfo->baseInfo.env, externalInfo->baseInfo.data,
                                       externalInfo->finalizeHint);
    }
    free(externalInfo);
}

static JSRuntime *runtime = NULL;

typedef struct
{
    FunctionInfo functionInfo;
    JSClassID classId; // uint32_t
} ConstructorInfo;

static JSClassID constructorClassId;

static JSValue callAsConstructor(JSContext *ctx, JSValueConst newTarget, int argc, JSValueConst *argv)
{
    JSValue prototypeValue = JS_GetPrototype(ctx, newTarget);
    if (JS_IsException(prototypeValue))
    {
        return prototypeValue;
    }
    ConstructorInfo *constructorInfo = JS_GetOpaque(prototypeValue, constructorClassId);
    JS_FreeValue(ctx, prototypeValue);
    if (!constructorInfo || !constructorInfo->functionInfo.baseInfo.env || !constructorInfo->functionInfo.callback)
    {
        assert(false);

        return JS_UNDEFINED;
    }
    JSValue thisValue = JS_NewObjectClass(ctx, (int)constructorInfo->classId);
    if (JS_IsException(thisValue))
    {
        return thisValue;
    }
    struct OpaqueNAPICallbackInfo callbackInfo = {newTarget, thisValue, argv,
                                                  constructorInfo->functionInfo.baseInfo.data, argc};
    NAPIHandleScope handleScope;
    NAPIStatus status = napi_open_handle_scope(constructorInfo->functionInfo.baseInfo.env, &handleScope);
    if (status != NAPIOK)
    {
        assert(false);
        JS_FreeValue(ctx, thisValue);

        return JS_UNDEFINED;
    }
    JSValue returnValue = JS_DupValue(ctx, *((JSValue *)constructorInfo->functionInfo.callback(
                                               constructorInfo->functionInfo.baseInfo.env, &callbackInfo)));
    JS_FreeValue(ctx, thisValue);
    JSValue exceptionValue = JS_GetException(ctx);
    status = napi_close_handle_scope(constructorInfo->functionInfo.baseInfo.env, handleScope);
    if (status != NAPIOK)
    {
        assert(false);
        JS_FreeValue(ctx, returnValue);

        return JS_UNDEFINED;
    }
    if (!JS_IsNull(exceptionValue))
    {
        JS_Throw(ctx, exceptionValue);
        JS_FreeValue(ctx, returnValue);

        return JS_EXCEPTION;
    }

    return returnValue;
}

// NAPIMemoryError/NAPIPendingException + addValueToHandleScope
NAPIStatus NAPIDefineClass(NAPIEnv env, const char *utf8name, size_t length, NAPICallback constructor, void *data,
                           NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, constructor);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env, length == NAPI_AUTO_LENGTH, NAPIInvalidArg);
    RETURN_STATUS_IF_FALSE(env, runtime, NAPIInvalidArg);

    ConstructorInfo *constructorInfo = malloc(sizeof(ConstructorInfo));
    RETURN_STATUS_IF_FALSE(env, constructorInfo, NAPIMemoryError);
    constructorInfo->functionInfo.baseInfo.env = env;
    constructorInfo->functionInfo.baseInfo.data = data;
    constructorInfo->functionInfo.callback = constructor;
    JS_NewClassID(&constructorInfo->classId);
    JSClassDef classDef = {
        utf8name ?: "",
    };
    int status = JS_NewClass(runtime, constructorInfo->classId, &classDef);
    if (status == -1)
    {
        free(constructorInfo);

        return setLastErrorCode(env, NAPIPendingException);
    }
    JSValue prototype = JS_NewObjectClass(env->context, (int)constructorClassId);
    if (JS_IsException(prototype))
    {
        free(constructorInfo);

        return setLastErrorCode(env, NAPIPendingException);
    }
    JS_SetOpaque(prototype, constructorInfo);
    // JS_NewCFunction2 会正确处理 utf8name 为空情况
    JSValue constructorValue = JS_NewCFunction2(env->context, callAsConstructor, utf8name, 0, JS_CFUNC_constructor, 0);
    if (JS_IsException(constructorValue))
    {
        // prototype 会自己 free ConstructorInfo 结构体
        JS_FreeValue(env->context, prototype);

        return setLastErrorCode(env, NAPIPendingException);
    }
    struct Handle *handle;
    NAPIStatus addStatus = addValueToHandleScope(env, constructorValue, &handle);
    if (addStatus != NAPIOK)
    {
        JS_FreeValue(env->context, constructorValue);
        JS_FreeValue(env->context, prototype);

        return status;
    }
    *result = (NAPIValue)&handle->value;
    // .prototype .constructor
    // 会自动引用计数 +1
    JS_SetConstructor(env->context, constructorValue, prototype);
    // context -> class_proto
    // 转移所有权
    JS_SetClassProto(env->context, constructorInfo->classId, prototype);

    return clearLastError(env);
}

// NAPIGenericFailure/NAPIMemoryError
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
        // 一定成功
        JS_NewClassID(&constructorClassId);
        JS_NewClassID(&functionClassId);
        JS_NewClassID(&externalClassId);

        JSClassDef classDef = {"External", externalFinalizer};
        int status = JS_NewClass(runtime, externalClassId, &classDef);
        if (status == -1)
        {
            JS_FreeRuntime(runtime);
            free(*env);
            runtime = NULL;

            return NAPIMemoryError;
        }

        classDef.class_name = "ExternalFunction";
        classDef.finalizer = functionFinalizer;
        status = JS_NewClass(runtime, functionClassId, &classDef);
        if (status == -1)
        {
            JS_FreeRuntime(runtime);
            free(*env);
            runtime = NULL;

            return NAPIMemoryError;
        }

        classDef.class_name = "ExternalConstructor";
        classDef.finalizer = functionFinalizer;
        status = JS_NewClass(runtime, constructorClassId, &classDef);
        if (status == -1)
        {
            JS_FreeRuntime(runtime);
            free(*env);
            runtime = NULL;

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
    JSValue prototype = JS_NewObject(context);
    if (JS_IsException(prototype))
    {
        JS_FreeContext(context);
        JS_FreeRuntime(runtime);
        free(*env);
        runtime = NULL;

        return NAPIGenericFailure;
    }
    JS_SetClassProto(context, externalClassId, prototype);
    prototype = JS_NewObject(context);
    if (JS_IsException(prototype))
    {
        JS_FreeContext(context);
        JS_FreeRuntime(runtime);
        free(*env);
        runtime = NULL;

        return NAPIGenericFailure;
    }
    JS_SetClassProto(context, constructorClassId, prototype);
    const char *string = "(function () { const symbol = Symbol(\"strictEqual\"); globalThis[symbol] = (op1, op2) => "
                         "op1 === op2; return symbol; })();";
    JSValue returnValue =
        JS_Eval(context, string, strlen(string), "https://napi.com/qjs_strict_equal.js", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(returnValue))
    {
        JS_FreeContext(context);
        JS_FreeRuntime(runtime);
        free(*env);
        runtime = NULL;

        return NAPIGenericFailure;
    }
    string = "(function () { return Symbol(\"referenceCount\"); })();";
    returnValue =
        JS_Eval(context, string, strlen(string), "https://napi.com/qjs_reference_count_symbol.js", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(returnValue))
    {
        JS_FreeContext(context);
        JS_FreeRuntime(runtime);
        free(*env);
        runtime = NULL;

        return NAPIGenericFailure;
    }
    contextCount += 1;
    (*env)->context = context;
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
    JS_FreeValue(env->context, env->strictEqualSymbolValue);
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