#include <sys/queue.h>

#include <napi/js_native_api.h>

// setLastErrorCode 会处理 env == NULL 问题
#define RETURN_STATUS_IF_FALSE(condition, status)                                                                      \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            return status;                                                                                             \
        }                                                                                                              \
    } while (0)

#define CHECK_ARG(arg) RETURN_STATUS_IF_FALSE(arg, NAPIInvalidArg)

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

#define CHECK_JSC(env)                                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        CHECK_ARG(env);                                                                                                \
        RETURN_STATUS_IF_FALSE(!((env)->lastException), NAPIPendingException);                                         \
    } while (0)

#include <JavaScriptCore/JavaScriptCore.h>
#include <assert.h>
#include <napi/js_native_api_types.h>
#include <stdio.h>
#include <stdlib.h>

struct OpaqueNAPIRef
{
    LIST_ENTRY(OpaqueNAPIRef) node; // size_t
    JSValueRef value;               // size_t
    uint8_t count;                  // 8
};

// undefined 和 null 实际上也可以当做 exception
// 抛出，所以异常检查只需要检查是否为 C NULL
struct OpaqueNAPIEnv
{
    JSGlobalContextRef context;                     // size_t
    JSValueRef lastException;                       // size_t
    LIST_HEAD(, OpaqueNAPIRef) strongReferenceHead; // size_t
};

// NAPIMemoryError
NAPIStatus napi_get_undefined(NAPIEnv env, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    *result = (NAPIValue)JSValueMakeUndefined(env->context);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return NAPIOK;
}

// NAPIMemoryError
NAPIStatus napi_get_null(NAPIEnv env, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    *result = (NAPIValue)JSValueMakeNull(env->context);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return NAPIOK;
}

// NAPIMemoryError
NAPIStatus napi_get_global(NAPIEnv env, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    *result = (NAPIValue)JSContextGetGlobalObject(env->context);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return NAPIOK;
}

// NAPIMemoryError
NAPIStatus napi_get_boolean(NAPIEnv env, bool value, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    *result = (NAPIValue)JSValueMakeBoolean(env->context, value);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return NAPIOK;
}

// NAPIMemoryError
NAPIStatus napi_create_double(NAPIEnv env, double value, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    *result = (NAPIValue)JSValueMakeNumber(env->context, value);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return NAPIOK;
}

// NAPIMemoryError
NAPIStatus napi_create_int32(NAPIEnv env, int32_t value, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    *result = (NAPIValue)JSValueMakeNumber(env->context, value);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return NAPIOK;
}

// NAPIMemoryError
NAPIStatus napi_create_uint32(NAPIEnv env, uint32_t value, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    *result = (NAPIValue)JSValueMakeNumber(env->context, value);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return NAPIOK;
}

// NAPIMemoryError
NAPIStatus napi_create_int64(NAPIEnv env, int64_t value, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    *result = (NAPIValue)JSValueMakeNumber(env->context, (double)value);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return NAPIOK;
}

// JavaScriptCore 只能接受 \0 结尾的字符串
// 传入 str NULL 则为 ""
// V8 引擎传入 NULL 直接崩溃
// NAPIMemoryError
NAPIStatus napi_create_string_utf8(NAPIEnv env, const char *str, size_t length, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

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

    return NAPIOK;
}

// 1. external -> 不透明指针 + finalizer + 调用一个回调
// 2. Function -> __function__ + external
// 3. Constructor -> __constructor__ + external
// 4. Reference -> 引用计数 + setWeak/clearWeak -> __reference__

// Function
struct OpaqueNAPICallbackInfo
{
    JSObjectRef newTarget;  // size_t
    JSObjectRef thisArg;    // size_t
    const JSValueRef *argv; // size_t
    void *data;             // size_t
    size_t argc;            // size_t
};

typedef struct
{
    NAPIEnv env; // size_t
    void *data;  // size_t
} BaseInfo;

typedef struct
{
    BaseInfo baseInfo;
    NAPIFinalize finalizeCallback; // size_t
    void *finalizeHint;            // size_t
} ExternalInfo;

typedef struct
{
    BaseInfo baseInfo;
    NAPICallback callback; // size_t
} FunctionInfo;

static const char *const FUNCTION_STRING = "__function__";

// 所有参数都会存在，返回值可以为 NULL
static JSValueRef callAsFunction(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount,
                                 const JSValueRef arguments[], JSValueRef *exception)
{
    // JSObjectSetPrototype 确实可以传入非对象 JSValue，但是会被替换为 JS
    // Null，并且可以正常取出来
    JSStringRef keyStringRef = JSStringCreateWithUTF8CString(FUNCTION_STRING);
    if (!keyStringRef)
    {
        return NULL;
    }
    JSValueRef valueRef = JSObjectGetProperty(ctx, function, keyStringRef, exception);
    if (*exception || !valueRef)
    {
        return NULL;
    }
    if (!JSValueIsObject(ctx, valueRef))
    {
        // 正常不应当出现
        assert(false);

        // 返回 NULL 会被转换为 undefined
        return NULL;
    }
    JSObjectRef prototypeObjectRef = JSValueToObject(ctx, valueRef, exception);
    if (*exception || !prototypeObjectRef)
    {
        // 正常不应当出现
        assert(false);

        return NULL;
    }
    FunctionInfo *functionInfo = JSObjectGetPrivate(prototypeObjectRef);
    if (!functionInfo || !functionInfo->baseInfo.env || !functionInfo->callback)
    {
        // 正常不应当出现
        assert(false);

        return NULL;
    }

    // JavaScriptCore 参数都不是 NULL
    struct OpaqueNAPICallbackInfo callbackInfo;
    callbackInfo.newTarget = NULL;
    callbackInfo.thisArg = thisObject;
    callbackInfo.argc = argumentCount;
    callbackInfo.argv = arguments;
    callbackInfo.data = functionInfo->baseInfo.data;

    JSValueRef returnValue = (JSValueRef)functionInfo->callback(functionInfo->baseInfo.env, &callbackInfo);
    if (functionInfo->baseInfo.env->lastException)
    {
        *exception = functionInfo->baseInfo.env->lastException;
        functionInfo->baseInfo.env->lastException = NULL;

        return NULL;
    }

    return returnValue;
}

static void functionFinalize(JSObjectRef object)
{
    free(JSObjectGetPrivate(object));
}

// NAPIMemoryError
NAPIStatus napi_create_function(NAPIEnv env, const char *utf8name, size_t length, NAPICallback cb, void *data,
                                NAPIValue *result)
{
    CHECK_JSC(env);
    CHECK_ARG(cb);
    CHECK_ARG(result);

    RETURN_STATUS_IF_FALSE(length == NAPI_AUTO_LENGTH, NAPIInvalidArg);

    FunctionInfo *functionInfo = malloc(sizeof(FunctionInfo));
    RETURN_STATUS_IF_FALSE(functionInfo, NAPIMemoryError);
    functionInfo->baseInfo.env = env;
    functionInfo->callback = cb;
    functionInfo->baseInfo.data = data;

    JSClassDefinition classDefinition = kJSClassDefinitionEmpty;
    // 使用 Object.prototype 作为 instance.[[prototype]]
    classDefinition.attributes = kJSClassAttributeNoAutomaticPrototype;
    // 最重要的是提供 finalize
    classDefinition.finalize = functionFinalize;
    JSClassRef classRef = JSClassCreate(&classDefinition);
    if (!classRef)
    {
        free(functionInfo);

        return NAPIMemoryError;
    }
    JSObjectRef prototype = JSObjectMake(env->context, classRef, functionInfo);
    JSClassRelease(classRef);
    if (!prototype)
    {
        free(functionInfo);

        return NAPIMemoryError;
    }

    // utf8name 会被当做函数的 .name 属性
    // V8 传入 NULL 会直接变成 ""
    JSStringRef stringRef = JSStringCreateWithUTF8CString(utf8name);
    // JSObjectMakeFunctionWithCallback 传入函数名为 NULL 则为 anonymous
    JSObjectRef functionObjectRef = JSObjectMakeFunctionWithCallback(env->context, stringRef, callAsFunction);
    // JSStringRelease 不能传入 NULL
    if (stringRef)
    {
        JSStringRelease(stringRef);
    }
    RETURN_STATUS_IF_FALSE(functionObjectRef, NAPIMemoryError);

    *result = (NAPIValue)functionObjectRef;
    stringRef = JSStringCreateWithUTF8CString(FUNCTION_STRING);
    RETURN_STATUS_IF_FALSE(stringRef, NAPIMemoryError);
    JSObjectSetProperty(env->context, functionObjectRef, stringRef, prototype,
                        kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontEnum | kJSPropertyAttributeDontDelete,
                        &env->lastException);
    JSStringRelease(stringRef);
    CHECK_JSC(env);

    return NAPIOK;
}

// NAPIPendingException/NAPIMemoryError
NAPIStatus napi_typeof(NAPIEnv env, NAPIValue value, NAPIValueType *result)
{
    CHECK_JSC(env);
    CHECK_ARG(value);
    CHECK_ARG(result);

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
        return NAPIInvalidArg;
    }

    return NAPIOK;
}

// NAPINumberExpected/NAPIPendingException
NAPIStatus napi_get_value_double(NAPIEnv env, NAPIValue value, double *result)
{
    CHECK_JSC(env);
    CHECK_ARG(value);
    CHECK_ARG(result);

    RETURN_STATUS_IF_FALSE(JSValueIsNumber(env->context, (JSValueRef)value), NAPINumberExpected);

    *result = JSValueToNumber(env->context, (JSValueRef)value, &env->lastException);
    CHECK_JSC(env);

    return NAPIOK;
}

NAPIStatus napi_get_value_int32(NAPIEnv env, NAPIValue value, int32_t *result)
{
    CHECK_JSC(env);
    CHECK_ARG(value);
    CHECK_ARG(result);

    RETURN_STATUS_IF_FALSE(JSValueIsNumber(env->context, (JSValueRef)value), NAPINumberExpected);

    // 缺少 int64_t 作为中间转换，会固定在 -2147483648
    // 虽然 ubsan 没有说明该行为是 UB，但是还是需要从规范上考虑是否该行为是 UB？ 但是 uint32_t 不会
    *result = (int32_t)(int64_t)JSValueToNumber(env->context, (JSValueRef)value, &env->lastException);
    CHECK_JSC(env);

    return NAPIOK;
}

NAPIStatus napi_get_value_uint32(NAPIEnv env, NAPIValue value, uint32_t *result)
{
    CHECK_JSC(env);
    CHECK_ARG(value);
    CHECK_ARG(result);

    RETURN_STATUS_IF_FALSE(JSValueIsNumber(env->context, (JSValueRef)value), NAPINumberExpected);

    *result = (uint32_t)JSValueToNumber(env->context, (JSValueRef)value, &env->lastException);
    CHECK_JSC(env);

    return NAPIOK;
}

NAPIStatus napi_get_value_int64(NAPIEnv env, NAPIValue value, int64_t *result)
{
    CHECK_JSC(env);
    CHECK_ARG(value);
    CHECK_ARG(result);

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

    return NAPIOK;
}

// NAPIBooleanExpected
NAPIStatus napi_get_value_bool(NAPIEnv env, NAPIValue value, bool *result)
{
    CHECK_ARG(env);
    CHECK_ARG(value);
    CHECK_ARG(result);

    RETURN_STATUS_IF_FALSE(JSValueIsBoolean(env->context, (JSValueRef)value), NAPIBooleanExpected);
    *result = JSValueToBoolean(env->context, (JSValueRef)value);

    return NAPIOK;
}

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
    CHECK_JSC(env);
    CHECK_ARG(value);

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
            CHECK_ARG(result);
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

                return NAPIMemoryError;
            }
            // 返回值一定带 \0
            size_t copied = JSStringGetUTF8CString(stringRef, buffer, length);
            free(buffer);
            if (!copied)
            {
                // 失败
                JSStringRelease(stringRef);

                return NAPIMemoryError;
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

                    return NAPIOK;
                }
                length = JSStringGetUTF8CString(stringRef, buffer, length);
                if (!length)
                {
                    JSStringRelease(stringRef);
                    free(buffer);

                    return NAPIMemoryError;
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

                                return NAPIMemoryError;
                            }
                            // 校验
                            if (((uint8_t *)buffer)[i] >> 6 != 2)
                            {
                                assert(false);
                                JSStringRelease(stringRef);
                                free(buffer);

                                return NAPIMemoryError;
                            }
                            if (((uint8_t *)buffer)[i] < 0x80 || ((uint8_t *)buffer)[i] > 0xbf)
                            {
                                is8Bit = false;
                                break;
                            }

                            ++i;
                            continue;
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

                            return NAPIMemoryError;
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

                    return NAPIMemoryError;
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

    return NAPIOK;
}

// NAPIMemoryError
NAPIStatus napi_coerce_to_bool(NAPIEnv env, NAPIValue value, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(value);
    CHECK_ARG(result);

    *result = (NAPIValue)JSValueMakeBoolean(env->context, JSValueToBoolean(env->context, (JSValueRef)value));
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return NAPIOK;
}

// NAPIMemoryError
NAPIStatus napi_coerce_to_number(NAPIEnv env, NAPIValue value, NAPIValue *result)
{
    CHECK_JSC(env);
    CHECK_ARG(value);
    CHECK_ARG(result);

    double doubleValue = JSValueToNumber(env->context, (JSValueRef)value, &env->lastException);
    CHECK_JSC(env);
    *result = (NAPIValue)JSValueMakeNumber(env->context, doubleValue);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return NAPIOK;
}

// NAPIMemoryError
NAPIStatus napi_coerce_to_string(NAPIEnv env, NAPIValue value, NAPIValue *result)
{
    CHECK_JSC(env);
    CHECK_ARG(value);
    CHECK_ARG(result);

    JSStringRef stringRef = JSValueToStringCopy(env->context, (JSValueRef)value, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(stringRef, NAPIMemoryError);
    *result = (NAPIValue)JSValueMakeString(env->context, stringRef);
    JSStringRelease(stringRef);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return NAPIOK;
}

NAPIStatus napi_set_property(NAPIEnv env, NAPIValue object, NAPIValue key, NAPIValue value)
{
    CHECK_JSC(env);
    CHECK_ARG(object);
    CHECK_ARG(key);
    CHECK_ARG(value);

    RETURN_STATUS_IF_FALSE(JSValueIsString(env->context, (JSValueRef)key) ||
                               JSValueIsNumber(env->context, (JSValueRef)key),
                           NAPINameExpected);
    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)object), NAPIObjectExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)object, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(objectRef, NAPIMemoryError);

    JSObjectSetPropertyForKey(env->context, objectRef, (JSValueRef)key, (JSValueRef)value, kJSPropertyAttributeNone,
                              &env->lastException);
    CHECK_JSC(env);

    return NAPIOK;
}

NAPIStatus napi_has_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result)
{
    CHECK_JSC(env);
    CHECK_ARG(object);
    CHECK_ARG(key);
    CHECK_ARG(result);

    RETURN_STATUS_IF_FALSE(JSValueIsString(env->context, (JSValueRef)key) ||
                               JSValueIsNumber(env->context, (JSValueRef)key),
                           NAPINameExpected);
    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)object), NAPIObjectExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)object, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(objectRef, NAPIMemoryError);

    *result = JSObjectHasPropertyForKey(env->context, objectRef, (JSValueRef)key, &env->lastException);
    CHECK_JSC(env);

    return NAPIOK;
}

NAPIStatus napi_get_property(NAPIEnv env, NAPIValue object, NAPIValue key, NAPIValue *result)
{
    CHECK_JSC(env);
    CHECK_ARG(object);
    CHECK_ARG(key);
    CHECK_ARG(result);

    RETURN_STATUS_IF_FALSE(JSValueIsString(env->context, (JSValueRef)key) ||
                               JSValueIsNumber(env->context, (JSValueRef)key),
                           NAPINameExpected);
    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)object), NAPIObjectExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)object, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(objectRef, NAPIMemoryError);

    *result = (NAPIValue)JSObjectGetPropertyForKey(env->context, objectRef, (JSValueRef)key, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return NAPIOK;
}

NAPIStatus napi_delete_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result)
{
    CHECK_JSC(env);
    CHECK_ARG(object);
    CHECK_ARG(key);

    RETURN_STATUS_IF_FALSE(JSValueIsString(env->context, (JSValueRef)key) ||
                               JSValueIsNumber(env->context, (JSValueRef)key),
                           NAPINameExpected);
    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)object), NAPIObjectExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)object, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(objectRef, NAPIMemoryError);

    bool deleteSuccess = JSObjectDeletePropertyForKey(env->context, objectRef, (JSValueRef)key, &env->lastException);
    CHECK_JSC(env);
    if (result)
    {
        *result = deleteSuccess;
    }

    return NAPIOK;
}

NAPIStatus napi_is_array(NAPIEnv env, NAPIValue value, bool *result)
{
    CHECK_ARG(env);
    CHECK_ARG(value);
    CHECK_ARG(result);

    *result = JSValueIsArray(env->context, (JSValueRef)value);

    return NAPIOK;
}

NAPIStatus napi_call_function(NAPIEnv env, NAPIValue thisValue, NAPIValue func, size_t argc, const NAPIValue *argv,
                              NAPIValue *result)
{
    CHECK_JSC(env);
    CHECK_ARG(func);
    if (argc > 0)
    {
        CHECK_ARG(argv);
    }

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)func), NAPIObjectExpected);

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)func, &env->lastException);
    RETURN_STATUS_IF_FALSE(objectRef, NAPIMemoryError);
    RETURN_STATUS_IF_FALSE(JSObjectIsFunction(env->context, objectRef), NAPIFunctionExpected);

    JSObjectRef thisObjectRef = NULL;
    if (!thisValue) {
        thisObjectRef = JSContextGetGlobalObject(env->context);
    } else {
        RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)thisValue), NAPIObjectExpected);
    }
    JSValueRef returnValue = NULL;
    if (!argc)
    {
        returnValue = JSObjectCallAsFunction(env->context, objectRef, thisObjectRef, 0, NULL, &env->lastException);
    }
    else if (argc <= 8)
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
        free(argumentArray);
    }
    CHECK_JSC(env);

    if (result)
    {
        RETURN_STATUS_IF_FALSE(returnValue, NAPIMemoryError);
        *result = (NAPIValue)returnValue;
    }

    return NAPIOK;
}

NAPIStatus napi_new_instance(NAPIEnv env, NAPIValue constructor, size_t argc, const NAPIValue *argv, NAPIValue *result)
{
    CHECK_JSC(env);
    CHECK_ARG(constructor);
    if (argc > 0)
    {
        CHECK_ARG(argv);
    }
    CHECK_ARG(result);

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)constructor), NAPIObjectExpected);
    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)constructor, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(objectRef, NAPIMemoryError);
    RETURN_STATUS_IF_FALSE(JSObjectIsConstructor(env->context, objectRef), NAPIFunctionExpected);

    *result = (NAPIValue)JSObjectCallAsConstructor(env->context, objectRef, argc, (const JSValueRef *)argv,
                                                   &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return NAPIOK;
}

NAPIStatus napi_instanceof(NAPIEnv env, NAPIValue object, NAPIValue constructor, bool *result)
{
    CHECK_JSC(env);
    CHECK_ARG(object);
    CHECK_ARG(constructor);
    CHECK_ARG(result);

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)constructor), NAPIObjectExpected);
    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)constructor, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(objectRef, NAPIMemoryError);
    RETURN_STATUS_IF_FALSE(JSObjectIsConstructor(env->context, objectRef), NAPIFunctionExpected);

    *result = JSValueIsInstanceOfConstructor(env->context, (JSValueRef)object, objectRef, &env->lastException);
    CHECK_JSC(env);

    return NAPIOK;
}

NAPIStatus napi_get_cb_info(NAPIEnv env, NAPICallbackInfo callbackInfo, size_t *argc, NAPIValue *argv,
                            NAPIValue *thisArg, void **data)
{
    CHECK_ARG(env);
    CHECK_ARG(callbackInfo);

    if (argv)
    {
        CHECK_ARG(argc);

        size_t i = 0;
        size_t min = *argc > callbackInfo->argc ? callbackInfo->argc : *argc;

        for (; i < min; i++)
        {
            argv[i] = (NAPIValue)callbackInfo->argv[i];
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
        *argc = callbackInfo->argc;
    }

    if (thisArg)
    {
        *thisArg = (NAPIValue)callbackInfo->thisArg;
    }

    if (data)
    {
        *data = callbackInfo->data;
    }

    return NAPIOK;
}

NAPIStatus napi_get_new_target(NAPIEnv env, NAPICallbackInfo callbackInfo, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(callbackInfo);
    CHECK_ARG(result);

    *result = (NAPIValue)callbackInfo->newTarget;

    return NAPIOK;
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
    if (info && info->classRef)
    {
        // JSClassRelease 不能传递 NULL
        JSClassRelease(info->classRef);
    }
    free(info);
}

static void externalFinalize(JSObjectRef object)
{
    ExternalInfo *info = JSObjectGetPrivate(object);
    if (info && info->finalizeCallback)
    {
        info->finalizeCallback(info->baseInfo.env, info->baseInfo.data, info->finalizeHint);
    }
    free(info);
}

// static inline JSClassRef createExternalClass()
//{
//     JSClassDefinition classDefinition = kJSClassDefinitionEmpty;
//     classDefinition.attributes = kJSClassAttributeNoAutomaticPrototype;
//     classDefinition.className = "External";
//     classDefinition.finalize = externalFinalize;
//     JSClassRef classRef = JSClassCreate(&classDefinition);
//
//     return classRef;
// }

// static inline bool createExternalInfo(NAPIEnv env, void *data, ExternalInfo **result)
//{
//     if (!result)
//     {
//         return false;
//     }
//     ExternalInfo *externalInfo = malloc(sizeof(ExternalInfo));
//     if (!externalInfo)
//     {
//         return false;
//     }
//     externalInfo->env = env;
//     externalInfo->data = data;
//     SLIST_INIT(&externalInfo->finalizerHead);
//     externalInfo->finalizeCallback = NULL;
//     externalInfo->finalizeHint = NULL;
//     *result = externalInfo;
//
//     return true;
// }

static const char *const CONSTRUCTOR_STRING = "__constructor__";

static JSObjectRef callAsConstructor(JSContextRef ctx, JSObjectRef constructor, size_t argumentCount,
                                     const JSValueRef arguments[], JSValueRef *exception)
{
    // constructor 为当初定义的 JSObjectRef，而不是子类的构造函数
    JSStringRef keyStringRef = JSStringCreateWithUTF8CString(CONSTRUCTOR_STRING);
    JSValueRef prototype = JSObjectGetProperty(ctx, constructor, keyStringRef, exception);
    JSStringRelease(keyStringRef);
    if (*exception || !prototype)
    {
        return NULL;
    }
    if (!JSValueIsObject(ctx, prototype))
    {
        // 正常不应当出现
        assert(false);

        return NULL;
    }
    JSObjectRef prototypeObjectRef = JSValueToObject(ctx, prototype, exception);
    if (*exception || !prototypeObjectRef)
    {
        // 正常不应当出现
        assert(false);

        return NULL;
    }
    ConstructorInfo *constructorInfo = JSObjectGetPrivate(prototypeObjectRef);
    if (!constructorInfo || !constructorInfo->functionInfo.baseInfo.env || !constructorInfo->functionInfo.callback)
    {
        // 正常不应当出现
        assert(false);

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

    struct OpaqueNAPICallbackInfo callbackInfo = {NULL, NULL, NULL, NULL, 0};
    callbackInfo.newTarget = constructor;
    callbackInfo.thisArg = instance;
    callbackInfo.argc = argumentCount;
    callbackInfo.argv = arguments;
    callbackInfo.data = constructorInfo->functionInfo.baseInfo.data;

    JSValueRef returnValue =
        (JSValueRef)constructorInfo->functionInfo.callback(constructorInfo->functionInfo.baseInfo.env, &callbackInfo);
    if (constructorInfo->functionInfo.baseInfo.env->lastException)
    {
        *exception = constructorInfo->functionInfo.baseInfo.env->lastException;
        constructorInfo->functionInfo.baseInfo.env->lastException = NULL;

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

NAPIStatus NAPIDefineClass(NAPIEnv env, const char *utf8name, size_t length, NAPICallback constructor, void *data,
                           NAPIValue *result)
{
    CHECK_JSC(env);
    CHECK_ARG(constructor);
    CHECK_ARG(result);

    RETURN_STATUS_IF_FALSE(length == NAPI_AUTO_LENGTH, NAPIInvalidArg);

    ConstructorInfo *constructorInfo = malloc(sizeof(ConstructorInfo));
    RETURN_STATUS_IF_FALSE(constructorInfo, NAPIMemoryError);
    constructorInfo->functionInfo.baseInfo.env = env;
    constructorInfo->functionInfo.baseInfo.data = data;
    constructorInfo->functionInfo.callback = constructor;
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

        return NAPIMemoryError;
    }
    classDefinition = kJSClassDefinitionEmpty;
    // 使用 Object.prototype 作为 instance.[[prototype]]
    classDefinition.attributes = kJSClassAttributeNoAutomaticPrototype;
    classDefinition.className = NULL;
    // 最重要的是提供 finalize
    classDefinition.finalize = constructorFinalize;
    JSClassRef classRef = JSClassCreate(&classDefinition);
    if (!classRef)
    {
        JSClassRelease(constructorInfo->classRef);
        free(constructorInfo);

        return NAPIMemoryError;
    }
    JSObjectRef prototype = JSObjectMake(env->context, classRef, constructorInfo);
    JSClassRelease(classRef);
    if (!prototype)
    {
        JSClassRelease(constructorInfo->classRef);
        free(constructorInfo);

        return NAPIMemoryError;
    }
    JSObjectRef function = JSObjectMakeConstructor(env->context, constructorInfo->classRef, callAsConstructor);
    RETURN_STATUS_IF_FALSE(function, NAPIMemoryError);
    JSStringRef stringRef = JSStringCreateWithUTF8CString(CONSTRUCTOR_STRING);
    RETURN_STATUS_IF_FALSE(stringRef, NAPIMemoryError);
    JSObjectSetProperty(env->context, function, stringRef, prototype,
                        kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontEnum | kJSPropertyAttributeDontDelete,
                        &env->lastException);
    CHECK_JSC(env);
    *result = (NAPIValue)function;

    return NAPIOK;
}

NAPIStatus napi_create_external(NAPIEnv env, void *data, NAPIFinalize finalizeCB, void *finalizeHint, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    ExternalInfo *externalInfo = malloc(sizeof(ExternalInfo));
    RETURN_STATUS_IF_FALSE(externalInfo, NAPIMemoryError);
    externalInfo->baseInfo.env = env;
    externalInfo->baseInfo.data = data;
    externalInfo->finalizeCallback = finalizeCB;
    externalInfo->finalizeHint = finalizeHint;
    JSClassDefinition classDefinition = kJSClassDefinitionEmpty;
    classDefinition.className = "External";
    classDefinition.attributes = kJSClassAttributeNoAutomaticPrototype;
    classDefinition.finalize = externalFinalize;
    JSClassRef classRef = JSClassCreate(&classDefinition);
    if (!classRef)
    {
        free(externalInfo);

        return NAPIMemoryError;
    }
    JSObjectRef objectRef = JSObjectMake(env->context, classRef, externalInfo);
    JSClassRelease(classRef);
    if (!objectRef)
    {
        free(externalInfo);

        return NAPIMemoryError;
    }
    *result = (NAPIValue)objectRef;

    return NAPIOK;
}

NAPIStatus napi_get_value_external(NAPIEnv env, NAPIValue value, void **result)
{
    CHECK_JSC(env);
    CHECK_ARG(value);
    CHECK_ARG(result);

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)value), NAPIObjectExpected);
    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)value, &env->lastException);
    CHECK_JSC(env);
    RETURN_STATUS_IF_FALSE(objectRef, NAPIMemoryError);

    ExternalInfo *info = JSObjectGetPrivate(objectRef);
    *result = info ? info->baseInfo.data : NULL;

    return NAPIOK;
}

static const char *const REFERENCE_STRING = "__reference__";

typedef struct
{
    LIST_HEAD(, OpaqueNAPIRef) referenceList;
} ReferenceInfo;

static void referenceFinalize(NAPIEnv env, void *finalizeData, __attribute((unused)) void *finalizeHint)
{
    if (!finalizeData || !env || !env->context)
    {
        assert(false);

        return;
    }
    ReferenceInfo *referenceInfo = finalizeData;
    NAPIRef reference;
    LIST_FOREACH(reference, &referenceInfo->referenceList, node)
    {
        assert(!reference->count);
        reference->value = JSValueMakeUndefined(env->context);
    }
    free(referenceInfo);
}

static NAPIStatus setWeak(NAPIEnv env, NAPIValue value, NAPIRef ref)
{
    CHECK_ARG(env);
    CHECK_ARG(value);
    CHECK_ARG(ref);

    NAPIValue stringValue;
    CHECK_NAPI(napi_create_string_utf8(env, REFERENCE_STRING, -1, &stringValue));
    NAPIValue referenceValue;
    CHECK_NAPI(napi_get_property(env, value, stringValue, &referenceValue));
    NAPIValueType valueType;
    CHECK_NAPI(napi_typeof(env, referenceValue, &valueType));
    RETURN_STATUS_IF_FALSE(valueType == NAPIUndefined || valueType == NAPIExternal, NAPIGenericFailure);
    ReferenceInfo *referenceInfo;
    if (valueType == NAPIUndefined)
    {
        referenceInfo = malloc(sizeof(ReferenceInfo));
        RETURN_STATUS_IF_FALSE(referenceInfo, NAPIMemoryError);
        LIST_INIT(&referenceInfo->referenceList);
        {
            NAPIStatus status = napi_create_external(env, referenceInfo, referenceFinalize, NULL, &referenceValue);
            if (status != NAPIOK)
            {
                free(referenceInfo);

                return status;
            }
        }
        CHECK_NAPI(napi_set_property(env, value, stringValue, referenceValue));
    }
    else
    {
        CHECK_NAPI(napi_get_value_external(env, referenceValue, (void **)&referenceInfo));
        if (!referenceInfo)
        {
            assert(false);

            return NAPIGenericFailure;
        }
    }
    LIST_INSERT_HEAD(&referenceInfo->referenceList, ref, node);

    return NAPIOK;
}

NAPIStatus napi_create_reference(NAPIEnv env, NAPIValue value, uint32_t initialRefCount, NAPIRef *result)
{
    CHECK_ARG(env);
    CHECK_ARG(value);
    CHECK_ARG(result);

    *result = malloc(sizeof(struct OpaqueNAPIRef));
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    // 标量 && 弱引用
    if (!JSValueIsObject(env->context, (JSValueRef)value) && !initialRefCount)
    {
        (*result)->count = 0;
        (*result)->value = JSValueMakeUndefined(env->context);

        return NAPIOK;
    }
    // 对象 || 强引用
    (*result)->value = (JSValueRef)value;
    (*result)->count = initialRefCount;
    // 强引用
    if (initialRefCount)
    {
        JSValueProtect(env->context, (JSValueRef)value);

        return NAPIOK;
    }
    // 对象 && 弱引用
    // setWeak
    NAPIStatus status = setWeak(env, value, *result);
    if (status != NAPIOK)
    {
        free(*result);

        return status;
    }

    return NAPIOK;
}

static NAPIStatus clearWeak(NAPIEnv env, NAPIRef ref)
{
    CHECK_ARG(env);
    CHECK_ARG(ref);

    NAPIValue stringValue;
    CHECK_NAPI(napi_create_string_utf8(env, REFERENCE_STRING, -1, &stringValue));
    NAPIValue externalValue;
    CHECK_NAPI(napi_get_property(env, (NAPIValue)&ref->value, stringValue, &externalValue));
    ReferenceInfo *referenceInfo;
    CHECK_NAPI(napi_get_value_external(env, externalValue, (void **)&referenceInfo));
    if (!referenceInfo)
    {
        assert(false);

        return NAPIGenericFailure;
    }
    if (!LIST_EMPTY(&referenceInfo->referenceList) && LIST_FIRST(&referenceInfo->referenceList) == ref &&
        !LIST_NEXT(ref, node))
    {
        bool deleteResult;
        CHECK_NAPI(napi_delete_property(env, (NAPIValue)&ref->value, stringValue, &deleteResult));
        RETURN_STATUS_IF_FALSE(deleteResult, NAPIGenericFailure);
    }
    LIST_REMOVE(ref, node);

    return NAPIOK;
}

NAPIStatus napi_delete_reference(NAPIEnv env, NAPIRef ref)
{
    CHECK_ARG(env);
    CHECK_ARG(ref);

    // 标量 && 弱引用（被 GC 也会这样）
    if (!JSValueIsObject(env->context, ref->value) && !ref->count)
    {
        free(ref);

        return NAPIOK;
    }
    // 对象 || 强引用
    if (ref->count)
    {
        JSValueUnprotect(env->context, ref->value);
        free(ref);

        return NAPIOK;
    }
    // 对象 && 弱引用
    CHECK_NAPI(clearWeak(env, ref));
    free(ref);

    return NAPIOK;
}

NAPIStatus napi_reference_ref(NAPIEnv env, NAPIRef ref, uint32_t *result)
{
    CHECK_ARG(env);
    CHECK_ARG(ref);

    if (!ref->count)
    {
        if (JSValueIsObject(env->context, ref->value))
        {
            CHECK_NAPI(clearWeak(env, ref));
        }
        JSValueProtect(env->context, ref->value);
    }
    uint8_t count = ++ref->count;
    if (result)
    {
        *result = count;
    }

    return NAPIOK;
}

NAPIStatus napi_reference_unref(NAPIEnv env, NAPIRef ref, uint32_t *result)
{
    CHECK_ARG(env);
    CHECK_ARG(ref);

    CHECK_ARG(env->context);

    RETURN_STATUS_IF_FALSE(ref->count, NAPIGenericFailure);

    if (ref->count == 1)
    {
        if (JSValueIsObject(env->context, ref->value))
        {
            CHECK_NAPI(setWeak(env, (NAPIValue)&ref->value, ref));
            JSValueUnprotect(env->context, ref->value);
        }
        else
        {
            JSValueUnprotect(env->context, ref->value);
            ref->value = JSValueMakeUndefined(env->context);
        }
    }
    uint8_t count = --ref->count;
    if (result)
    {
        *result = count;
    }

    return NAPIOK;
}

NAPIStatus napi_get_reference_value(NAPIEnv env, NAPIRef ref, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(ref);
    CHECK_ARG(result);

    if (!ref->count && JSValueIsUndefined(env->context, ref->value))
    {
        *result = NULL;
    }
    else
    {
        *result = (NAPIValue)&ref->value;
    }

    return NAPIOK;
}

NAPIStatus napi_open_handle_scope(NAPIEnv env, NAPIHandleScope *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    *result = (NAPIHandleScope)1;

    return NAPIOK;
}

NAPIStatus napi_close_handle_scope(NAPIEnv env, NAPIHandleScope scope)
{
    CHECK_ARG(env);
    CHECK_ARG(scope);

    return NAPIOK;
}

struct OpaqueNAPIEscapableHandleScope
{
    bool escapeCalled;
};

NAPIStatus napi_open_escapable_handle_scope(NAPIEnv env, NAPIEscapableHandleScope *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    *result = malloc(sizeof(struct OpaqueNAPIEscapableHandleScope));
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);
    (*result)->escapeCalled = false;

    return NAPIOK;
}

NAPIStatus napi_close_escapable_handle_scope(NAPIEnv env, NAPIEscapableHandleScope scope)
{
    CHECK_ARG(env);
    CHECK_ARG(scope);

    free(scope);

    return NAPIOK;
}

NAPIStatus napi_escape_handle(NAPIEnv env, NAPIEscapableHandleScope scope, NAPIValue escapee, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(scope);
    CHECK_ARG(escapee);
    CHECK_ARG(result);

    RETURN_STATUS_IF_FALSE(!scope->escapeCalled, NAPIEscapeCalledTwice);
    scope->escapeCalled = true;
    *result = escapee;

    return NAPIOK;
}

NAPIStatus napi_throw(NAPIEnv env, NAPIValue error)
{
    CHECK_ARG(env);
    CHECK_ARG(error);

    env->lastException = (JSValueRef)error;

    return NAPIOK;
}

NAPIStatus napi_get_and_clear_last_exception(NAPIEnv env, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    if (!env->lastException)
    {
        return napi_get_undefined(env, result);
    }
    else
    {
        *result = (NAPIValue)env->lastException;
        env->lastException = NULL;
    }

    return NAPIOK;
}

NAPIStatus NAPIRunScript(NAPIEnv env, const char *utf8Script, const char *utf8SourceUrl, NAPIValue *result)
{
    CHECK_JSC(env);
    CHECK_ARG(utf8Script);

    JSStringRef scriptStringRef = JSStringCreateWithUTF8CString(utf8Script);
    JSStringRef sourceUrl = NULL;
    if (utf8SourceUrl)
    {
        sourceUrl = JSStringCreateWithUTF8CString(utf8SourceUrl);
    }

    JSValueRef valueRef = JSEvaluateScript(env->context, scriptStringRef, NULL, sourceUrl, 1, &env->lastException);
    CHECK_JSC(env);
    if (result)
    {
        *result = (NAPIValue)valueRef;
    }

    return NAPIOK;
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

    if ((virtualMachine && !contextCount) || (!virtualMachine && contextCount))
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
    CHECK_ARG(env);

    JSGlobalContextRelease(env->context);
    if (--contextCount == 0 && virtualMachine)
    {
        // virtualMachine 不能为 NULL
        JSContextGroupRelease(virtualMachine);
        virtualMachine = NULL;
    }

    free(env);

    return NAPIOK;
}

NAPIStatus NAPIGetValueStringUTF8(NAPIEnv env, NAPIValue value, const char **result)
{
    CHECK_ARG(env);
    CHECK_ARG(value);
    CHECK_ARG(result);

    RETURN_STATUS_IF_FALSE(JSValueIsString(env->context, (JSValueRef)value), NAPIStringExpected);
    JSStringRef stringRef = JSValueToStringCopy(env->context, (JSValueRef)value, &env->lastException);
    CHECK_JSC(env);
    // 不为 0
    size_t length = JSStringGetMaximumUTF8CStringSize(stringRef);
    char *string = malloc(sizeof(char) * length);
    RETURN_STATUS_IF_FALSE(string, NAPIMemoryError);
    if (!JSStringGetUTF8CString(stringRef, string, length))
    {
        free(string);

        return NAPIGenericFailure;
    }
    *result = string;

    return NAPIOK;
}

NAPIStatus NAPIFreeUTF8String(NAPIEnv env, const char *cString)
{
    CHECK_ARG(env);
    CHECK_ARG(cString);

    free((void *)cString);

    return NAPIOK;
}
