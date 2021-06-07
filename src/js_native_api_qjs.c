// 所有函数都可能抛出 NAPIInvalidArg
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
    JSValue value;            // 64/128
    SLIST_ENTRY(Handle) node; // size_t
};

struct OpaqueNAPIHandleScope
{
    SLIST_ENTRY(OpaqueNAPIHandleScope) node; // size_t
    SLIST_HEAD(, Handle) handleList;         // size_t
};

// typedef struct
//{
//     const char *errorMessage; // size_t
//     void *engineReserved; // size_t
//     uint32_t engineErrorCode; // 32
//     NAPIStatus errorCode; // enum，有需要的话根据 iOS 或 Android 的 clang 情况做单独处理
// } NAPIExtendedErrorInfo;
struct OpaqueNAPIEnv
{
    JSValue strictEqualSymbolValue;                      // 64/128
    JSContext *context;                                  // size_t
    SLIST_HEAD(, OpaqueNAPIHandleScope) handleScopeList; // size_t
    NAPIExtendedErrorInfo lastError;
};

// static inline
NAPIStatus setLastErrorCode(NAPIEnv env, NAPIStatus errorCode)
{
    CHECK_ENV(env);

    env->lastError.errorCode = errorCode;
    env->lastError.engineReserved = NULL;
    env->lastError.engineErrorCode = 0;
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

// NAPIGenericFailure + addValueToHandleScope
NAPIStatus napi_get_global(NAPIEnv env, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    // JS_GetGlobalObject 不能传入 NULL
    CHECK_ARG(env, env->context);

    // JS_GetGlobalObject 返回已经引用计数 +1
    // 实际上 globalValue 可能为 JS_EXCEPTION
    JSValue globalValue = JS_GetGlobalObject(env->context);
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(globalValue), NAPIGenericFailure);
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

// NAPIPendingException + addValueToHandleScope
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

// NAPIPendingException + addValueToHandleScope
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
    CHECK_NAPI(addValueToHandleScope(env, jsValue, &handle));
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
    CHECK_NAPI(addValueToHandleScope(env, jsValue, &handle));
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

        return setLastErrorCode(env, status);
    }
    *result = (NAPIValue)&stringHandle->value;

    return clearLastError(env);
}

typedef enum
{
    ConversionOK,    /* conversion successful */
    SourceExhausted, /* partial character in source, but hit end */
    TargetExhausted, /* inSufficent. room in target for conversion */
    SourceIllegal    /* source sequence is illegal/malformed */
} ConversionResult;

static const uint32_t UNI_SUR_HIGH_START = 0xd800;

static const uint32_t UNI_SUR_HIGH_END = 0xdbff;

static const uint32_t UNI_SUR_LOW_START = 0xdc00;

static const uint32_t UNI_SUR_LOW_END = 0xdfff;

static const uint8_t halfShift = 10;

static const uint32_t halfBase = 0x10000;

static const uint32_t UNI_REPLACEMENT_CHAR = 0x0000fffd;

/*
 * Once the bits are split out into bytes of UTF-8, this is a mask OR-ed
 * into the first byte, depending on how many bytes follow.  There are
 * as many entries in this table as there are UTF-8 sequence types.
 * (I.e., one byte sequence, two byte... etc.). Remember that sequencs
 * for *legal* UTF-8 will be 4 or fewer bytes total.
 */
static const uint8_t firstByteMark[7] = {0x0, 0x0, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc};

static ConversionResult convertUTF16toUTF8(const uint16_t *sourceStart, const uint16_t *sourceEnd,
                                           uint8_t **targetStart, const uint8_t *targetEnd)
{
    ConversionResult result = ConversionOK;
    uint8_t *target = *targetStart;
    while (sourceStart <= sourceEnd)
    {
        uint32_t unicodeValue;
        // 无效为 0xfffd，3 字节
        uint8_t bytesToWrite = 3;
        const uint32_t byteMask = 0xBF;
        const uint32_t byteMark = 0x80;
        unicodeValue = *sourceStart++;
        if (unicodeValue >= UNI_SUR_HIGH_START && unicodeValue <= UNI_SUR_HIGH_END)
        {
            // 代理平面
            if (sourceStart < sourceEnd)
            {
                uint16_t tail = *sourceStart;
                if (tail >= UNI_SUR_LOW_START && tail <= UNI_SUR_LOW_END)
                {
                    unicodeValue =
                        ((unicodeValue - UNI_SUR_HIGH_START) << halfShift) + (tail - UNI_SUR_LOW_START) + halfBase;
                    ++sourceStart;
                }
                else
                {
                    result = SourceIllegal;

                    break;
                }
            }
            else
            {
                result = SourceExhausted;

                break;
            }
        }
        else
        {
            /* UTF-16 surrogate values are illegal in UTF-32 */
            if (unicodeValue >= UNI_SUR_LOW_START && unicodeValue <= UNI_SUR_LOW_END)
            {
                result = SourceIllegal;

                break;
            }
        }
        /* Figure out how many bytes the result will require */
        if (unicodeValue < (uint32_t)0x80)
        {
            bytesToWrite = 1;
        }
        else if (unicodeValue < (uint32_t)0x800)
        {
            bytesToWrite = 2;
        }
        else if (unicodeValue < (uint32_t)0x10000)
        {
            bytesToWrite = 3;
        }
        else if (unicodeValue < (uint32_t)0x110000)
        {
            bytesToWrite = 4;
        }
        else
        {
            //            bytesToWrite = 3;
            unicodeValue = UNI_REPLACEMENT_CHAR;
        }

        if (target + bytesToWrite - 1 > targetEnd)
        {
            //            target -= bytesToWrite - 1;
            result = TargetExhausted;

            break;
        }
        // 从后往前写
        // offset 为 3，2，1，0
        // 但是 0 不考虑在循环做处理
        for (uint8_t offset = bytesToWrite - 1; offset > 0; --offset)
        {
            *(target + offset) = (uint8_t)((unicodeValue | byteMark) & byteMask);
        }
        *target = (uint8_t)(unicodeValue | firstByteMark[bytesToWrite]);
        target += bytesToWrite;
    }
    *targetStart = target;

    return result;
}

// V8 引擎传入 NULL 返回 ""
// NAPIPendingException/NAPIMemoryError/NAPIGenericFailure + addValueToHandleScope
NAPIStatus napi_create_string_utf16(NAPIEnv env, const char16_t *str, size_t length, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    if (length == NAPI_AUTO_LENGTH)
    {
        if (str)
        {
            length = strlen((const char *)str);
            RETURN_STATUS_IF_FALSE(env, !(length % 2), NAPIInvalidArg);
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
        utf8String = malloc(sizeof(uint8_t) * 4 * length);
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
        uint8_t *destination = utf8String;
        ConversionResult conversionResult =
            convertUTF16toUTF8(str, str + length - 1, &destination, destination + length - 1);
        if (*((uint8_t *)&value) == 1)
        {
            // 大端
            free((void *)str);
        }
        if (conversionResult == SourceIllegal || conversionResult == SourceExhausted)
        {
            free(utf8String);

            return setLastErrorCode(env, NAPIGenericFailure);
        }
        // 部分转换也算转换成功
        utf8StringLength = destination - utf8String;
    }
    JSValue stringValue = JS_NewStringLen(env->context, (const char *)utf8String, utf8StringLength);
    free(utf8String);
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(stringValue), NAPIPendingException);
    struct Handle *stringHandle;
    NAPIStatus status = addValueToHandleScope(env, stringValue, &stringHandle);
    if (status != NAPIOK)
    {
        JS_FreeValue(env->context, stringValue);

        return setLastErrorCode(env, status);
    }
    *result = (NAPIValue)&stringHandle->value;

    return clearLastError(env);
}

// napi_get_global + napi_get_named_property + napi_call_function
NAPIStatus napi_create_symbol(NAPIEnv env, NAPIValue description, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    NAPIValue global, symbolFunc;
    CHECK_NAPI(napi_get_global(env, &global));
    CHECK_NAPI(napi_get_named_property(env, global, "Symbol", &symbolFunc));
    CHECK_NAPI(napi_call_function(env, global, symbolFunc, 1, &description, result));

    return clearLastError(env);
}

typedef struct
{
    NAPIEnv env;
    void *data;
    NAPICallback callback;
} FunctionInfo;

static JSClassID functionClassId;

struct OpaqueNAPICallbackInfo
{
    JSValue newTarget; // 128/64
    JSValue thisArg;   // 128/64
    JSValue *argv;     // 128/64
    void *data;        // 64/32
    int argc;
};

static JSValue callAsFunction(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic,
                              JSValue *func_data)
{
    FunctionInfo *functionInfo = JS_GetOpaque(func_data[0], functionClassId);
    if (!functionInfo || !functionInfo->env || !functionInfo->callback)
    {
        assert(false);

        return JS_UNDEFINED;
    }
    clearLastError(functionInfo->env);
    struct OpaqueNAPICallbackInfo callbackInfo;
    callbackInfo.newTarget = JS_UNDEFINED;
    callbackInfo.thisArg = this_val;
    callbackInfo.argc = argc;
    callbackInfo.argv = argv;
    callbackInfo.data = functionInfo->data;
    NAPIHandleScope handleScope;
    NAPIStatus status = napi_open_handle_scope(functionInfo->env, &handleScope);
    if (status != NAPIOK)
    {
        return JS_UNDEFINED;
    }
    JSValue returnValue = *((JSValue *)functionInfo->callback(functionInfo->env, &callbackInfo));
    // 转移所有权
    JS_DupValue(functionInfo->env->context, returnValue);
    status = napi_close_handle_scope(functionInfo->env, handleScope);
    if (status != NAPIOK)
    {
        assert(false);
        JS_FreeValue(functionInfo->env->context, returnValue);

        return JS_UNDEFINED;
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

    CHECK_ARG(env, env->context);

    // malloc
    FunctionInfo *functionInfo = malloc(sizeof(FunctionInfo));
    RETURN_STATUS_IF_FALSE(env, functionInfo, NAPIMemoryError);
    functionInfo->env = env;
    functionInfo->data = data;
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

    // rc: 1
    // fakePrototypeValue 引用计数 +1
    JSValue functionValue = JS_NewCFunctionData(env->context, callAsFunction, 0, 0, 1, &fakePrototypeValue);
    // 转移所有权
    JS_FreeValue(env->context, fakePrototypeValue);
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(functionValue), NAPIPendingException);
    struct Handle *functionHandle;
    NAPIStatus addStatus = addValueToHandleScope(env, functionValue, &functionHandle);
    if (addStatus != NAPIOK)
    {
        JS_FreeValue(env->context, functionValue);

        return setLastErrorCode(env, addStatus);
    }
    *result = (NAPIValue)&functionHandle->value;

    return clearLastError(env);
}

// NAPIStringExpected + napi_set_named_property
static inline NAPIStatus setErrorCode(NAPIEnv env, NAPIValue error, NAPIValue code, const char *codeCString)
{
    CHECK_ENV(env);

    CHECK_ARG(env, env->context);

    if (code || codeCString)
    {
        JSValue codeValue;
        if (code)
        {
            codeValue = *((JSValue *)code);
            RETURN_STATUS_IF_FALSE(env, JS_IsString(codeValue), NAPIStringExpected);
        }
        else
        {
            codeValue = JS_NewStringLen(env->context, codeCString, codeCString ? strlen(codeCString) : 0);
            RETURN_STATUS_IF_FALSE(env, !JS_IsException(codeValue), NAPIPendingException);
        }
        CHECK_NAPI(napi_set_named_property(env, error, "code", code));
    }

    return clearLastError(env);
}

// NAPIPendingException/NAPIGenericFailure + addValueToHandleScope
NAPIStatus napi_create_error(NAPIEnv env, NAPIValue code, NAPIValue msg, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, msg);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->context);

    JSValue errorValue = JS_NewError(env->context);
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(errorValue), NAPIPendingException);
    int status = JS_DefinePropertyValueStr(env->context, errorValue, "message", *((JSValue *)msg),
                                           JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
    if (status <= 0)
    {
        JS_FreeValue(env->context, errorValue);
        if (status == -1)
        {
            return setLastErrorCode(env, NAPIPendingException);
        }
        else
        {
            return setLastErrorCode(env, NAPIGenericFailure);
        }
    }
    NAPIStatus callStatus = setErrorCode(env, (NAPIValue)&errorValue, code);
    if (callStatus != NAPIOK)
    {
        JS_FreeValue(env->context, errorValue);

        return callStatus;
    }
    struct Handle *errorHandle;
    NAPIStatus addStatus = addValueToHandleScope(env, errorValue, &errorHandle);
    if (addStatus != NAPIOK)
    {
        JS_FreeValue(env->context, errorValue);

        return setLastErrorCode(env, addStatus);
    }
    *result = (NAPIValue)&errorHandle->value;

    return clearLastError(env);
}

// NAPIPendingException/NAPIGenericFailure + addValueToHandleScope
NAPIStatus napi_create_type_error(NAPIEnv env, NAPIValue code, NAPIValue msg, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, msg);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->context);

    JS_ThrowTypeError(env->context, "");
    JSValue errorValue = JS_GetException(env->context);
    RETURN_STATUS_IF_FALSE(env, JS_IsError(env->context, errorValue), NAPIGenericFailure);
    int status = JS_DefinePropertyValueStr(env->context, errorValue, "message", *((JSValue *)msg),
                                           JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
    if (status <= 0)
    {
        JS_FreeValue(env->context, errorValue);
        if (status == -1)
        {
            return setLastErrorCode(env, NAPIPendingException);
        }
        else
        {
            return setLastErrorCode(env, NAPIGenericFailure);
        }
    }
    NAPIStatus callStatus = setErrorCode(env, (NAPIValue)&errorValue, code);
    if (callStatus != NAPIOK)
    {
        JS_FreeValue(env->context, errorValue);

        return callStatus;
    }
    struct Handle *errorHandle;
    NAPIStatus addStatus = addValueToHandleScope(env, errorValue, &errorHandle);
    if (addStatus != NAPIOK)
    {
        JS_FreeValue(env->context, errorValue);

        return setLastErrorCode(env, addStatus);
    }
    *result = (NAPIValue)&errorHandle->value;

    return clearLastError(env);
}

// NAPIPendingException/NAPIGenericFailure + addValueToHandleScope
NAPIStatus napi_create_range_error(NAPIEnv env, NAPIValue code, NAPIValue msg, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, msg);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->context);

    JS_ThrowRangeError(env->context, "");
    JSValue errorValue = JS_GetException(env->context);
    RETURN_STATUS_IF_FALSE(env, JS_IsError(env->context, errorValue), NAPIGenericFailure);
    int status = JS_DefinePropertyValueStr(env->context, errorValue, "message", *((JSValue *)msg),
                                           JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
    if (status <= 0)
    {
        JS_FreeValue(env->context, errorValue);
        if (status == -1)
        {
            return setLastErrorCode(env, NAPIPendingException);
        }
        else
        {
            return setLastErrorCode(env, NAPIGenericFailure);
        }
    }
    NAPIStatus callStatus = setErrorCode(env, (NAPIValue)&errorValue, code);
    if (callStatus != NAPIOK)
    {
        JS_FreeValue(env->context, errorValue);

        return callStatus;
    }
    struct Handle *errorHandle;
    NAPIStatus addStatus = addValueToHandleScope(env, errorValue, &errorHandle);
    if (addStatus != NAPIOK)
    {
        JS_FreeValue(env->context, errorValue);

        return setLastErrorCode(env, addStatus);
    }
    *result = (NAPIValue)&errorHandle->value;

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

NAPIStatus napi_get_value_uint32(NAPIEnv env, NAPIValue value, uint32_t *result)
{
    NAPI_PREAMBLE(env);
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

NAPIStatus napi_get_value_int64(NAPIEnv env, NAPIValue value, int64_t *result)
{
    NAPI_PREAMBLE(env);
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

static inline bool getUTF8CharacterBytesLength(uint8_t codePoint, uint8_t *bytesToWrite)
{
    if (!bytesToWrite)
    {
        return false;
    }
    if (codePoint >> 7 == 0)
    {
        *bytesToWrite = 1;
    }
    else if (codePoint >> 5 == 6)
    {
        *bytesToWrite = 2;
    }
    else if (codePoint >> 4 == 0xe)
    {
        *bytesToWrite = 3;
    }
    else if (codePoint >> 3 == 0x1e)
    {
        *bytesToWrite = 4;
    }
    else if (codePoint >> 2 == 0x3e)
    {
        *bytesToWrite = 5;
    }
    else if (codePoint >> 1 == 0x7e)
    {
        *bytesToWrite = 6;
    }
    else
    {
        return false;
    }

    return true;
}

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

    size_t length;
    const char *cString = JS_ToCStringLen(env->context, &length, *((JSValue *)value));
    if (!buf)
    {
        CHECK_ARG(env, result);
        *result = length;
        JS_FreeCString(env->context, cString);

        return clearLastError(env);
    }

    if (bufsize == 1)
    {
        buf[0] = '\0';
        JS_FreeCString(env->context, cString);

        return clearLastError(env);
    }
    // bufsize > 1
    if (length + 1 <= bufsize)
    {
        memmove(buf, cString, length);
        buf[length] = '\0';
        JS_FreeCString(env->context, cString);

        return clearLastError(env);
    }
    // bufsize > 1 length > 0
    // 截断
    memmove(buf, cString, bufsize);
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
        }
        else if (codePoint >> 7 == 0 || codePoint >> 6 == 3)
        {
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

NAPIStatus napi_get_value_string_utf16(NAPIEnv env, NAPIValue value, char16_t *buf, size_t bufsize, size_t *result)
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
    if (!buf)
    {
        CHECK_ARG(env, result);
        size_t length;
        const char *cString = JS_ToCStringLen(env->context, &length, *((JSValue *)value));
        // 计算大小
        *result = 0;
        const uint8_t *nextPointer = (const uint8_t *)cString;
        while (nextPointer < (const uint8_t *)cString + length)
        {
            int unicode =
                unicode_from_utf8(nextPointer, (int)(length - (nextPointer - (const uint8_t *)cString)), &nextPointer);
            if (unicode == -1)
            {
                break;
            }
            if (unicode < 0x10000)
            {
                ++(*result);
            }
            else
            {
                *result += 2;
            }
        }
        JS_FreeCString(env->context, cString);

        return clearLastError(env);
    }
    if (bufsize == 1)
    {
        buf[0] = '\0';

        return clearLastError(env);
    }
    size_t length;
    const char *cString = JS_ToCStringLen(env->context, &length, *((JSValue *)value));
    // 计算大小
    size_t index = 0;
    const uint8_t *nextPointer = (const uint8_t *)cString;
    while (nextPointer < (const uint8_t *)cString + length)
    {
        uint32_t unicode =
            unicode_from_utf8(nextPointer, (int)(length - (nextPointer - (const uint8_t *)cString)), &nextPointer);
        if (unicode == -1)
        {
            buf[index] = '\0';
            JS_FreeCString(env->context, cString);

            return setLastErrorCode(env, NAPIGenericFailure);
        }
        if (unicode < 0x10000)
        {
            uint8_t *ptr = (uint8_t *)&buf[index++];
            ptr[0] = unicode & 0xff;
            ptr[1] = unicode >> 8;
        }
        else if (index + 2 < bufsize)
        {
            unicode -= 0x10000;
            uint16_t head = (unicode >> 10) + 0xd800;
            uint16_t tail = unicode & 0x3ff + 0xdc00;
            uint8_t *ptr = (uint8_t *)&buf[index++];
            ptr[0] = head & 0xff;
            ptr[1] = head >> 8;
            ptr[3] = tail & 0xff;
            ptr[4] = tail >> 8;
        }
        else
        {
            buf[index] = '\0';
            JS_FreeCString(env->context, cString);

            return clearLastError(env);
        }
    }
    buf[index] = '\0';
    JS_FreeCString(env->context, cString);

    return clearLastError(env);
}

NAPIStatus napi_coerce_to_bool(NAPIEnv env, NAPIValue value, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->context);

    int boolStatus = JS_ToBool(env->context, *((JSValue *)value));
    RETURN_STATUS_IF_FALSE(env, boolStatus != -1, NAPIPendingException);
    JSValue boolValue = JS_NewBool(env->context, boolStatus);
    struct Handle *boolHandle;
    CHECK_NAPI(addValueToHandleScope(env, boolValue, &boolHandle));
    *result = (NAPIValue)&boolHandle->value;

    return clearLastError(env);
}

NAPIStatus napi_coerce_to_number(NAPIEnv env, NAPIValue value, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->context);

    double doubleValue;
    int floatStatus = JS_ToFloat64(env->context, &doubleValue, *((JSValue *)value));
    JSValue floatValue = JS_NewFloat64(env->context, doubleValue);
    struct Handle *floatHandle;
    CHECK_NAPI(addValueToHandleScope(env, floatValue, &floatHandle));
    *result = (NAPIValue)&floatHandle->value;

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

    CHECK_ARG(env, env->context);

    JSValue stringValue = JS_ToString(env->context, *((JSValue *)value));

    return clearLastError(env);
}

NAPIStatus napi_get_prototype(NAPIEnv env, NAPIValue object, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->context);

    JSValue prototypeValue = JS_GetPrototype(env->context, *((JSValue *)object));
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(prototypeValue), NAPIPendingException);
    struct Handle *prototypeHandle;
    NAPIStatus status = addValueToHandleScope(env, prototypeValue, &prototypeHandle);
    if (status != NAPIOK)
    {
        JS_FreeValue(env->context, prototypeValue);

        return setLastErrorCode(env, status);
    }
    *result = (NAPIValue)&prototypeHandle->value;

    return clearLastError(env);
}

NAPIStatus napi_get_property_names(NAPIEnv env, NAPIValue object, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    NAPIValue global, objectCtor, function;
    CHECK_NAPI(napi_get_global(env, &global));
    CHECK_NAPI(napi_get_named_property(env, global, "Object", &objectCtor));
    CHECK_NAPI(napi_get_named_property(env, objectCtor, "keys", &function));
    CHECK_NAPI(napi_call_function(env, objectCtor, function, 1, &object, result));

    return clearLastError(env);
}

NAPIStatus napi_set_property(NAPIEnv env, NAPIValue object, NAPIValue key, NAPIValue value)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, key);
    CHECK_ARG(env, value);

    CHECK_ARG(env, env->context);

    JSAtom atom = JS_ValueToAtom(env->context, *((JSValue *)key));
    RETURN_STATUS_IF_FALSE(env, atom != JS_ATOM_NULL, NAPINameExpected);

    int status =
        JS_SetProperty(env->context, *((JSValue *)object), atom, JS_DupValue(env->context, *((JSValue *)value)));
    RETURN_STATUS_IF_FALSE(env, status != -1, NAPIPendingException);
    RETURN_STATUS_IF_FALSE(env, status, NAPIGenericFailure);

    return clearLastError(env);
}

NAPIStatus napi_has_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);
    CHECK_ARG(env, key);

    CHECK_ARG(env, env->context);

    JSAtom atom = JS_ValueToAtom(env->context, *((JSValue *)key));
    RETURN_STATUS_IF_FALSE(env, atom != JS_ATOM_NULL, NAPINameExpected);
    int status = JS_HasProperty(env->context, *((JSValue *)object), atom);
    RETURN_STATUS_IF_FALSE(env, status != -1, NAPIPendingException);
    *result = status;

    return clearLastError(env);
}

NAPIStatus napi_get_property(NAPIEnv env, NAPIValue object, NAPIValue key, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, key);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->context);
    JSAtom atom = JS_ValueToAtom(env->context, *((JSValue *)key));
    RETURN_STATUS_IF_FALSE(env, atom != JS_ATOM_NULL, NAPINameExpected);
    JSValue value = JS_GetProperty(env->context, *((JSValue *)object), atom);
    struct Handle *handle;
    CHECK_NAPI(addValueToHandleScope(env, value, &handle));
    *result = (NAPIValue)&handle->value;

    return clearLastError(env);
}

NAPIStatus napi_delete_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, key);

    CHECK_ARG(env, env->context);

    JSAtom atom = JS_ValueToAtom(env->context, *((JSValue *)key));
    RETURN_STATUS_IF_FALSE(env, atom != JS_ATOM_NULL, NAPINameExpected);
    *result = JS_DeleteProperty(env->context, *((JSValue *)object), atom, 0);

    return clearLastError(env);
}

NAPIStatus napi_has_own_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, key);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->context);

    JSAtom atom = JS_ValueToAtom(env->context, *((JSValue *)key));
    RETURN_STATUS_IF_FALSE(env, atom != JS_ATOM_NULL, NAPINameExpected);
    int status = JS_GetOwnProperty(env->context, NULL, *((JSValue *)object), atom);
    RETURN_STATUS_IF_FALSE(env, status != -1, NAPIPendingException);
    *result = status;

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

NAPIStatus napi_has_named_property(NAPIEnv env, NAPIValue object, const char *utf8name, bool *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->context);

    JSAtom atom = JS_NewAtomLen(env->context, utf8name, utf8name ? strlen(utf8name) : 0);
    RETURN_STATUS_IF_FALSE(env, atom != JS_ATOM_NULL, NAPINameExpected);
    int status = JS_HasProperty(env->context, *((JSValue *)object), atom);
    RETURN_STATUS_IF_FALSE(env, status != -1, NAPIPendingException);
    *result = status;

    return clearLastError(env);
}

NAPIStatus napi_get_named_property(NAPIEnv env, NAPIValue object, const char *utf8name, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->context);

    JSAtom atom = JS_NewAtomLen(env->context, utf8name, utf8name ? strlen(utf8name) : 0);
    RETURN_STATUS_IF_FALSE(env, atom != JS_ATOM_NULL, NAPINameExpected);
    if (!utf8name)
    {
        utf8name = "";
    }
    JSValue value = JS_GetPropertyStr(env->context, *((JSValue *)object), utf8name);
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(value), NAPIPendingException);
    struct Handle *handle;
    CHECK_NAPI(addValueToHandleScope(env, value, &handle));
    *result = (NAPIValue)&handle->value;

    return clearLastError(env);
}

NAPIStatus napi_set_element(NAPIEnv env, NAPIValue object, uint32_t index, NAPIValue value)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);

    CHECK_ARG(env, env->context);

    int status = JS_SetPropertyUint32(env->context, *((JSValue *)object), index, *((JSValue *)value));
    RETURN_STATUS_IF_FALSE(env, status != -1, NAPIPendingException);

    return clearLastError(env);
}

NAPIStatus napi_has_element(NAPIEnv env, NAPIValue object, uint32_t index, bool *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->context);

    JSAtom atom = JS_NewAtomUInt32(env->context, index);
    RETURN_STATUS_IF_FALSE(env, atom != JS_ATOM_NULL, NAPINameExpected);
    int status = JS_HasProperty(env->context, *((JSValue *)object), atom);
    RETURN_STATUS_IF_FALSE(env, status != -1, NAPIPendingException);
    *result = status;

    return clearLastError(env);
}

NAPIStatus napi_get_element(NAPIEnv env, NAPIValue object, uint32_t index, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->context);

    JSValue value = JS_GetPropertyUint32(env->context, *((JSValue *)object), index);
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(value), NAPIPendingException);
    struct Handle *handle;
    CHECK_NAPI(addValueToHandleScope(env, value, &handle));
    *result = (NAPIValue)&handle->value;

    return clearLastError(env);
}

NAPIStatus napi_delete_element(NAPIEnv env, NAPIValue object, uint32_t index, bool *result)
{
    NAPI_PREAMBLE(env);

    CHECK_ARG(env, env->context);

    JSAtom atom = JS_NewAtomUInt32(env->context, index);
    RETURN_STATUS_IF_FALSE(env, atom != JS_ATOM_NULL, NAPINameExpected);
    *result = JS_DeleteProperty(env->context, *((JSValue *)object), atom, 0);

    return clearLastError(env);
}

NAPIStatus napi_define_properties(NAPIEnv env, NAPIValue object, size_t property_count,
                                  const NAPIPropertyDescriptor *properties)
{
    NAPI_PREAMBLE(env);

    CHECK_ARG(env, env->context);

    if (property_count > 0)
    {
        CHECK_ARG(env, properties);
    }

    for (size_t i = 0; i < property_count; i++)
    {
        const NAPIPropertyDescriptor *p = properties + i;

        int flags = 0;

        if (p->attributes & NAPIConfigurable)
        {
            flags |= JS_PROP_HAS_CONFIGURABLE | JS_PROP_CONFIGURABLE;
        }
        if (p->attributes & NAPIEnumerable)
        {
            flags |= JS_PROP_HAS_ENUMERABLE | JS_PROP_ENUMERABLE;
        }

        NAPIValue getter, setter, value;
        if (p->getter || p->setter)
        {
            if (p->getter)
            {
                CHECK_NAPI(napi_create_function(env, p->utf8name, NAPI_AUTO_LENGTH, p->getter, p->data, &getter));
                flags |= JS_PROP_HAS_GET;
            }
            if (p->setter)
            {
                CHECK_NAPI(napi_create_function(env, p->utf8name, NAPI_AUTO_LENGTH, p->setter, p->data, &setter));
                flags |= JS_PROP_HAS_SET;
            }
        }
        else if (p->method)
        {
            CHECK_NAPI(napi_create_function(env, p->utf8name, NAPI_AUTO_LENGTH, p->method, p->data, &value));
        }
        else
        {
            // value
            if (p->attributes & NAPIWritable)
            {
                flags |= JS_PROP_HAS_WRITABLE | JS_PROP_WRITABLE;
            }
            value = p->value;
        }

        if (value)
        {
            if (p->name)
            {
                JSAtom atom = JS_ValueToAtom(env->context, *((JSValue *)p->name));
                RETURN_STATUS_IF_FALSE(env, atom != JS_ATOM_NULL, NAPIPendingException);
                RETURN_STATUS_IF_FALSE(
                    env, JS_DefinePropertyValue(env->context, *((JSValue *)object), atom, *((JSValue *)value), flags),
                    NAPIGenericFailure);
            }
            else
            {
                RETURN_STATUS_IF_FALSE(env,
                                       JS_DefinePropertyValueStr(env->context, *((JSValue *)object), p->utf8name ?: "",
                                                                 *((JSValue *)value), flags),
                                       NAPIGenericFailure);
            }

            return clearLastError(env);
        }
        else if (getter || setter)
        {
            JSAtom atom = JS_ATOM_NULL;
            if (p->name)
            {
                atom = JS_ValueToAtom(env->context, *((JSValue *)p->name));
                RETURN_STATUS_IF_FALSE(env, atom != JS_ATOM_NULL, NAPIPendingException);
            }
            else
            {
                atom = JS_NewAtomLen(env->context, p->utf8name, p->utf8name ? strlen(p->utf8name) : 0);
                RETURN_STATUS_IF_FALSE(env, atom != JS_ATOM_NULL, NAPIPendingException);
            }
            JS_DefinePropertyGetSet(env->context, *((JSValue *)object), atom,
                                    getter ? *((JSValue *)getter) : JS_UNDEFINED,
                                    setter ? *((JSValue *)setter) : JS_UNDEFINED);
        }
    }

    return clearLastError(env);
}

NAPIStatus napi_is_array(NAPIEnv env, NAPIValue value, bool *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->context);

    int status = JS_IsArray(env->context, *((JSValue *)value));
    RETURN_STATUS_IF_FALSE(env, status != -1, NAPIPendingException);
    *result = status;

    return clearLastError(env);
}

NAPIStatus napi_get_array_length(NAPIEnv env, NAPIValue value, uint32_t *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->context);

    JSValue lengthValue = JS_GetPropertyStr(env->context, *((JSValue *)value), "length");
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(lengthValue), NAPIPendingException);
    RETURN_STATUS_IF_FALSE(env, JS_IsNumber(lengthValue), NAPIGenericFailure);
    *result = JS_VALUE_GET_INT(lengthValue);

    return clearLastError(env);
}

NAPIStatus napi_strict_equals(NAPIEnv env, NAPIValue lhs, NAPIValue rhs, bool *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, lhs);
    CHECK_ARG(env, rhs);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->context);

    JSValue globalValue = JS_GetGlobalObject(env->context);
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(globalValue), NAPIPendingException);
    JSAtom atom = JS_ValueToAtom(env->context, env->strictEqualSymbolValue);
    if (atom == JS_ATOM_NULL)
    {
        JS_FreeValue(env->context, globalValue);

        return setLastErrorCode(env, NAPIPendingException);
    }
    JSValue strictEqualValue = JS_GetProperty(env->context, globalValue, atom);
    JS_FreeValue(env->context, globalValue);
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(strictEqualValue), NAPIPendingException);
    if (!JS_IsFunction(env->context, strictEqualValue))
    {
        JS_FreeValue(env->context, strictEqualValue);

        return setLastErrorCode(env, NAPIGenericFailure);
    }
    JSValue argv[] = {
        *((JSValue *)lhs),
        *((JSValue *)rhs),
    };
    JSValue returnValue = JS_Call(env->context, strictEqualValue, JS_UNDEFINED, 2, argv);
    JS_FreeValue(env->context, strictEqualValue);
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(returnValue), NAPIPendingException);
    if (!JS_IsBool(returnValue))
    {
        JS_FreeValue(env->context, returnValue);

        return setLastErrorCode(env, NAPIGenericFailure);
    }
    *result = JS_VALUE_GET_BOOL(returnValue);

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

    CHECK_ARG(env, env->context);

    RETURN_STATUS_IF_FALSE(env, JS_IsFunction(env->context, *((JSValue *)func)), NAPIFunctionExpected);
    JSValue *internalArgv = NULL;
    if (argc > 0)
    {
        internalArgv = malloc(sizeof(JSValue) * argc);
        RETURN_STATUS_IF_FALSE(env, internalArgv, NAPIMemoryError);
        for (size_t i = 0; i < argc; ++i)
        {
            internalArgv[i] = *((JSValue *)argv[i]);
        }
    }
    JSValue returnValue = JS_Call(env->context, *((JSValue *)func), JS_UNDEFINED, (int)argc, internalArgv);
    free(internalArgv);
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(returnValue), NAPIPendingException);
    struct Handle *handle;
    NAPIStatus status = addValueToHandleScope(env, returnValue, &handle);
    if (status != NAPIOK)
    {
        JS_FreeValue(env->context, returnValue);

        return setLastErrorCode(env, status);
    }
    *result = (NAPIValue)&handle->value;

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

    CHECK_ARG(env, env->context);

    RETURN_STATUS_IF_FALSE(env, JS_IsConstructor(env->context, *((JSValue *)constructor)), NAPIFunctionExpected);
    JSValue *internalArgv = NULL;
    if (argc > 0)
    {
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

        return setLastErrorCode(env, status);
    }
    *result = (NAPIValue)&handle->value;

    return clearLastError(env);
}

NAPIStatus napi_instanceof(NAPIEnv env, NAPIValue object, NAPIValue constructor, bool *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, object);
    CHECK_ARG(env, result);

    RETURN_STATUS_IF_FALSE(env, JS_IsConstructor(env->context, *((JSValue *)constructor)), NAPIFunctionExpected);
    int status = JS_IsInstanceOf(env->context, *((JSValue *)object), *((JSValue *)constructor));
    RETURN_STATUS_IF_FALSE(env, status != -1, NAPIPendingException);
    *result = status;

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
            argv[i] = (NAPIValue)&cbinfo->argv[i];
        }
        if (i < *argc)
        {
            for (; i < *argc; i++)
            {
                argv[i] = (NAPIValue)&JS_UNDEFINED;
            }
        }
    }
    if (argc)
    {
        *argc = cbinfo->argc;
    }
    if (thisArg)
    {
        *thisArg = (NAPIValue)&cbinfo->thisArg;
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

    *result = (NAPIValue)&cbinfo->newTarget;

    return clearLastError(env);
}

static JSValue callAsConstructor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv)
{
}

NAPIStatus napi_define_class(NAPIEnv env, const char *utf8name, size_t length, NAPICallback constructor, void *data,
                             size_t propertyCount, const NAPIPropertyDescriptor *properties, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);
    CHECK_ARG(env, constructor);

    if (propertyCount > 0)
    {
        CHECK_ARG(env, properties);
    }

    CHECK_ARG(env, env->context);

    RETURN_STATUS_IF_FALSE(env, length == NAPI_AUTO_LENGTH, NAPIInvalidArg);
    JSValue constructorValue = JS_NewCFunction2(env->context, callAsConstructor, utf8name, 0, JS_CFUNC_constructor, 0);
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(constructorValue), NAPIPendingException);
    JSValue prototype = JS_NewObjectClass(env->context, (int)functionClassId);
    if (JS_IsException(prototype))
    {
        JS_FreeValue(env->context, constructorValue);

        return setLastErrorCode(env, NAPIPendingException);
    }
    JS_SetConstructor(env->context, constructorValue, prototype);
    JS_SetClassProto(env->context, functionClassId, prototype);

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
            JS_FreeValue(env->context, constructorValue);
            JS_FreeValue(env->context, prototype);

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
            JS_FreeValue(env->context, constructorValue);
            JS_FreeValue(env->context, prototype);
            free(staticDescriptors);

            return setLastErrorCode(env, NAPIMemoryError);
        }
    }

    //    size_t instancePropertyIndex = 0;
    //    size_t staticPropertyIndex = 0;
    //
    //    for (size_t i = 0; i < propertyCount; i++)
    //    {
    //        if ((properties[i].attributes & NAPIStatic) != 0)
    //        {
    //            staticDescriptors[staticPropertyIndex] = properties[i];
    //            staticPropertyIndex += 1;
    //        }
    //        else
    //        {
    //            instanceDescriptors[instancePropertyIndex] = properties[i];
    //            instancePropertyIndex += 1;
    //        }
    //    }
    //
    //    if (staticPropertyCount > 0)
    //    {
    //        NAPIStatus status = napi_define_properties(env, (NAPIValue)function, staticPropertyIndex,
    //        staticDescriptors); if (status != NAPIOK)
    //        {
    //            free(staticDescriptors);
    //            free(instanceDescriptors);
    //
    //            return status;
    //        }
    //    }
    //
    //    free(staticDescriptors);
    //
    //    if (instancePropertyCount > 0)
    //    {
    //        NAPIValue prototypeValue;
    //        NAPIStatus status = napi_get_named_property(env, (NAPIValue)function, "prototype", &prototypeValue);
    //        if (status != NAPIOK)
    //        {
    //            free(instanceDescriptors);
    //
    //            return status;
    //        }
    //        status = napi_define_properties(env, prototypeValue, instancePropertyIndex, instanceDescriptors);
    //        if (status != NAPIOK)
    //        {
    //            free(instanceDescriptors);
    //
    //            return status;
    //        }
    //    }
    //    free(instanceDescriptors);

    return clearLastError(env);
}

typedef struct
{
    NAPIEnv env;
    void *finalizeHint;
    NAPIFinalize finalizeCallback;
    void *data
} ExternalInfo;

static JSClassID externalClassId;

NAPIStatus napi_create_external(NAPIEnv env, void *data, NAPIFinalize finalizeCB, void *finalizeHint, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    ExternalInfo *externalInfo = malloc(sizeof(ExternalInfo));
    RETURN_STATUS_IF_FALSE(env, externalInfo, NAPIMemoryError);
    externalInfo->env = env;
    externalInfo->finalizeCallback = finalizeCB;
    externalInfo->finalizeHint = finalizeHint;
    externalInfo->data = data;
    JSValue object = JS_NewObjectClass(env->context, (int)functionClassId);
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

        return setLastErrorCode(env, status);
    }
    *result = (NAPIValue)&handle->value;

    return clearLastError(env);
}

NAPIStatus napi_get_value_external(NAPIEnv env, NAPIValue value, void **result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->context);

    ExternalInfo *externalInfo = JS_GetOpaque(*((JSValue *)value), externalClassId);
    *result = externalInfo ? externalInfo->data : NULL;

    return clearLastError(env);
}

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

NAPIStatus napi_throw_error(NAPIEnv env, const char *code, const char *msg)
{
    NAPI_PREAMBLE(env);

    CHECK_ARG(env, env->context);

    JSValue errorValue = JS_NewError(env->context);
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(errorValue), NAPIPendingException);
    int status = JS_DefinePropertyValueStr(env->context, errorValue, "message", *((JSValue *)msg),
                                           JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
    if (status <= 0)
    {
        JS_FreeValue(env->context, errorValue);
        if (status == -1)
        {
            return setLastErrorCode(env, NAPIPendingException);
        }
        else
        {
            return setLastErrorCode(env, NAPIGenericFailure);
        }
    }
    JSNewString NAPIStatus callStatus = setErrorCode(env, (NAPIValue)&errorValue, code);
    if (callStatus != NAPIOK)
    {
        JS_FreeValue(env->context, errorValue);

        return callStatus;
    }

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
        externalInfo->finalizeCallback(externalInfo->env, externalInfo->data, externalInfo->finalizeHint);
        x
    }
    free(externalInfo);
}

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
        JS_NewClassID(&externalClassId);
        JSClassDef externalClassDef = {"External", externalFinalizer};
        int status = JS_NewClass(runtime, externalClassId, &externalClassDef);
        if (status == -1)
        {
            JS_FreeRuntime(runtime);
            free(*env);
            runtime = NULL;

            return NAPIMemoryError;
        }
        JS_NewClassID(&functionClassId);
        JSClassDef functionClassDef = {"ExternalFunction", functionFinalizer};
        status = JS_NewClass(runtime, functionClassId, &functionClassDef);
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
    const char *string =
        "const symbol = Symbol(\"strictEqual\");globalThis[symbol] = (op1, op2) => op1 === op2;symbol;";
    JSValue returnValue =
        JS_Eval(context, string, strlen(string), "https://napi.com/qjs_builtin.js", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(returnValue))
    {
        JS_FreeContext(context);
        JS_FreeRuntime(runtime);
        free(*env);

        return NAPIGenericFailure;
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