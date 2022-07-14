#include <napi/js_native_api.h>
#include <stdbool.h>
#include <sys/queue.h>

// setLastErrorCode 会处理 env == NULL 问题
#define RETURN_STATUS_IF_FALSE(condition, status)                                                                      \
    if (!(condition))                                                                                                  \
    {                                                                                                                  \
        return status;                                                                                                 \
    }

#define CHECK_ARG(arg, status)                                                                                         \
    if (!arg)                                                                                                          \
    {                                                                                                                  \
        return NAPI##status##InvalidArg;                                                                               \
    }

#define CHECK_NAPI(expr, exprStatus, returnStatus)                                                                     \
    {                                                                                                                  \
        NAPI##exprStatus##Status status = expr;                                                                        \
        if (status != NAPI##exprStatus##OK)                                                                            \
        {                                                                                                              \
            return (NAPI##returnStatus##Status)status;                                                                 \
        }                                                                                                              \
    }

#define CHECK_JSC(env)                                                                                                 \
    CHECK_ARG(env, Exception)                                                                                          \
    RETURN_STATUS_IF_FALSE(!((env)->lastException), NAPIExceptionPendingException)

#include <JavaScriptCore/JavaScriptCore.h>
#include <assert.h>
#include <napi/js_native_api_debugger.h>
#include <napi/js_native_api_types.h>
#include <stdio.h>
#include <stdlib.h>

struct OpaqueNAPIRef
{
    LIST_ENTRY(OpaqueNAPIRef) node; // size_t
    JSValueRef value;               // size_t
    uint8_t count;                  // 8
};

struct ReferenceInfo
{
    LIST_ENTRY(ReferenceInfo) node;
    LIST_HEAD(, OpaqueNAPIRef) referenceList;
    bool isEnvFreed;
};

// undefined 和 null 实际上也可以当做 exception
// 抛出，所以异常检查只需要检查是否为 C NULL
struct OpaqueNAPIEnv
{
    JSGlobalContextRef context; // size_t
    JSValueRef lastException;   // size_t
    LIST_HEAD(, ReferenceInfo) referenceList;
    LIST_HEAD(, OpaqueNAPIRef) strongRefList;
    LIST_HEAD(, OpaqueNAPIRef) valueList;
};

// NAPIMemoryError
NAPICommonStatus napi_get_undefined(NAPIEnv env, NAPIValue *result)
{
    CHECK_ARG(env, Common)
    CHECK_ARG(result, Common)

    *result = (NAPIValue)JSValueMakeUndefined(env->context);

    return NAPICommonOK;
}

// NAPIMemoryError
NAPICommonStatus napi_get_null(NAPIEnv env, NAPIValue *result)
{
    CHECK_ARG(env, Common)
    CHECK_ARG(result, Common)

    *result = (NAPIValue)JSValueMakeNull(env->context);

    return NAPICommonOK;
}

// NAPIMemoryError
NAPIErrorStatus napi_get_global(NAPIEnv env, NAPIValue *result)
{
    CHECK_ARG(env, Error)
    CHECK_ARG(result, Error)

    *result = (NAPIValue)JSContextGetGlobalObject(env->context);
    RETURN_STATUS_IF_FALSE(*result, NAPIErrorMemoryError);

    return NAPIErrorOK;
}

// NAPIMemoryError
NAPIErrorStatus napi_get_boolean(NAPIEnv env, bool value, NAPIValue *result)
{
    CHECK_ARG(env, Error)
    CHECK_ARG(result, Error)

    *result = (NAPIValue)JSValueMakeBoolean(env->context, value);
    RETURN_STATUS_IF_FALSE(*result, NAPIErrorMemoryError)

    return NAPIErrorOK;
}

// NAPIMemoryError
NAPIErrorStatus napi_create_double(NAPIEnv env, double value, NAPIValue *result)
{
    CHECK_ARG(env, Error)
    CHECK_ARG(result, Error)

    *result = (NAPIValue)JSValueMakeNumber(env->context, value);
    RETURN_STATUS_IF_FALSE(*result, NAPIErrorMemoryError)

    return NAPIErrorOK;
}

// JavaScriptCore 只能接受 \0 结尾的字符串
// 传入 str NULL 则为 ""
// V8 引擎传入 NULL 直接崩溃
// NAPIMemoryError
NAPIExceptionStatus napi_create_string_utf8(NAPIEnv env, const char *str, NAPIValue *result)
{
    CHECK_ARG(env, Exception)
    CHECK_ARG(result, Exception)

    // 传入 NULL，触发 OpaqueJSString()
    JSStringRef stringRef = JSStringCreateWithUTF8CString(str);
    // stringRef == NULL，会调用 String()
    *result = (NAPIValue)JSValueMakeString(env->context, stringRef);
    if (stringRef)
    {
        JSStringRelease(stringRef);
    }
    RETURN_STATUS_IF_FALSE(*result, NAPIExceptionMemoryError)

    return NAPIExceptionOK;
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
    void *data;                    // size_t
                                   //    BaseInfo baseInfo;
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
NAPIExceptionStatus napi_create_function(NAPIEnv env, const char *utf8name, NAPICallback cb, void *data,
                                         NAPIValue *result)
{
    CHECK_JSC(env)
    CHECK_ARG(cb, Exception)
    CHECK_ARG(result, Exception)

    FunctionInfo *functionInfo = malloc(sizeof(FunctionInfo));
    RETURN_STATUS_IF_FALSE(functionInfo, NAPIExceptionMemoryError)
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

        return NAPIExceptionMemoryError;
    }
    JSObjectRef prototype = JSObjectMake(env->context, classRef, functionInfo);
    JSClassRelease(classRef);
    if (!prototype)
    {
        free(functionInfo);

        return NAPIExceptionMemoryError;
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
    RETURN_STATUS_IF_FALSE(functionObjectRef, NAPIExceptionMemoryError)

    *result = (NAPIValue)functionObjectRef;
    stringRef = JSStringCreateWithUTF8CString(FUNCTION_STRING);
    RETURN_STATUS_IF_FALSE(stringRef, NAPIExceptionMemoryError)
    JSObjectSetProperty(env->context, functionObjectRef, stringRef, prototype,
                        kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontEnum | kJSPropertyAttributeDontDelete,
                        &env->lastException);
    JSStringRelease(stringRef);
    CHECK_JSC(env)

    return NAPIExceptionOK;
}

// NAPIPendingException/NAPIMemoryError
NAPICommonStatus napi_typeof(NAPIEnv env, NAPIValue value, NAPIValueType *result)
{
    CHECK_ARG(env, Common)
    CHECK_ARG(value, Common)
    CHECK_ARG(result, Common)

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
        JSValueRef exception = NULL;
        JSObjectRef object = JSValueToObject(env->context, (JSValueRef)value, &exception);
        RETURN_STATUS_IF_FALSE(!exception && object, NAPICommonInvalidArg)
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

        return NAPICommonInvalidArg;
    }

    return NAPICommonOK;
}

// NAPINumberExpected/NAPIPendingException
NAPIErrorStatus napi_get_value_double(NAPIEnv env, NAPIValue value, double *result)
{
    CHECK_ARG(env, Error)
    CHECK_ARG(value, Error)
    CHECK_ARG(result, Error)

    RETURN_STATUS_IF_FALSE(JSValueIsNumber(env->context, (JSValueRef)value), NAPIErrorNumberExpected)

    JSValueRef exception = NULL;
    *result = JSValueToNumber(env->context, (JSValueRef)value, &exception);
    RETURN_STATUS_IF_FALSE(!exception, NAPIErrorNumberExpected)

    return NAPIErrorOK;
}

// NAPIBooleanExpected
NAPIErrorStatus napi_get_value_bool(NAPIEnv env, NAPIValue value, bool *result)
{
    CHECK_ARG(env, Error)
    CHECK_ARG(value, Error)
    CHECK_ARG(result, Error)

    RETURN_STATUS_IF_FALSE(JSValueIsBoolean(env->context, (JSValueRef)value), NAPIErrorBooleanExpected)
    *result = JSValueToBoolean(env->context, (JSValueRef)value);

    return NAPIErrorOK;
}

// NAPIMemoryError
NAPIExceptionStatus napi_coerce_to_bool(NAPIEnv env, NAPIValue value, NAPIValue *result)
{
    CHECK_ARG(env, Exception)
    CHECK_ARG(value, Exception)
    CHECK_ARG(result, Exception)

    *result = (NAPIValue)JSValueMakeBoolean(env->context, JSValueToBoolean(env->context, (JSValueRef)value));
    RETURN_STATUS_IF_FALSE(*result, NAPIExceptionMemoryError)

    return NAPIExceptionOK;
}

// NAPIMemoryError
NAPIExceptionStatus napi_coerce_to_number(NAPIEnv env, NAPIValue value, NAPIValue *result)
{
    CHECK_JSC(env)
    CHECK_ARG(value, Exception)
    CHECK_ARG(result, Exception)

    double doubleValue = JSValueToNumber(env->context, (JSValueRef)value, &env->lastException);
    CHECK_JSC(env)
    *result = (NAPIValue)JSValueMakeNumber(env->context, doubleValue);
    RETURN_STATUS_IF_FALSE(*result, NAPIExceptionMemoryError)

    return NAPIExceptionOK;
}

// NAPIMemoryError
NAPIExceptionStatus napi_coerce_to_string(NAPIEnv env, NAPIValue value, NAPIValue *result)
{
    CHECK_JSC(env)
    CHECK_ARG(value, Exception)
    CHECK_ARG(result, Exception)

    JSStringRef stringRef = JSValueToStringCopy(env->context, (JSValueRef)value, &env->lastException);
    CHECK_JSC(env)
    RETURN_STATUS_IF_FALSE(stringRef, NAPIExceptionMemoryError)
    *result = (NAPIValue)JSValueMakeString(env->context, stringRef);
    JSStringRelease(stringRef);
    RETURN_STATUS_IF_FALSE(*result, NAPIExceptionMemoryError)

    return NAPIExceptionOK;
}

NAPIExceptionStatus napi_set_property(NAPIEnv env, NAPIValue object, NAPIValue key, NAPIValue value)
{
    CHECK_JSC(env)
    CHECK_ARG(object, Exception)
    CHECK_ARG(key, Exception)
    CHECK_ARG(value, Exception)

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)object), NAPIExceptionObjectExpected)
    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)object, &env->lastException);
    CHECK_JSC(env)
    RETURN_STATUS_IF_FALSE(objectRef, NAPIExceptionMemoryError)

    if (JSValueIsString(env->context, (JSValueRef)key))
    {
        JSStringRef stringRef = JSValueToStringCopy(env->context, (JSValueRef)key, &env->lastException);
        CHECK_JSC(env)
        RETURN_STATUS_IF_FALSE(stringRef, NAPIExceptionMemoryError)
        JSObjectSetProperty(env->context, objectRef, stringRef, (JSValueRef)value, kJSPropertyAttributeNone,
                            &env->lastException);
        JSStringRelease(stringRef);
    }
    else if (JSValueIsNumber(env->context, (JSValueRef)key))
    {
        double doubleValue;
        CHECK_NAPI(napi_get_value_double(env, key, &doubleValue), Error, Exception)
        JSObjectSetPropertyAtIndex(env->context, objectRef, (unsigned int)doubleValue, (JSValueRef)value,
                                   &env->lastException);
    }
    else
    {
        return NAPIExceptionNameExpected;
    }
    CHECK_JSC(env)

    return NAPIExceptionOK;
}

NAPIExceptionStatus napi_has_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result)
{
    CHECK_JSC(env)
    CHECK_ARG(object, Exception)
    CHECK_ARG(key, Exception)
    CHECK_ARG(result, Exception)

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)object), NAPIExceptionObjectExpected)
    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)object, &env->lastException);
    CHECK_JSC(env)
    RETURN_STATUS_IF_FALSE(objectRef, NAPIExceptionMemoryError)

    if (JSValueIsString(env->context, (JSValueRef)key))
    {
        JSStringRef stringRef = JSValueToStringCopy(env->context, (JSValueRef)key, &env->lastException);
        RETURN_STATUS_IF_FALSE(stringRef, NAPIExceptionMemoryError)
        *result = JSObjectHasProperty(env->context, objectRef, stringRef);
        JSStringRelease(stringRef);
    }
    else if (JSValueIsNumber(env->context, (JSValueRef)key))
    {
        double doubleValue;
        CHECK_NAPI(napi_get_value_double(env, key, &doubleValue), Error, Exception)
        *result = JSObjectGetPropertyAtIndex(env->context, objectRef, (unsigned int)doubleValue, &env->lastException);
        CHECK_JSC(env)
    }
    else
    {
        return NAPIExceptionNameExpected;
    }

    return NAPIExceptionOK;
}

NAPIExceptionStatus napi_get_property(NAPIEnv env, NAPIValue object, NAPIValue key, NAPIValue *result)
{
    CHECK_JSC(env)
    CHECK_ARG(object, Exception)
    CHECK_ARG(key, Exception)
    CHECK_ARG(result, Exception)

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)object), NAPIExceptionObjectExpected)
    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)object, &env->lastException);
    CHECK_JSC(env)
    RETURN_STATUS_IF_FALSE(objectRef, NAPIExceptionMemoryError)

    if (JSValueIsString(env->context, (JSValueRef)key))
    {
        JSStringRef stringRef = JSValueToStringCopy(env->context, (JSValueRef)key, &env->lastException);
        RETURN_STATUS_IF_FALSE(stringRef, NAPIExceptionMemoryError)
        *result = (NAPIValue)JSObjectGetProperty(env->context, objectRef, stringRef, &env->lastException);
        JSStringRelease(stringRef);
    }
    else if (JSValueIsNumber(env->context, (JSValueRef)key))
    {
        double doubleValue;
        CHECK_NAPI(napi_get_value_double(env, key, &doubleValue), Error, Exception)
        *result = (NAPIValue)JSObjectGetPropertyAtIndex(env->context, objectRef, (unsigned int)doubleValue,
                                                        &env->lastException);
    }
    else
    {
        return NAPIExceptionNameExpected;
    }

    return NAPIExceptionOK;
}

// key 必须为字符串
NAPIExceptionStatus napi_delete_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result)
{
    CHECK_JSC(env)
    CHECK_ARG(object, Exception)
    CHECK_ARG(key, Exception)

    RETURN_STATUS_IF_FALSE(JSValueIsString(env->context, (JSValueRef)key), NAPIExceptionStringExpected)
    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)object), NAPIExceptionObjectExpected)

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)object, &env->lastException);
    CHECK_JSC(env)
    RETURN_STATUS_IF_FALSE(objectRef, NAPIExceptionMemoryError)

    JSStringRef stringRef = JSValueToStringCopy(env->context, (JSValueRef)key, &env->lastException);
    CHECK_JSC(env)
    RETURN_STATUS_IF_FALSE(stringRef, NAPIExceptionMemoryError)

    bool deleteSuccess = JSObjectDeleteProperty(env->context, objectRef, stringRef, &env->lastException);
    JSStringRelease(stringRef);
    CHECK_JSC(env)
    if (result)
    {
        *result = deleteSuccess;
    }

    return NAPIExceptionOK;
}

NAPICommonStatus napi_is_array(NAPIEnv env, NAPIValue value, bool *result)
{
    CHECK_ARG(env, Common)
    CHECK_ARG(value, Common)
    CHECK_ARG(result, Common)

    *result = JSValueIsArray(env->context, (JSValueRef)value);

    return NAPICommonOK;
}

NAPIExceptionStatus napi_call_function(NAPIEnv env, NAPIValue thisValue, NAPIValue func, size_t argc,
                                       const NAPIValue *argv, NAPIValue *result)
{
    CHECK_JSC(env)
    CHECK_ARG(func, Exception)
    if (argc > 0)
    {
        CHECK_ARG(argv, Exception)
    }

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)func), NAPIExceptionObjectExpected)

    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)func, &env->lastException);
    RETURN_STATUS_IF_FALSE(objectRef, NAPIExceptionMemoryError)
    RETURN_STATUS_IF_FALSE(JSObjectIsFunction(env->context, objectRef), NAPIExceptionFunctionExpected)

    JSObjectRef thisObjectRef = NULL;
    if (!thisValue)
    {
        thisObjectRef = JSContextGetGlobalObject(env->context);
    }
    else
    {
        RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)thisValue), NAPIExceptionObjectExpected)
        thisObjectRef = JSValueToObject(env->context, (JSValueRef)thisValue, &env->lastException);
        CHECK_JSC(env)
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
        RETURN_STATUS_IF_FALSE(argumentArray, NAPIExceptionMemoryError)
        for (size_t i = 0; i < argc; ++i)
        {
            argumentArray[i] = (JSValueRef)argv[i];
        }
        returnValue =
            JSObjectCallAsFunction(env->context, objectRef, thisObjectRef, argc, argumentArray, &env->lastException);
        free(argumentArray);
    }
    CHECK_JSC(env)

    if (result)
    {
        RETURN_STATUS_IF_FALSE(returnValue, NAPIExceptionMemoryError)
        *result = (NAPIValue)returnValue;
    }

    return NAPIExceptionOK;
}

NAPIExceptionStatus napi_new_instance(NAPIEnv env, NAPIValue constructor, size_t argc, const NAPIValue *argv,
                                      NAPIValue *result)
{
    CHECK_JSC(env)
    CHECK_ARG(constructor, Exception)
    if (argc > 0)
    {
        CHECK_ARG(argv, Exception)
    }
    CHECK_ARG(result, Exception)

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)constructor), NAPIExceptionObjectExpected)
    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)constructor, &env->lastException);
    CHECK_JSC(env)
    RETURN_STATUS_IF_FALSE(objectRef, NAPIExceptionMemoryError)
    RETURN_STATUS_IF_FALSE(JSObjectIsConstructor(env->context, objectRef), NAPIExceptionFunctionExpected)

    *result = (NAPIValue)JSObjectCallAsConstructor(env->context, objectRef, argc, (const JSValueRef *)argv,
                                                   &env->lastException);
    CHECK_JSC(env)
    RETURN_STATUS_IF_FALSE(*result, NAPIExceptionMemoryError)

    return NAPIExceptionOK;
}

NAPIExceptionStatus napi_instanceof(NAPIEnv env, NAPIValue object, NAPIValue constructor, bool *result)
{
    CHECK_JSC(env)
    CHECK_ARG(object, Exception)
    CHECK_ARG(constructor, Exception)
    CHECK_ARG(result, Exception)

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)constructor), NAPIExceptionObjectExpected)
    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)constructor, &env->lastException);
    CHECK_JSC(env)
    RETURN_STATUS_IF_FALSE(objectRef, NAPIExceptionMemoryError)
    RETURN_STATUS_IF_FALSE(JSObjectIsConstructor(env->context, objectRef), NAPIExceptionFunctionExpected)

    *result = JSValueIsInstanceOfConstructor(env->context, (JSValueRef)object, objectRef, &env->lastException);
    CHECK_JSC(env)

    return NAPIExceptionOK;
}

NAPICommonStatus napi_get_cb_info(NAPIEnv env, NAPICallbackInfo callbackInfo, size_t *argc, NAPIValue *argv,
                                  NAPIValue *thisArg, void **data)
{
    CHECK_ARG(env, Common)
    CHECK_ARG(callbackInfo, Common)

    if (argv)
    {
        CHECK_ARG(argc, Common)

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

    return NAPICommonOK;
}

NAPICommonStatus napi_get_new_target(NAPIEnv env, NAPICallbackInfo callbackInfo, NAPIValue *result)
{
    CHECK_ARG(env, Common)
    CHECK_ARG(callbackInfo, Common)
    CHECK_ARG(result, Common)

    *result = (NAPIValue)callbackInfo->newTarget;

    return NAPICommonOK;
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
        info->finalizeCallback(info->data, info->finalizeHint);
    }
    free(info);
}

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

    if (!returnValue || !JSValueIsObject(ctx, returnValue))
    {
        return instance;
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

// 没有 .name 和 .prototype.constructor
NAPIExceptionStatus NAPIDefineClass(NAPIEnv env, const char *utf8name, NAPICallback constructor, void *data,
                                    NAPIValue *result)
{
    CHECK_JSC(env)
    CHECK_ARG(constructor, Exception)
    CHECK_ARG(result, Exception)

    ConstructorInfo *constructorInfo = malloc(sizeof(ConstructorInfo));
    RETURN_STATUS_IF_FALSE(constructorInfo, NAPIExceptionMemoryError)
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

        return NAPIExceptionMemoryError;
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

        return NAPIExceptionMemoryError;
    }
    JSObjectRef prototype = JSObjectMake(env->context, classRef, constructorInfo);
    JSClassRelease(classRef);
    if (!prototype)
    {
        JSClassRelease(constructorInfo->classRef);
        free(constructorInfo);

        return NAPIExceptionMemoryError;
    }
    JSObjectRef function = JSObjectMakeConstructor(env->context, constructorInfo->classRef, callAsConstructor);
    RETURN_STATUS_IF_FALSE(function, NAPIExceptionMemoryError)
    JSStringRef stringRef = JSStringCreateWithUTF8CString(CONSTRUCTOR_STRING);
    RETURN_STATUS_IF_FALSE(stringRef, NAPIExceptionMemoryError)
    JSObjectSetProperty(env->context, function, stringRef, prototype,
                        kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontEnum | kJSPropertyAttributeDontDelete,
                        &env->lastException);
    CHECK_JSC(env)
    *result = (NAPIValue)function;

    return NAPIExceptionOK;
}

NAPIExceptionStatus napi_create_external(NAPIEnv env, void *data, NAPIFinalize finalizeCB, void *finalizeHint,
                                         NAPIValue *result)
{
    CHECK_ARG(env, Exception)
    CHECK_ARG(result, Exception)

    ExternalInfo *externalInfo = malloc(sizeof(ExternalInfo));
    RETURN_STATUS_IF_FALSE(externalInfo, NAPIExceptionMemoryError)
    externalInfo->data = data;
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

        return NAPIExceptionMemoryError;
    }
    JSObjectRef objectRef = JSObjectMake(env->context, classRef, externalInfo);
    JSClassRelease(classRef);
    if (!objectRef)
    {
        free(externalInfo);

        return NAPIExceptionMemoryError;
    }
    *result = (NAPIValue)objectRef;

    return NAPIExceptionOK;
}

NAPIErrorStatus napi_get_value_external(NAPIEnv env, NAPIValue value, void **result)
{
    CHECK_ARG(env, Error)
    CHECK_ARG(value, Error)
    CHECK_ARG(result, Error)

    RETURN_STATUS_IF_FALSE(JSValueIsObject(env->context, (JSValueRef)value), NAPIErrorExternalExpected)
    JSValueRef exception = NULL;
    JSObjectRef objectRef = JSValueToObject(env->context, (JSValueRef)value, &exception);
    RETURN_STATUS_IF_FALSE(!exception && objectRef, NAPIErrorExternalExpected)

    ExternalInfo *info = JSObjectGetPrivate(objectRef);
    *result = info ? info->data : NULL;

    return NAPIErrorOK;
}

static const char *const REFERENCE_STRING = "__reference__";

static void referenceFinalize(void *finalizeData, void *finalizeHint)
{
    if (!finalizeData)
    {
        assert(false);

        return;
    }
    struct ReferenceInfo *referenceInfo = finalizeData;
    if (!referenceInfo->isEnvFreed)
    {
        NAPIRef reference;
        LIST_FOREACH(reference, &referenceInfo->referenceList, node)
        {
            assert(!reference->count);
            reference->value = JSValueMakeUndefined(((NAPIEnv)finalizeHint)->context);
            LIST_REMOVE(reference, node);
            LIST_INSERT_HEAD(&((NAPIEnv)finalizeHint)->valueList, reference, node);
        }
        LIST_REMOVE(referenceInfo, node);
    }
    free(referenceInfo);
}

static NAPIExceptionStatus setWeak(NAPIEnv env, NAPIValue value, NAPIRef ref)
{
    CHECK_ARG(env, Exception)
    CHECK_ARG(value, Exception)
    CHECK_ARG(ref, Exception)

    NAPIValue stringValue;
    CHECK_NAPI(napi_create_string_utf8(env, REFERENCE_STRING, &stringValue), Exception, Exception)
    NAPIValue referenceValue;
    CHECK_NAPI(napi_get_property(env, value, stringValue, &referenceValue), Exception, Exception)
    NAPIValueType valueType;
    CHECK_NAPI(napi_typeof(env, referenceValue, &valueType), Common, Exception)
    RETURN_STATUS_IF_FALSE(valueType == NAPIUndefined || valueType == NAPIExternal, NAPIExceptionGenericFailure)
    struct ReferenceInfo *referenceInfo;
    if (valueType == NAPIUndefined)
    {
        referenceInfo = malloc(sizeof(struct ReferenceInfo));
        referenceInfo->isEnvFreed = false;
        RETURN_STATUS_IF_FALSE(referenceInfo, NAPIExceptionMemoryError)
        LIST_INIT(&referenceInfo->referenceList);
        {
            NAPIExceptionStatus status =
                napi_create_external(env, referenceInfo, referenceFinalize, env, &referenceValue);
            if (status != NAPIExceptionOK)
            {
                free(referenceInfo);

                return status;
            }
        }
        CHECK_NAPI(napi_set_property(env, value, stringValue, referenceValue), Exception, Exception)
        LIST_INSERT_HEAD(&env->referenceList, referenceInfo, node);
    }
    else
    {
        CHECK_NAPI(napi_get_value_external(env, referenceValue, (void **)&referenceInfo), Error, Exception)
        if (!referenceInfo)
        {
            assert(false);

            return NAPIExceptionGenericFailure;
        }
    }
    LIST_INSERT_HEAD(&referenceInfo->referenceList, ref, node);

    return NAPIExceptionOK;
}

NAPIExceptionStatus napi_create_reference(NAPIEnv env, NAPIValue value, uint32_t initialRefCount, NAPIRef *result)
{
    CHECK_ARG(env, Exception)
    CHECK_ARG(value, Exception)
    CHECK_ARG(result, Exception)

    *result = malloc(sizeof(struct OpaqueNAPIRef));
    RETURN_STATUS_IF_FALSE(*result, NAPIExceptionMemoryError)

    // 标量 && 弱引用
    if (!JSValueIsObject(env->context, (JSValueRef)value) && !initialRefCount)
    {
        (*result)->count = 0;
        (*result)->value = JSValueMakeUndefined(env->context);
        LIST_INSERT_HEAD(&env->valueList, *result, node);

        return NAPIExceptionOK;
    }
    // 对象 || 强引用
    (*result)->value = (JSValueRef)value;
    (*result)->count = initialRefCount;
    // 强引用
    if (initialRefCount)
    {
        JSValueProtect(env->context, (JSValueRef)value);
        LIST_INSERT_HEAD(&env->strongRefList, *result, node);

        return NAPIExceptionOK;
    }
    // 对象 && 弱引用
    // setWeak
    NAPIExceptionStatus status = setWeak(env, value, *result);
    if (status != NAPIExceptionOK)
    {
        free(*result);

        return status;
    }

    return NAPIExceptionOK;
}

static NAPIExceptionStatus clearWeak(NAPIEnv env, NAPIRef ref)
{
    CHECK_ARG(env, Exception)
    CHECK_ARG(ref, Exception)

    NAPIValue stringValue;
    CHECK_NAPI(napi_create_string_utf8(env, REFERENCE_STRING, &stringValue), Exception, Exception)
    NAPIValue externalValue;
    CHECK_NAPI(napi_get_property(env, (NAPIValue)ref->value, stringValue, &externalValue), Exception, Exception)
    struct ReferenceInfo *referenceInfo;
    CHECK_NAPI(napi_get_value_external(env, externalValue, (void **)&referenceInfo), Error, Exception)
    if (!referenceInfo)
    {
        assert(false);

        return NAPIExceptionGenericFailure;
    }
    if (!LIST_EMPTY(&referenceInfo->referenceList) && LIST_FIRST(&referenceInfo->referenceList) == ref &&
        !LIST_NEXT(ref, node))
    {
        bool deleteResult;
        CHECK_NAPI(napi_delete_property(env, (NAPIValue)ref->value, stringValue, &deleteResult), Exception, Exception)
        RETURN_STATUS_IF_FALSE(deleteResult, NAPIExceptionGenericFailure)
    }
    LIST_REMOVE(ref, node);

    return NAPIExceptionOK;
}

NAPIExceptionStatus napi_delete_reference(NAPIEnv env, NAPIRef ref)
{
    CHECK_ARG(env, Exception)
    CHECK_ARG(ref, Exception)

    // 标量 && 弱引用（被 GC 也会这样）
    if (!JSValueIsObject(env->context, ref->value) && !ref->count)
    {
        LIST_REMOVE(ref, node);
        free(ref);

        return NAPIExceptionOK;
    }
    // 对象 || 强引用
    if (ref->count)
    {
        LIST_REMOVE(ref, node);
        JSValueUnprotect(env->context, ref->value);
        free(ref);

        return NAPIExceptionOK;
    }
    // 对象 && 弱引用
    CHECK_NAPI(clearWeak(env, ref), Exception, Exception)
    free(ref);

    return NAPIExceptionOK;
}

NAPIExceptionStatus napi_reference_ref(NAPIEnv env, NAPIRef ref, uint32_t *result)
{
    CHECK_ARG(env, Exception)
    CHECK_ARG(ref, Exception)

    if (!ref->count)
    {
        if (JSValueIsObject(env->context, ref->value))
        {
            CHECK_NAPI(clearWeak(env, ref), Exception, Exception)
        }
        else
        {
            LIST_REMOVE(ref, node);
        }
        LIST_INSERT_HEAD(&env->strongRefList, ref, node);
        JSValueProtect(env->context, ref->value);
    }
    uint8_t count = ++ref->count;
    if (result)
    {
        *result = count;
    }

    return NAPIExceptionOK;
}

NAPIExceptionStatus napi_reference_unref(NAPIEnv env, NAPIRef ref, uint32_t *result)
{
    CHECK_ARG(env, Exception)
    CHECK_ARG(ref, Exception)

    RETURN_STATUS_IF_FALSE(ref->count, NAPIExceptionGenericFailure)

    if (ref->count == 1)
    {
        LIST_REMOVE(ref, node);
        if (JSValueIsObject(env->context, ref->value))
        {
            CHECK_NAPI(setWeak(env, (NAPIValue)ref->value, ref), Exception, Exception)
            JSValueUnprotect(env->context, ref->value);
        }
        else
        {
            LIST_INSERT_HEAD(&env->valueList, ref, node);
            JSValueUnprotect(env->context, ref->value);
            ref->value = JSValueMakeUndefined(env->context);
        }
    }
    uint8_t count = --ref->count;
    if (result)
    {
        *result = count;
    }

    return NAPIExceptionOK;
}

NAPIErrorStatus napi_get_reference_value(NAPIEnv env, NAPIRef ref, NAPIValue *result)
{
    CHECK_ARG(env, Error)
    CHECK_ARG(ref, Error)
    CHECK_ARG(result, Error)

    if (!ref->count && JSValueIsUndefined(env->context, ref->value))
    {
        *result = NULL;
    }
    else
    {
        *result = (NAPIValue)ref->value;
    }

    return NAPIErrorOK;
}

NAPIErrorStatus napi_open_handle_scope(NAPIEnv env, NAPIHandleScope *result)
{
    CHECK_ARG(env, Error)
    CHECK_ARG(result, Error)

    *result = (NAPIHandleScope)1;

    return NAPIErrorOK;
}

NAPICommonStatus napi_close_handle_scope(__attribute__((unused)) NAPIEnv env,
                                         __attribute__((unused)) NAPIHandleScope scope)
{
    return NAPICommonOK;
}

struct OpaqueNAPIEscapableHandleScope
{
    bool escapeCalled;
};

NAPIErrorStatus napi_open_escapable_handle_scope(NAPIEnv env, NAPIEscapableHandleScope *result)
{
    CHECK_ARG(env, Error)
    CHECK_ARG(result, Error)

    *result = malloc(sizeof(struct OpaqueNAPIEscapableHandleScope));
    RETURN_STATUS_IF_FALSE(*result, NAPIErrorMemoryError)
    (*result)->escapeCalled = false;

    return NAPIErrorOK;
}

NAPICommonStatus napi_close_escapable_handle_scope(__attribute__((unused)) NAPIEnv env, NAPIEscapableHandleScope scope)
{
    CHECK_ARG(scope, Common)

    free(scope);

    return NAPICommonOK;
}

NAPIErrorStatus napi_escape_handle(NAPIEnv env, NAPIEscapableHandleScope scope, NAPIValue escapee, NAPIValue *result)
{
    CHECK_ARG(env, Error)
    CHECK_ARG(scope, Error)
    CHECK_ARG(escapee, Error)
    CHECK_ARG(result, Error)

    RETURN_STATUS_IF_FALSE(!scope->escapeCalled, NAPIErrorEscapeCalledTwice)
    scope->escapeCalled = true;
    *result = escapee;

    return NAPIErrorOK;
}

NAPIExceptionStatus napi_throw(NAPIEnv env, NAPIValue error)
{
    CHECK_ARG(env, Exception)
    CHECK_ARG(error, Exception)

    env->lastException = (JSValueRef)error;

    return NAPIExceptionOK;
}

NAPIErrorStatus napi_get_and_clear_last_exception(NAPIEnv env, NAPIValue *result)
{
    CHECK_ARG(env, Error)
    CHECK_ARG(result, Error)

    if (!env->lastException)
    {
        return (NAPIErrorStatus)napi_get_undefined(env, result);
    }
    else
    {
        *result = (NAPIValue)env->lastException;
        env->lastException = NULL;
    }

    return NAPIErrorOK;
}

NAPICommonStatus NAPIClearLastException(NAPIEnv env)
{
    CHECK_ARG(env, Common)

    env->lastException = NULL;

    return NAPICommonOK;
}

NAPIExceptionStatus NAPIRunScript(NAPIEnv env, const char *utf8Script, const char *utf8SourceUrl, NAPIValue *result)
{
    CHECK_JSC(env)

    JSStringRef scriptStringRef = JSStringCreateWithUTF8CString(utf8Script);
    JSStringRef sourceUrl = NULL;
    if (utf8SourceUrl)
    {
        sourceUrl = JSStringCreateWithUTF8CString(utf8SourceUrl);
    }
    // JSEvaluateScript 要求 scriptStringRef 必定存在
    JSValueRef valueRef = JSEvaluateScript(env->context, scriptStringRef, NULL, sourceUrl, 1, &env->lastException);
    JSStringRelease(scriptStringRef);
    if (utf8SourceUrl)
    {
        JSStringRelease(sourceUrl);
    }
    CHECK_JSC(env)
    if (result)
    {
        *result = (NAPIValue)valueRef;
    }

    return NAPIExceptionOK;
}

static JSContextGroupRef virtualMachine = NULL;

static uint8_t contextCount = 0;

NAPICommonStatus NAPIEnableDebugger(__attribute__((unused)) NAPIEnv env,
                                    __attribute__((unused)) const char *debuggerTitle,
                                    __attribute__((unused)) bool waitForDebugger)
{
    return NAPICommonOK;
}

NAPICommonStatus NAPIDisableDebugger(__attribute__((unused)) NAPIEnv env)
{
    return NAPICommonOK;
}

NAPICommonStatus NAPISetMessageQueueThread(__attribute__((unused)) NAPIEnv env,
                                           __attribute__((unused)) MessageQueueThreadWrapper jsQueueWrapper)
{
    return NAPICommonOK;
}

NAPIErrorStatus NAPICreateEnv(NAPIEnv *env)
{
    // *env 才是 NAPIEnv
    if (!env)
    {
        return NAPIErrorInvalidArg;
    }

    if ((virtualMachine && !contextCount) || (!virtualMachine && contextCount))
    {
        assert(false);

        return NAPIErrorGenericFailure;
    }

    *env = malloc(sizeof(struct OpaqueNAPIEnv));
    if (!*env)
    {
        return NAPIErrorMemoryError;
    }

    if (!virtualMachine)
    {
        virtualMachine = JSContextGroupCreate();
        if (!virtualMachine)
        {
            free(*env);

            return NAPIErrorMemoryError;
        }
    }
    JSGlobalContextRef globalContext = JSGlobalContextCreateInGroup(virtualMachine, NULL);
    if (!globalContext)
    {
        free(*env);
        JSContextGroupRelease(virtualMachine);
        virtualMachine = NULL;

        return NAPIErrorMemoryError;
    }
    contextCount += 1;
    (*env)->context = globalContext;
    (*env)->lastException = NULL;
    LIST_INIT(&(*env)->strongRefList);
    LIST_INIT(&(*env)->valueList);
    LIST_INIT(&(*env)->referenceList);

    return NAPIErrorOK;
}

NAPICommonStatus NAPIFreeEnv(NAPIEnv env)
{
    CHECK_ARG(env, Common)

    NAPIRef ref, temp;
    LIST_FOREACH_SAFE(ref, &env->strongRefList, node, temp)
    {
        LIST_REMOVE(ref, node);
        JSValueUnprotect(env->context, ref->value);
        free(ref);
    }
    struct ReferenceInfo *referenceInfo, *tempReferenceInfo;
    LIST_FOREACH_SAFE(referenceInfo, &env->referenceList, node, tempReferenceInfo)
    {
        LIST_FOREACH_SAFE(ref, &referenceInfo->referenceList, node, temp)
        {
            LIST_REMOVE(ref, node);
            free(ref);
        }
        LIST_REMOVE(referenceInfo, node);
        referenceInfo->isEnvFreed = true;
    }
    LIST_FOREACH_SAFE(ref, &env->valueList, node, temp)
    {
        LIST_REMOVE(ref, node);
        free(ref);
    }
    JSGlobalContextRelease(env->context);
    if (--contextCount == 0 && virtualMachine)
    {
        // virtualMachine 不能为 NULL
        JSContextGroupRelease(virtualMachine);
        virtualMachine = NULL;
    }

    free(env);

    return NAPICommonOK;
}

NAPIErrorStatus NAPIGetValueStringUTF8(NAPIEnv env, NAPIValue value, const char **result)
{
    CHECK_ARG(env, Error)
    CHECK_ARG(value, Error)
    CHECK_ARG(result, Error)

    RETURN_STATUS_IF_FALSE(JSValueIsString(env->context, (JSValueRef)value), NAPIErrorStringExpected)
    JSValueRef exception = NULL;
    JSStringRef stringRef = JSValueToStringCopy(env->context, (JSValueRef)value, &exception);
    RETURN_STATUS_IF_FALSE(!exception && stringRef, NAPIErrorStringExpected)
    // 不为 0
    size_t length = JSStringGetMaximumUTF8CStringSize(stringRef);
    char *string = malloc(sizeof(char) * length);
    RETURN_STATUS_IF_FALSE(string, NAPIErrorMemoryError)
    if (!JSStringGetUTF8CString(stringRef, string, length))
    {
        free(string);

        return NAPIErrorGenericFailure;
    }
    *result = string;

    return NAPIErrorOK;
}

NAPICommonStatus NAPIFreeUTF8String(NAPIEnv env, const char *cString)
{
    CHECK_ARG(env, Common)
    CHECK_ARG(cString, Common)

    free((void *)cString);

    return NAPICommonOK;
}

NAPI_EXPORT NAPIExceptionStatus NAPICompileToByteBuffer(__attribute__((unused)) NAPIEnv env,
                                                        __attribute__((unused)) const char *script,
                                                        __attribute__((unused)) const char *sourceUrl,
                                                        __attribute__((unused)) const uint8_t **byteBuffer,
                                                        __attribute__((unused)) size_t *bufferSize)
{
    return NAPIExceptionOK;
}

NAPI_EXPORT NAPICommonStatus NAPIFreeByteBuffer(__attribute__((unused)) NAPIEnv env,
                                                __attribute__((unused)) const uint8_t *byteBuffer)
{
    return NAPICommonOK;
}

NAPI_EXPORT NAPIExceptionStatus NAPIRunByteBuffer(__attribute__((unused)) NAPIEnv env,
                                                  __attribute__((unused)) const uint8_t *byteBuffer,
                                                  __attribute__((unused)) size_t bufferSize,
                                                  __attribute__((unused)) NAPIValue *result)
{
    return NAPIExceptionOK;
}
