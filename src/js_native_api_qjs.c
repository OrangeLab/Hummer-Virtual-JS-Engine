// 所有函数都可能抛出 NAPIInvalidArg NAPIOK
// 当返回值为 NAPIOK 时候，必须保证 result 可用，但是发生错误的时候，result 也可能会被修改，所以建议不要对 result 做
// NULL 初始化

#include <assert.h>
#include <math.h>
#include <napi/js_native_api.h>
#include <quickjs.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#ifndef SLIST_FOREACH_SAFE
#define SLIST_FOREACH_SAFE(var, head, field, tvar)                                                                     \
    for ((var) = SLIST_FIRST((head)); (var) && ((tvar) = SLIST_NEXT((var), field), 1); (var) = (tvar))
#endif

#ifndef LIST_FOREACH_SAFE
#define LIST_FOREACH_SAFE(var, head, field, tvar)                                                                      \
    for ((var) = LIST_FIRST((head)); (var) && ((tvar) = LIST_NEXT((var), field), 1); (var) = (tvar))
#endif

#define RETURN_STATUS_IF_FALSE(condition, status)                                                                      \
    if (!(condition))                                                                                                  \
    {                                                                                                                  \
        return status;                                                                                                 \
    }

#define CHECK_NAPI(expr)                                                                                               \
    {                                                                                                                  \
        NAPIStatus status = expr;                                                                                      \
        if (status != NAPIOK)                                                                                          \
        {                                                                                                              \
            return status;                                                                                             \
        }                                                                                                              \
    }

#define CHECK_ARG(arg)                                                                                                 \
    if (!arg)                                                                                                          \
    {                                                                                                                  \
        assert(false);                                                                                                 \
    }

// JS_GetException 会转移所有权
// JS_Throw 会获取所有权
// 所以如果存在异常，先获取所有权，再交还所有权，最终所有权依旧在
// JSContext 中，QuickJS 目前 JS null 代表正常，未来考虑写成这样
// !JS_IsUndefined(exceptionValue) && !JS_IsNull(exceptionValue)
// NAPI_PREAMBLE 同时会检查 env->context
#define NAPI_PREAMBLE(env)                                                                                             \
    {                                                                                                                  \
        CHECK_ARG(env);                                                                                                \
        CHECK_ARG((env)->context);                                                                                     \
        JSValue exceptionValue = JS_GetException((env)->context);                                                      \
        if (!JS_IsNull(exceptionValue))                                                                                \
        {                                                                                                              \
            JS_Throw((env)->context, exceptionValue);                                                                  \
                                                                                                                       \
            return NAPIPendingException;                                                                               \
        }                                                                                                              \
    }

struct Handle
{
    JSValue value;            // 64/128
    SLIST_ENTRY(Handle) node; // size_t
};

struct OpaqueNAPIHandleScope
{
    LIST_ENTRY(OpaqueNAPIHandleScope) node; // size_t
    SLIST_HEAD(, Handle) handleList;        // size_t
};

struct OpaqueNAPIRef
{
    JSValue value;                  // 64/128
    LIST_ENTRY(OpaqueNAPIRef) node; // size_t
    uint8_t referenceCount;         // 8
};

struct ReferenceInfo
{
    LIST_ENTRY(ReferenceInfo) node;
    LIST_HEAD(, OpaqueNAPIRef) referenceList;
    bool isEnvFreed;
};

// 1. external -> 不透明指针 + finalizer + 调用一个回调
// 2. Function -> JSValue data 数组 + finalizer
// 3. Constructor -> .[[prototype]] = new External()
// 4. Reference -> 引用计数 + setWeak/clearWeak -> referenceSymbolValue

// typedef struct
// {
//     const char *errorMessage; // size_t
//     NAPIStatus errorCode;     // enum，有需要的话根据 iOS 或 Android 的 clang 情况做单独处理
// } NAPIExtendedErrorInfo;
struct OpaqueNAPIEnv
{
    JSValue referenceSymbolValue;                       // 64/128
    JSContext *context;                                 // size_t
    LIST_HEAD(, OpaqueNAPIHandleScope) handleScopeList; // size_t
    bool isThrowNull;
    LIST_HEAD(, ReferenceInfo) referenceList;
    LIST_HEAD(, OpaqueNAPIRef) strongRefList;
    LIST_HEAD(, OpaqueNAPIRef) valueList;
};

// 这个函数不会修改引用计数和所有权
// NAPIHandleScopeEmpty/NAPIMemoryError
static NAPIStatus addValueToHandleScope(NAPIEnv env, JSValue value, struct Handle **result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    RETURN_STATUS_IF_FALSE(!LIST_EMPTY(&(*env).handleScopeList), NAPIHandleScopeEmpty);
    *result = malloc(sizeof(struct Handle));
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);
    (*result)->value = value;
    SLIST_INSERT_HEAD(&(LIST_FIRST(&(env)->handleScopeList))->handleList, *result, node);

    return NAPIOK;
}

static JSValueConst undefinedValue = JS_UNDEFINED;

NAPIStatus napi_get_undefined(NAPIEnv env, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    *result = (NAPIValue)&undefinedValue;

    return NAPIOK;
}

static JSValueConst nullValue = JS_NULL;

NAPIStatus napi_get_null(NAPIEnv env, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    *result = (NAPIValue)&nullValue;

    return NAPIOK;
}

// NAPIGenericFailure + addValueToHandleScope
NAPIStatus napi_get_global(NAPIEnv env, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    // QuickJS 要求传入 context 不能为 NULL
    CHECK_ARG(env->context);

    // JS_GetGlobalObject 返回已经引用计数 +1
    // 实际上 globalValue 可能为 JS_EXCEPTION，但是由于不是 JS_GetGlobalObject 中发生的异常，因此返回 generic failure
    JSValue globalValue = JS_GetGlobalObject(env->context);
    if (JS_IsException(globalValue))
    {
        assert(false);

        return NAPIGenericFailure;
    }
    struct Handle *globalHandle;
    NAPIStatus status = addValueToHandleScope(env, globalValue, &globalHandle);
    if (status != NAPIOK)
    {
        JS_FreeValue(env->context, globalValue);

        return status;
    }
    *result = (NAPIValue)&globalHandle->value;

    return NAPIOK;
}

// + addValueToHandleScope
NAPIStatus napi_get_boolean(NAPIEnv env, bool value, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    // JS_NewBool 忽略 JSContext
    JSValue jsValue = JS_NewBool(env->context, value);
    struct Handle *handle;
    CHECK_NAPI(addValueToHandleScope(env, jsValue, &handle));
    *result = (NAPIValue)&handle->value;

    return NAPIOK;
}

// + addValueToHandleScope
NAPIStatus napi_create_double(NAPIEnv env, double value, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    JSValue jsValue = JS_NewFloat64(env->context, value);
    struct Handle *handle;
    CHECK_NAPI(addValueToHandleScope(env, jsValue, &handle));
    *result = (NAPIValue)&handle->value;

    return NAPIOK;
}

// NAPIPendingException + addValueToHandleScope
NAPIStatus napi_create_string_utf8(NAPIEnv env, const char *str, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(result);

    size_t length;
    if (str)
    {
        length = strlen(str);
    }
    else
    {
        // str 依旧保持 NULL
        length = 0;
    }
    // length == 0 的情况下会返回 ""
    JSValue stringValue = JS_NewStringLen(env->context, str, length);
    RETURN_STATUS_IF_FALSE(!JS_IsException(stringValue), NAPIPendingException);
    struct Handle *stringHandle;
    NAPIStatus status = addValueToHandleScope(env, stringValue, &stringHandle);
    if (status != NAPIOK)
    {
        JS_FreeValue(env->context, stringValue);

        return status;
    }
    *result = (NAPIValue)&stringHandle->value;

    return NAPIOK;
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

// classId 必须为 0 才会被分配
static JSClassID functionClassId = 0;

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
    // JS_GetOpaque 不会抛出异常
    FunctionInfo *functionInfo = JS_GetOpaque(funcData[0], functionClassId);
    if (!functionInfo || !functionInfo->callback || !functionInfo->baseInfo.env)
    {
        assert(false);

        return undefinedValue;
    }
    // thisVal 有可能为 undefined，如果直接调用函数，比如 test() 而不是 this.test() 或者 globalThis.test()
    bool useGlobalValue = false;
    if (JS_IsUndefined(thisVal))
    {
        useGlobalValue = true;
        thisVal = JS_GetGlobalObject(ctx);
        if (JS_IsException(thisVal))
        {
            assert(false);

            return thisVal;
        }
    }
    struct OpaqueNAPICallbackInfo callbackInfo = {undefinedValue, thisVal, argv, functionInfo->baseInfo.data, argc};
    // napi_open_handle_scope 失败需要容错，这里需要初始化为 NULL 判断
    NAPIHandleScope handleScope = NULL;
    // 内存分配失败，由最外层做 HandleScope
    {
        NAPIStatus status = napi_open_handle_scope(functionInfo->baseInfo.env, &handleScope);
        if (status != NAPIOK)
        {
            if (useGlobalValue)
            {
                JS_FreeValue(ctx, thisVal);
            }

            return undefinedValue;
        }
    }
    // 正常返回值应当是由 HandleScope 持有，所以需要引用计数 +1
    NAPIValue retVal = functionInfo->callback(functionInfo->baseInfo.env, &callbackInfo);
    if (useGlobalValue)
    {
        JS_FreeValue(ctx, thisVal);
    }
    JSValue returnValue = undefinedValue;
    if (retVal)
    {
        returnValue = JS_DupValue(ctx, *((JSValue *)retVal));
    }
    JSValue exceptionValue = JS_GetException(ctx);
    if (handleScope)
    {
        napi_close_handle_scope(functionInfo->baseInfo.env, handleScope);
    }
    if (functionInfo->baseInfo.env->isThrowNull)
    {
        JS_FreeValue(ctx, returnValue);
        functionInfo->baseInfo.env->isThrowNull = false;

        return JS_EXCEPTION;
    }
    else if (!JS_IsNull(exceptionValue))
    {
        JS_FreeValue(ctx, returnValue);

        return JS_Throw(ctx, exceptionValue);
    }

    return returnValue;
}

// NAPIMemoryError/NAPIPendingException + addValueToHandleScope + napi_create_string_utf8
NAPIStatus napi_create_function(NAPIEnv env, const char *utf8name, NAPICallback cb, void *data, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(cb);
    CHECK_ARG(result);

    NAPIValue nameValue;
    CHECK_NAPI(napi_create_string_utf8(env, utf8name, &nameValue));

    // malloc
    FunctionInfo *functionInfo = malloc(sizeof(FunctionInfo));
    RETURN_STATUS_IF_FALSE(functionInfo, NAPIMemoryError);
    functionInfo->baseInfo.env = env;
    functionInfo->baseInfo.data = data;
    functionInfo->callback = cb;

    // rc: 1
    JSValue dataValue = JS_NewObjectClass(env->context, (int)functionClassId);
    if (JS_IsException(dataValue))
    {
        free(functionInfo);

        return NAPIPendingException;
    }
    // functionInfo 生命周期被 JSValue 托管
    JS_SetOpaque(dataValue, functionInfo);

    // fakePrototypeValue 引用计数 +1 => rc: 2
    JSValue functionValue = JS_NewCFunctionData(env->context, callAsFunction, 0, 0, 1, &dataValue);
    // 转移所有权
    JS_FreeValue(env->context, dataValue);
    // JS_DefinePropertyValueStr -> JS_DefinePropertyValue 转移所有权
    // ES6 和 ES5.1 不兼容的点：length -> configurable: true
    // JS_DefinePropertyValueStr -> JS_DefinePropertyValue 自动传入 JS_PROP_HAS_CONFIGURABLE
    JS_DefinePropertyValueStr(env->context, functionValue, "name", JS_DupValue(env->context, *((JSValue *)nameValue)),
                              JS_PROP_CONFIGURABLE);
    // 如果 functionValue 创建失败，fakePrototypeValue rc 不会被 +1，上面的引用计数 -1 => rc 为
    // 0，发生垃圾回收，FunctionInfo 结构体也被回收
    RETURN_STATUS_IF_FALSE(!JS_IsException(functionValue), NAPIPendingException);
    struct Handle *functionHandle;
    NAPIStatus status = addValueToHandleScope(env, functionValue, &functionHandle);
    if (status != NAPIOK)
    {
        // 由于 dataValue 所有权被 functionValue 持有，所以只需要对 functionValue 做引用计数 -1
        JS_FreeValue(env->context, functionValue);

        return status;
    }
    *result = (NAPIValue)&functionHandle->value;

    return NAPIOK;
}

static JSClassID externalClassId = 0;

NAPIStatus napi_typeof(NAPIEnv env, NAPIValue value, NAPIValueType *result)
{
    CHECK_ARG(env);
    CHECK_ARG(value);
    CHECK_ARG(result);

    CHECK_ARG(env->context);

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
        return NAPIInvalidArg;
    }

    return NAPIOK;
}

// NAPINumberExpected
NAPIStatus napi_get_value_double(NAPIEnv env, NAPIValue value, double *result)
{
    CHECK_ARG(env);
    CHECK_ARG(value);
    CHECK_ARG(result);

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
        return NAPINumberExpected;
    }

    return NAPIOK;
}

// NAPIBooleanExpected
NAPIStatus napi_get_value_bool(NAPIEnv env, NAPIValue value, bool *result)
{
    CHECK_ARG(env);
    CHECK_ARG(value);
    CHECK_ARG(result);

    RETURN_STATUS_IF_FALSE(JS_IsBool(*((JSValue *)value)), NAPIBooleanExpected);
    *result = JS_VALUE_GET_BOOL(*((JSValue *)value));

    return NAPIOK;
}

// NAPIPendingException + napi_get_boolean
NAPIStatus napi_coerce_to_bool(NAPIEnv env, NAPIValue value, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(value);
    CHECK_ARG(result);

    int boolStatus = JS_ToBool(env->context, *((JSValue *)value));
    RETURN_STATUS_IF_FALSE(boolStatus != -1, NAPIPendingException);
    CHECK_NAPI(napi_get_boolean(env, boolStatus, result));

    return NAPIOK;
}

// NAPIPendingException + napi_create_double
NAPIStatus napi_coerce_to_number(NAPIEnv env, NAPIValue value, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(value);
    CHECK_ARG(result);

    double doubleValue;
    // JS_ToFloat64 只有 -1 0 两种
    int floatStatus = JS_ToFloat64(env->context, &doubleValue, *((JSValue *)value));
    RETURN_STATUS_IF_FALSE(floatStatus != -1, NAPIPendingException);
    CHECK_NAPI(napi_create_double(env, doubleValue, result));

    return NAPIOK;
}

// NAPIPendingException + addValueToHandleScope
NAPIStatus napi_coerce_to_string(NAPIEnv env, NAPIValue value, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(value);
    CHECK_ARG(result);

    JSValue stringValue = JS_ToString(env->context, *((JSValue *)value));
    RETURN_STATUS_IF_FALSE(!JS_IsException(stringValue), NAPIPendingException);
    struct Handle *stringHandle;
    NAPIStatus status = addValueToHandleScope(env, stringValue, &stringHandle);
    if (status != NAPIOK)
    {
        JS_FreeValue(env->context, stringValue);

        return status;
    }
    *result = (NAPIValue)&stringHandle->value;

    return NAPIOK;
}

// NAPIPendingException/NAPIGenericFailure
NAPIStatus napi_set_property(NAPIEnv env, NAPIValue object, NAPIValue key, NAPIValue value)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(object);
    CHECK_ARG(key);
    CHECK_ARG(value);

    JSAtom atom = JS_ValueToAtom(env->context, *((JSValue *)key));
    RETURN_STATUS_IF_FALSE(atom != JS_ATOM_NULL, NAPIPendingException);
    // JS_SetProperty 转移所有权
    int status =
        JS_SetProperty(env->context, *((JSValue *)object), atom, JS_DupValue(env->context, *((JSValue *)value)));
    JS_FreeAtom(env->context, atom);
    RETURN_STATUS_IF_FALSE(status != -1, NAPIPendingException);
    RETURN_STATUS_IF_FALSE(status, NAPIGenericFailure);

    return NAPIOK;
}

// NAPIPendingException
NAPIStatus napi_has_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(object);
    CHECK_ARG(key);
    CHECK_ARG(result);

    JSAtom atom = JS_ValueToAtom(env->context, *((JSValue *)key));
    RETURN_STATUS_IF_FALSE(atom != JS_ATOM_NULL, NAPIPendingException);
    int status = JS_HasProperty(env->context, *((JSValue *)object), atom);
    JS_FreeAtom(env->context, atom);
    RETURN_STATUS_IF_FALSE(status != -1, NAPIPendingException);
    *result = status;

    return NAPIOK;
}

// NAPIPendingException + addValueToHandleScope
NAPIStatus napi_get_property(NAPIEnv env, NAPIValue object, NAPIValue key, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(object);
    CHECK_ARG(key);
    CHECK_ARG(result);

    JSAtom atom = JS_ValueToAtom(env->context, *((JSValue *)key));
    RETURN_STATUS_IF_FALSE(atom != JS_ATOM_NULL, NAPIPendingException);
    JSValue value = JS_GetProperty(env->context, *((JSValue *)object), atom);
    JS_FreeAtom(env->context, atom);
    RETURN_STATUS_IF_FALSE(!JS_IsException(value), NAPIPendingException);
    struct Handle *handle;
    NAPIStatus status = addValueToHandleScope(env, value, &handle);
    if (status != NAPIOK)
    {
        JS_FreeValue(env->context, value);

        return status;
    }
    *result = (NAPIValue)&handle->value;

    return NAPIOK;
}

// NAPIPendingException
NAPIStatus napi_delete_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(object);
    CHECK_ARG(key);

    JSAtom atom = JS_ValueToAtom(env->context, *((JSValue *)key));
    RETURN_STATUS_IF_FALSE(atom != JS_ATOM_NULL, NAPIPendingException);
    // 不抛出异常
    int status = JS_DeleteProperty(env->context, *((JSValue *)object), atom, 0);
    JS_FreeAtom(env->context, atom);
    RETURN_STATUS_IF_FALSE(status != -1, NAPIPendingException);
    if (result)
    {
        *result = status;
    }

    return NAPIOK;
}

NAPIStatus napi_is_array(NAPIEnv env, NAPIValue value, bool *result)
{
    CHECK_ARG(env);
    CHECK_ARG(value);
    CHECK_ARG(result);

    CHECK_ARG(env->context);

    *result = JS_IsArray(env->context, *((JSValue *)value));

    return NAPIOK;
}

static void processPendingTask(NAPIEnv env)
{
    if (!env)
    {
        return;
    }

    for (int err = 1; err > 0;)
    {
        JSContext *context;
        err = JS_ExecutePendingJob(JS_GetRuntime(env->context), &context);
        if (err == -1)
        {
            // 正常情况下 JS_ExecutePendingJob 返回 -1
            // 代表引擎内部异常，比如内存分配失败等
            JSValue inlineExceptionValue = JS_GetException(context);
            JS_FreeValue(context, inlineExceptionValue);
        }
    }
}

// NAPIMemoryError/NAPIPendingException + addValueToHandleScope
NAPIStatus napi_call_function(NAPIEnv env, NAPIValue thisValue, NAPIValue func, size_t argc, const NAPIValue *argv,
                              NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(func);

    if (!thisValue)
    {
        CHECK_NAPI(napi_get_global(env, &thisValue));
    }

    JSValue *internalArgv = NULL;
    if (argc > 0)
    {
        CHECK_ARG(argv);
        internalArgv = malloc(sizeof(JSValue) * argc);
        RETURN_STATUS_IF_FALSE(internalArgv, NAPIMemoryError);
        for (size_t i = 0; i < argc; ++i)
        {
            internalArgv[i] = *((JSValue *)argv[i]);
        }
    }

    // JS_Call 返回值带所有权
    JSValue returnValue = JS_Call(env->context, *((JSValue *)func), *((JSValue *)thisValue), (int)argc, internalArgv);
    free(internalArgv);
    if (JS_IsException(returnValue))
    {
        JSValue exceptionValue = JS_GetException(env->context);
        processPendingTask(env);
        if (JS_IsNull(exceptionValue))
        {
            env->isThrowNull = true;
        }
        JS_Throw(env->context, exceptionValue);

        return NAPIPendingException;
    }
    processPendingTask(env);
    if (result)
    {
        struct Handle *handle;
        NAPIStatus status = addValueToHandleScope(env, returnValue, &handle);
        if (status != NAPIOK)
        {
            JS_FreeValue(env->context, returnValue);

            return status;
        }
        *result = (NAPIValue)&handle->value;
    }
    else
    {
        JS_FreeValue(env->context, returnValue);
    }

    return NAPIOK;
}

// NAPIPendingException/NAPIMemoryError + addValueToHandleScope
NAPIStatus napi_new_instance(NAPIEnv env, NAPIValue constructor, size_t argc, const NAPIValue *argv, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(constructor);
    CHECK_ARG(result);

    JSValue *internalArgv = NULL;
    if (argc > 0)
    {
        CHECK_ARG(argv);
        internalArgv = malloc(sizeof(JSValue) * argc);
        RETURN_STATUS_IF_FALSE(internalArgv, NAPIMemoryError);
        for (size_t i = 0; i < argc; ++i)
        {
            internalArgv[i] = *((JSValue *)argv[i]);
        }
    }

    JSValue returnValue = JS_CallConstructor(env->context, *((JSValue *)constructor), (int)argc, internalArgv);
    free(internalArgv);
    if (JS_IsException(returnValue))
    {
        JSValue exceptionValue = JS_GetException(env->context);
        processPendingTask(env);
        if (JS_IsNull(exceptionValue))
        {
            env->isThrowNull = true;
        }
        JS_Throw(env->context, exceptionValue);

        return NAPIPendingException;
    }
    processPendingTask(env);
    struct Handle *handle;
    NAPIStatus status = addValueToHandleScope(env, returnValue, &handle);
    if (status != NAPIOK)
    {
        JS_FreeValue(env->context, returnValue);

        return status;
    }
    *result = (NAPIValue)&handle->value;

    return NAPIOK;
}

// NAPIPendingException
NAPIStatus napi_instanceof(NAPIEnv env, NAPIValue object, NAPIValue constructor, bool *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(object);
    CHECK_ARG(constructor);
    CHECK_ARG(result);

    int status = JS_IsInstanceOf(env->context, *((JSValue *)object), *((JSValue *)constructor));
    RETURN_STATUS_IF_FALSE(status != -1, NAPIPendingException);
    *result = status;

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
        size_t min = callbackInfo->argc < 0 || *argc > (size_t)callbackInfo->argc ? callbackInfo->argc : *argc;
        for (; i < min; ++i)
        {
            argv[i] = (NAPIValue)&callbackInfo->argv[i];
        }
        if (i < *argc)
        {
            for (; i < *argc; ++i)
            {
                argv[i] = (NAPIValue)&undefinedValue;
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

    return NAPIOK;
}

NAPIStatus napi_get_new_target(NAPIEnv env, NAPICallbackInfo callbackInfo, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(callbackInfo);
    CHECK_ARG(result);

    if (!JS_IsUndefined(callbackInfo->newTarget))
    {
        *result = (NAPIValue)&callbackInfo->newTarget;
    }
    else
    {
        *result = NULL;
    }

    return NAPIOK;
}

typedef struct
{
    //    BaseInfo baseInfo;
    void *data;                    // size_t
    void *finalizeHint;            // size_t
    NAPIFinalize finalizeCallback; // size_t
} ExternalInfo;

// NAPIMemoryError/NAPIPendingException + addValueToHandleScope
NAPIStatus napi_create_external(NAPIEnv env, void *data, NAPIFinalize finalizeCB, void *finalizeHint, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(result);

    ExternalInfo *externalInfo = malloc(sizeof(ExternalInfo));
    RETURN_STATUS_IF_FALSE(externalInfo, NAPIMemoryError);
    externalInfo->data = data;
    externalInfo->finalizeHint = finalizeHint;
    externalInfo->finalizeCallback = NULL;
    JSValue object = JS_NewObjectClass(env->context, (int)externalClassId);
    if (JS_IsException(object))
    {
        free(externalInfo);

        return NAPIPendingException;
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
    // 不能先设置回调，万一出错，业务方也会收到回调
    externalInfo->finalizeCallback = finalizeCB;

    return NAPIOK;
}

NAPIStatus napi_get_value_external(NAPIEnv env, NAPIValue value, void **result)
{
    CHECK_ARG(env);
    CHECK_ARG(value);
    CHECK_ARG(result);

    ExternalInfo *externalInfo = JS_GetOpaque(*((JSValue *)value), externalClassId);
    *result = externalInfo ? externalInfo->data : NULL;

    return NAPIOK;
}

static bool gcLock = false;

static void referenceFinalize(void *finalizeData, void *finalizeHint)
{
    assert(!gcLock);
    if (!finalizeData)
    {
        assert(false);

        return;
    }
    struct ReferenceInfo *referenceInfo = finalizeData;
    // 如果弱引用列表为空，则为 NAPIFreeEnv 调用后
    if (!referenceInfo->isEnvFreed)
    {
        if (!finalizeHint)
        {
            assert(false);

            return;
        }
        NAPIRef reference, temp;
        LIST_FOREACH_SAFE(reference, &referenceInfo->referenceList, node, temp)
        {
            assert(!reference->referenceCount);
            reference->value = undefinedValue;
            LIST_REMOVE(reference, node);
            LIST_INSERT_HEAD(&((NAPIEnv)finalizeHint)->valueList, reference, node);
        }
        LIST_REMOVE(referenceInfo, node);
    }
    free(referenceInfo);
}

// NAPINameExpected/NAPIMemoryError/NAPIGenericFailure + napi_create_string_utf8 + napi_get_property + napi_typeof +
// napi_create_external
// + napi_get_value_external + napi_set_property
static NAPIStatus setWeak(NAPIEnv env, NAPIValue value, NAPIRef ref)
{
    CHECK_ARG(env);
    CHECK_ARG(value);
    CHECK_ARG(ref);

    RETURN_STATUS_IF_FALSE(JS_IsSymbol(env->referenceSymbolValue), NAPINameExpected);

    NAPIValue referenceValue;
    CHECK_NAPI(napi_get_property(env, value, (NAPIValue)&env->referenceSymbolValue, &referenceValue));
    NAPIValueType valueType;
    CHECK_NAPI(napi_typeof(env, referenceValue, &valueType));
    RETURN_STATUS_IF_FALSE(valueType == NAPIUndefined || valueType == NAPIExternal, NAPIGenericFailure);
    struct ReferenceInfo *referenceInfo;
    if (valueType == NAPIUndefined)
    {
        referenceInfo = malloc(sizeof(struct ReferenceInfo));
        referenceInfo->isEnvFreed = false;
        RETURN_STATUS_IF_FALSE(referenceInfo, NAPIMemoryError);
        LIST_INIT(&referenceInfo->referenceList);
        {
            NAPIStatus status = napi_create_external(env, referenceInfo, referenceFinalize, env, &referenceValue);
            if (status != NAPIOK)
            {
                free(referenceInfo);

                return status;
            }
        }
        CHECK_NAPI(napi_set_property(env, value, (NAPIValue)&env->referenceSymbolValue, referenceValue));
        LIST_INSERT_HEAD(&env->referenceList, referenceInfo, node);
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

// NAPIMemoryError + setWeak
NAPIStatus napi_create_reference(NAPIEnv env, NAPIValue value, uint32_t initialRefCount, NAPIRef *result)
{
    CHECK_ARG(env);
    CHECK_ARG(value);
    CHECK_ARG(result);

    CHECK_ARG(env->context);

    *result = malloc(sizeof(struct OpaqueNAPIRef));
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);
    // 标量 && 弱引用
    if (!JS_IsObject(*((JSValue *)value)) && !initialRefCount)
    {
        (*result)->referenceCount = 0;
        (*result)->value = undefinedValue;
        LIST_INSERT_HEAD(&env->valueList, *result, node);

        return NAPIOK;
    }
    // 对象 || 强引用
    (*result)->value = *((JSValue *)value);
    (*result)->referenceCount = initialRefCount;
    // 强引用
    if (initialRefCount)
    {
        (*result)->value = JS_DupValue(env->context, (*result)->value);
        LIST_INSERT_HEAD(&env->strongRefList, *result, node);

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

// NAPINameExpected/NAPIGenericFailure + napi_create_string_utf8 + napi_get_property + napi_get_value_external +
// napi_delete_property
static NAPIStatus clearWeak(NAPIEnv env, NAPIRef ref)
{
    CHECK_ARG(env);
    CHECK_ARG(ref);

    RETURN_STATUS_IF_FALSE(JS_IsSymbol(env->referenceSymbolValue), NAPINameExpected);

    NAPIValue externalValue;
    CHECK_NAPI(napi_get_property(env, (NAPIValue)&ref->value, (NAPIValue)&env->referenceSymbolValue, &externalValue));
    struct ReferenceInfo *referenceInfo;
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
        CHECK_NAPI(
            napi_delete_property(env, (NAPIValue)&ref->value, (NAPIValue)&env->referenceSymbolValue, &deleteResult));
        RETURN_STATUS_IF_FALSE(deleteResult, NAPIGenericFailure);
    }
    LIST_REMOVE(ref, node);

    return NAPIOK;
}

// NAPIGenericFailure + clearWeak
NAPIStatus napi_delete_reference(NAPIEnv env, NAPIRef ref)
{
    CHECK_ARG(env);
    CHECK_ARG(ref);

    CHECK_ARG(env->context);

    // 标量 && 弱引用（被 GC 也会这样）
    if (!JS_IsObject(ref->value) && !ref->referenceCount)
    {
        LIST_REMOVE(ref, node);
        free(ref);

        return NAPIOK;
    }
    // 对象 || 强引用
    if (ref->referenceCount)
    {
        LIST_REMOVE(ref, node);
        JS_FreeValue(env->context, ref->value);
        free(ref);

        return NAPIOK;
    }
    // 对象 && 弱引用
    CHECK_NAPI(clearWeak(env, ref));
    free(ref);

    return NAPIOK;
}

// NAPIGenericFailure + clearWeak
NAPIStatus napi_reference_ref(NAPIEnv env, NAPIRef ref, uint32_t *result)
{
    CHECK_ARG(env);
    CHECK_ARG(ref);

    CHECK_ARG(env->context);

    // !ref->referenceCount && JS_IsUndefined(ref->value) 已经被 GC
    //    RETURN_STATUS_IF_FALSE(ref->referenceCount || !JS_IsUndefined(ref->value), NAPIGenericFailure);

    if (!ref->referenceCount)
    {
        if (JS_IsObject(ref->value))
        {
            CHECK_NAPI(clearWeak(env, ref));
        }
        else
        {
            LIST_REMOVE(ref, node);
        }
        LIST_INSERT_HEAD(&env->strongRefList, ref, node);
        ref->value = JS_DupValue(env->context, ref->value);
    }
    uint8_t count = ++ref->referenceCount;
    if (result)
    {
        *result = count;
    }

    return NAPIOK;
}

// NAPIGenericFailure + setWeak
NAPIStatus napi_reference_unref(NAPIEnv env, NAPIRef ref, uint32_t *result)
{
    CHECK_ARG(env);
    CHECK_ARG(ref);

    CHECK_ARG(env->context);

    RETURN_STATUS_IF_FALSE(ref->referenceCount, NAPIGenericFailure);

    if (ref->referenceCount == 1)
    {
        LIST_REMOVE(ref, node);
        if (JS_IsObject(ref->value))
        {
            CHECK_NAPI(setWeak(env, (NAPIValue)&ref->value, ref));
            JS_FreeValue(env->context, ref->value);
        }
        else
        {
            LIST_INSERT_HEAD(&env->valueList, ref, node);
            JS_FreeValue(env->context, ref->value);
            ref->value = undefinedValue;
        }
    }
    uint8_t count = --ref->referenceCount;
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

    if (!ref->referenceCount && JS_IsUndefined(ref->value))
    {
        *result = NULL;
    }
    else
    {
        *result = (NAPIValue)&ref->value;
    }

    return NAPIOK;
}

// NAPIMemoryError
NAPIStatus napi_open_handle_scope(NAPIEnv env, NAPIHandleScope *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    *result = malloc(sizeof(struct OpaqueNAPIHandleScope));
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);
    SLIST_INIT(&(*result)->handleList);
    LIST_INSERT_HEAD(&env->handleScopeList, *result, node);

    return NAPIOK;
}

void napi_close_handle_scope(NAPIEnv env, NAPIHandleScope scope)
{
    CHECK_ARG(env);
    CHECK_ARG(scope);

    // JS_FreeValue 不能传入 NULL
    CHECK_ARG(env->context);

    // 先入后出 stack 规则
    assert(LIST_FIRST(&env->handleScopeList) == scope);
    struct Handle *handle, *tempHandle;
    SLIST_FOREACH_SAFE(handle, &scope->handleList, node, tempHandle)
    {
        JS_FreeValue(env->context, handle->value);
        free(handle);
    }
    LIST_REMOVE(scope, node);
    free(scope);
}

struct OpaqueNAPIEscapableHandleScope
{
    struct OpaqueNAPIHandleScope handleScope;
    bool escapeCalled;
};

// NAPIMemoryError
NAPIStatus napi_open_escapable_handle_scope(NAPIEnv env, NAPIEscapableHandleScope *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    // 万一前面的 handleScope 被 close 了，会导致当前 EscapableHandleScope 变成最上层
    // handleScope，这里的判断就没有意义了
    //    RETURN_STATUS_IF_FALSE(LIST_FIRST(&env->handleScopeList), NAPIHandleScopeMismatch);
    *result = malloc(sizeof(struct OpaqueNAPIEscapableHandleScope));
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);
    (*result)->escapeCalled = false;
    SLIST_INIT(&(*result)->handleScope.handleList);
    LIST_INSERT_HEAD(&env->handleScopeList, &((*result)->handleScope), node);

    return NAPIOK;
}

void napi_close_escapable_handle_scope(NAPIEnv env, NAPIEscapableHandleScope scope)
{
    CHECK_ARG(env);
    CHECK_ARG(scope);

    // JS_FreeValue 不能传入 NULL
    CHECK_ARG(env->context);

    assert(LIST_FIRST(&env->handleScopeList) == &scope->handleScope);
    struct Handle *handle, *tempHandle;
    SLIST_FOREACH_SAFE(handle, &(&scope->handleScope)->handleList, node, tempHandle)
    {
        JS_FreeValue(env->context, handle->value);
        free(handle);
    }
    LIST_REMOVE(&scope->handleScope, node);
    free(scope);
}

// NAPIMemoryError/NAPIEscapeCalledTwice/NAPIHandleScopeEmpty
NAPIStatus napi_escape_handle(NAPIEnv env, NAPIEscapableHandleScope scope, NAPIValue escapee, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(scope);
    CHECK_ARG(escapee);
    CHECK_ARG(result);

    CHECK_ARG(env->context);

    RETURN_STATUS_IF_FALSE(!scope->escapeCalled, NAPIEscapeCalledTwice);

    NAPIHandleScope handleScope = LIST_NEXT(&scope->handleScope, node);
    RETURN_STATUS_IF_FALSE(handleScope, NAPIHandleScopeEmpty);
    struct Handle *handle = malloc(sizeof(struct Handle));
    RETURN_STATUS_IF_FALSE(handle, NAPIMemoryError);
    scope->escapeCalled = true;
    handle->value = JS_DupValue(env->context, *((JSValue *)escapee));
    SLIST_INSERT_HEAD(&handleScope->handleList, handle, node);
    *result = (NAPIValue)&handle->value;

    return NAPIOK;
}

NAPIStatus napi_throw(NAPIEnv env, NAPIValue error)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(error);

    CHECK_ARG(env->context);

    if (!JS_IsNull(*((JSValue *)error)))
    {
        JS_Throw(env->context, JS_DupValue(env->context, *((JSValue *)error)));
    }
    else
    {
        env->isThrowNull = true;
    }

    return NAPIOK;
}

// + addValueToHandleScope
NAPIStatus napi_get_and_clear_last_exception(NAPIEnv env, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    // JS_GetException
    CHECK_ARG(env->context);

    JSValue exceptionValue = JS_GetException(env->context);
    if (JS_IsNull(exceptionValue) && !env->isThrowNull)
    {
        exceptionValue = undefinedValue;
    }
    env->isThrowNull = false;
    struct Handle *exceptionHandle;
    NAPIStatus status = addValueToHandleScope(env, exceptionValue, &exceptionHandle);
    if (status != NAPIOK)
    {
        JS_FreeValue(env->context, exceptionValue);

        return status;
    }
    *result = (NAPIValue)&exceptionHandle->value;

    return NAPIOK;
}

// NAPIPendingException + addValueToHandleScope
NAPIStatus NAPIRunScript(NAPIEnv env, const char *script, const char *sourceUrl, NAPIValue *result)
{
    NAPI_PREAMBLE(env);

    // script 传入 NULL 会崩
    if (!script)
    {
        script = "";
    }
    // utf8SourceUrl 传入 NULL 会导致崩溃，JS_NewAtom 调用 strlen 不能传入 NULL
    if (!sourceUrl)
    {
        sourceUrl = "";
    }
    JSValue returnValue = JS_Eval(env->context, script, strlen(script), sourceUrl, JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(returnValue))
    {
        JSValue exceptionValue = JS_GetException(env->context);
        processPendingTask(env);
        if (JS_IsNull(exceptionValue))
        {
            env->isThrowNull = true;
        }
        JS_Throw(env->context, exceptionValue);

        return NAPIPendingException;
    }
    processPendingTask(env);
    if (result)
    {
        struct Handle *returnHandle;
        NAPIStatus status = addValueToHandleScope(env, returnValue, &returnHandle);
        if (status != NAPIOK)
        {
            JS_FreeValue(env->context, returnValue);

            return status;
        }
        *result = (NAPIValue)&returnHandle->value;
    }

    return NAPIOK;
}

static uint8_t contextCount = 0;

static void functionFinalizer(JSRuntime *rt, JSValue val)
{
    FunctionInfo *functionInfo = JS_GetOpaque(val, functionClassId);
    free(functionInfo);
}

static void externalFinalizer(JSRuntime *rt, JSValue val)
{
    ExternalInfo *externalInfo = JS_GetOpaque(val, externalClassId);
    if (externalInfo && externalInfo->finalizeCallback)
    {
        externalInfo->finalizeCallback(externalInfo->data, externalInfo->finalizeHint);
    }
    free(externalInfo);
}

static JSRuntime *runtime = NULL;

typedef struct
{
    FunctionInfo functionInfo;
    JSClassID classId; // uint32_t
} ConstructorInfo;

static JSClassID constructorClassId = 0;

static void constructorFinalizer(JSRuntime *rt, JSValue val)
{
    ConstructorInfo *constructorInfo = JS_GetOpaque(val, constructorClassId);
    free(constructorInfo);
}

static JSValue callAsConstructor(JSContext *ctx, JSValueConst newTarget, int argc, JSValueConst *argv)
{
    JSValue prototypeValue = JS_GetPropertyStr(ctx, newTarget, "prototype");
    if (JS_IsException(prototypeValue))
    {
        return prototypeValue;
    }
    ConstructorInfo *constructorInfo = JS_GetOpaque(prototypeValue, constructorClassId);
    JS_FreeValue(ctx, prototypeValue);
    if (!constructorInfo || !constructorInfo->functionInfo.baseInfo.env || !constructorInfo->functionInfo.callback)
    {
        assert(false);

        return undefinedValue;
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

        return undefinedValue;
    }
    NAPIValue retVal =
        constructorInfo->functionInfo.callback(constructorInfo->functionInfo.baseInfo.env, &callbackInfo);
    if (retVal && JS_IsObject(*((JSValue *)retVal)))
    {
        JSValue returnValue = JS_DupValue(ctx, *((JSValue *)retVal));
        JS_FreeValue(ctx, thisValue);
        thisValue = returnValue;
    }
    JSValue exceptionValue = JS_GetException(ctx);
    napi_close_handle_scope(constructorInfo->functionInfo.baseInfo.env, handleScope);
    if (constructorInfo->functionInfo.baseInfo.env->isThrowNull)
    {
        JS_FreeValue(ctx, thisValue);
        constructorInfo->functionInfo.baseInfo.env->isThrowNull = false;

        return JS_EXCEPTION;
    }
    else if (!JS_IsNull(exceptionValue))
    {
        JS_FreeValue(ctx, thisValue);

        return JS_Throw(ctx, exceptionValue);
    }

    return thisValue;
}

// NAPIMemoryError/NAPIPendingException + addValueToHandleScope
NAPIStatus NAPIDefineClass(NAPIEnv env, const char *utf8name, NAPICallback constructor, void *data, NAPIValue *result)
{
    NAPI_PREAMBLE(env);
    CHECK_ARG(constructor);
    CHECK_ARG(result);

    CHECK_ARG(runtime);

    ConstructorInfo *constructorInfo = malloc(sizeof(ConstructorInfo));
    RETURN_STATUS_IF_FALSE(constructorInfo, NAPIMemoryError);
    constructorInfo->functionInfo.baseInfo.env = env;
    constructorInfo->functionInfo.baseInfo.data = data;
    constructorInfo->functionInfo.callback = constructor;
    constructorInfo->classId = 0;
    JS_NewClassID(&constructorInfo->classId);
    JSClassDef classDef = {utf8name ?: "", NULL, NULL, NULL, NULL};
    int status = JS_NewClass(runtime, constructorInfo->classId, &classDef);
    if (status == -1)
    {
        free(constructorInfo);

        return NAPIPendingException;
    }
    JSValue prototype = JS_NewObjectClass(env->context, (int)constructorClassId);
    if (JS_IsException(prototype))
    {
        free(constructorInfo);

        return NAPIPendingException;
    }
    JS_SetOpaque(prototype, constructorInfo);
    // JS_NewCFunction2 会正确处理 utf8name 为空情况
    JSValue constructorValue = JS_NewCFunction2(env->context, callAsConstructor, utf8name, 0, JS_CFUNC_constructor, 0);
    if (JS_IsException(constructorValue))
    {
        // prototype 会自己 free ConstructorInfo 结构体
        JS_FreeValue(env->context, prototype);

        return NAPIPendingException;
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

    return NAPIOK;
}

// NAPIGenericFailure/NAPIMemoryError
NAPIStatus NAPICreateEnv(NAPIEnv *env)
{
    CHECK_ARG(env);
    if ((runtime && !contextCount) || (!runtime && contextCount))
    {
        assert(false);

        return NAPIGenericFailure;
    }
    *env = malloc(sizeof(struct OpaqueNAPIEnv));
    RETURN_STATUS_IF_FALSE(*env, NAPIMemoryError);
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

        JSClassDef classDef = {"External", externalFinalizer, NULL, NULL, NULL};
        int status = JS_NewClass(runtime, externalClassId, &classDef);
        if (status == -1)
        {
            JS_FreeRuntime(runtime);
            free(*env);
            runtime = NULL;
            constructorClassId = 0;
            functionClassId = 0;
            externalClassId = 0;

            return NAPIGenericFailure;
        }

        classDef.class_name = "FunctionData";
        classDef.finalizer = functionFinalizer;
        status = JS_NewClass(runtime, functionClassId, &classDef);
        if (status == -1)
        {
            JS_FreeRuntime(runtime);
            free(*env);
            runtime = NULL;
            constructorClassId = 0;
            functionClassId = 0;
            externalClassId = 0;

            return NAPIGenericFailure;
        }

        classDef.class_name = "ConstructorPrototype";
        classDef.finalizer = constructorFinalizer;
        status = JS_NewClass(runtime, constructorClassId, &classDef);
        if (status == -1)
        {
            JS_FreeRuntime(runtime);
            free(*env);
            runtime = NULL;
            constructorClassId = 0;
            functionClassId = 0;
            externalClassId = 0;

            return NAPIGenericFailure;
        }
    }
    JSContext *context = JS_NewContext(runtime);
    //    js_std_add_helpers(context, 0, NULL);
    if (!context)
    {
        free(*env);
        JS_FreeRuntime(runtime);
        runtime = NULL;
        constructorClassId = 0;
        functionClassId = 0;
        externalClassId = 0;

        return NAPIMemoryError;
    }
    JSValue prototype = JS_NewObject(context);
    if (JS_IsException(prototype))
    {
        JS_FreeContext(context);
        JS_FreeRuntime(runtime);
        free(*env);
        runtime = NULL;
        constructorClassId = 0;
        functionClassId = 0;
        externalClassId = 0;

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
        constructorClassId = 0;
        functionClassId = 0;
        externalClassId = 0;

        return NAPIGenericFailure;
    }
    JS_SetClassProto(context, constructorClassId, prototype);
    const char *string = "(function () { return Symbol(\"reference\") })();";
    (*env)->referenceSymbolValue =
        JS_Eval(context, string, strlen(string), "https://n-api.com/qjs_reference_symbol.js", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException((*env)->referenceSymbolValue))
    {
        JS_FreeContext(context);
        JS_FreeRuntime(runtime);
        free(*env);
        runtime = NULL;
        constructorClassId = 0;
        functionClassId = 0;
        externalClassId = 0;

        return NAPIGenericFailure;
    }
    contextCount += 1;
    (*env)->context = context;
    (*env)->isThrowNull = false;
    LIST_INIT(&(*env)->handleScopeList);
    LIST_INIT(&(*env)->referenceList);
    LIST_INIT(&(*env)->valueList);
    LIST_INIT(&(*env)->strongRefList);

    return NAPIOK;
}

void NAPIFreeEnv(NAPIEnv env)
{
    CHECK_ARG(env);

    assert(LIST_EMPTY(&(*env).handleScopeList));
    //    NAPIHandleScope handleScope, tempHandleScope;
    //    LIST_FOREACH_SAFE(handleScope, &(*env).handleScopeList, node, tempHandleScope)
    //    napi_close_handle_scope(env, handleScope);
    NAPIRef ref, temp;
    LIST_FOREACH_SAFE(ref, &env->strongRefList, node, temp)
    {
        LIST_REMOVE(ref, node);
        JS_FreeValue(env->context, ref->value);
        free(ref);
    }
    struct ReferenceInfo *referenceInfo;
    gcLock = true;
    LIST_FOREACH(referenceInfo, &env->referenceList, node)
    {
        LIST_FOREACH_SAFE(ref, &referenceInfo->referenceList, node, temp)
        {
            LIST_REMOVE(ref, node);
            if (!temp)
            {
                // 最后一个
                bool deleteResult;
                // 可能触发 GC，如果触发 GC 会导致 referenceInfo 被销毁，遍历走不下去，所以添加了一个 gcLock
                if (napi_delete_property(env, (NAPIValue)&ref->value, (NAPIValue)&env->referenceSymbolValue,
                                         &deleteResult) != NAPIOK)
                {
                    assert(false);
                }
                assert(deleteResult);
            }
            free(ref);
        }
        referenceInfo->isEnvFreed = true;
    }
    gcLock = false;
    // 到这一步，所有弱引用已经全部释放完成
    LIST_FOREACH_SAFE(ref, &env->valueList, node, temp)
    {
        LIST_REMOVE(ref, node);
        free(ref);
    }
    JS_FreeValue(env->context, env->referenceSymbolValue);
    JS_FreeContext(env->context);
    if (--contextCount == 0 && runtime)
    {
        // virtualMachine 不能为 NULL
        JS_FreeRuntime(runtime);
        runtime = NULL;
        constructorClassId = 0;
        functionClassId = 0;
        externalClassId = 0;
    }
    free(env);
}

NAPIStatus NAPIGetValueStringUTF8(NAPIEnv env, NAPIValue value, const char **result)
{
    CHECK_ARG(env);
    CHECK_ARG(value);
    CHECK_ARG(result);

    CHECK_ARG(env->context);
    RETURN_STATUS_IF_FALSE(JS_IsString(*((JSValue *)value)), NAPIStringExpected);

    *result = JS_ToCString(env->context, *((JSValue *)value));

    return NAPIOK;
}

NAPIStatus NAPIFreeUTF8String(NAPIEnv env, const char *cString)
{
    CHECK_ARG(env);

    CHECK_ARG(env->context);

    JS_FreeCString(env->context, cString);

    return NAPIOK;
}

void NAPIEnableDebugger(__attribute__((unused)) NAPIEnv env, __attribute__((unused)) const char *debuggerTitle)
{
}

void NAPIDisableDebugger(__attribute__((unused)) NAPIEnv env)
{
}