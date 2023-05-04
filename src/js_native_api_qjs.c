// 所有函数都可能抛出 NAPIInvalidArg NAPIOK
// 当返回值为 NAPIOK 时候，必须保证 result 可用，但是发生错误的时候，result 也可能会被修改，所以建议不要对 result 做
// NULL 初始化

#include <assert.h>
#include <napi/js_native_api.h>
#include <napi/js_native_api_debugger.h>
#include <quickjs.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#include <limits.h>

#ifndef SLIST_FOREACH_SAFE
#define SLIST_FOREACH_SAFE(var, head, field, tvar)                                                                     \
    for ((var) = SLIST_FIRST((head)); (var) && ((tvar) = SLIST_NEXT((var), field), 1); (var) = (tvar))
#endif

#ifndef LIST_FOREACH_SAFE
#define LIST_FOREACH_SAFE(var, head, field, tvar)                                                                      \
    for ((var) = LIST_FIRST((head)); (var) && ((tvar) = LIST_NEXT((var), field), 1); (var) = (tvar))
#endif

#define RETURN_STATUS_IF_FALSE(condition, status)                                                                      \
    if (__builtin_expect(!(condition), false))                                                                         \
    {                                                                                                                  \
        return status;                                                                                                 \
    }

#define CHECK_NAPI(expr, exprStatus, returnStatus)                                                                     \
    {                                                                                                                  \
        NAPI##exprStatus##Status status = expr;                                                                        \
        if (__builtin_expect(status != NAPI##exprStatus##OK, false))                                                   \
        {                                                                                                              \
            return (NAPI##returnStatus##Status)status;                                                                 \
        }                                                                                                              \
    }

#define CHECK_ARG(arg, status)                                                                                         \
    if (__builtin_expect(!(arg), false))                                                                               \
    {                                                                                                                  \
        return NAPI##status##InvalidArg;                                                                               \
    }

// JS_GetException 会转移所有权
// JS_Throw 会获取所有权
// 所以如果存在异常，先获取所有权，再交还所有权，最终所有权依旧在
// JSContext 中，QuickJS 目前 JS null 代表正常，未来考虑写成这样
// !JS_IsUndefined(exceptionValue) && !JS_IsNull(exceptionValue)
// NAPI_PREAMBLE 同时会检查 env->context
#define NAPI_PREAMBLE(env)                                                                                             \
    {                                                                                                                  \
        CHECK_ARG(env, Exception)                                                                                      \
        JSValue exceptionValue = JS_GetException((env)->context);                                                      \
        if (__builtin_expect(!JS_IsNull(exceptionValue), false))                                                       \
        {                                                                                                              \
            JS_Throw((env)->context, exceptionValue);                                                                  \
                                                                                                                       \
            return NAPIExceptionPendingException;                                                                      \
        }                                                                                                              \
        RETURN_STATUS_IF_FALSE(!(env)->isThrowNull, NAPIExceptionPendingException)                                     \
    }

struct Handle
{
    JSValue value;            // size_t * 2
    SLIST_ENTRY(Handle) node; // size_t
};

struct OpaqueNAPIHandleScope
{
    LIST_ENTRY(OpaqueNAPIHandleScope) node; // size_t
    SLIST_HEAD(, Handle) handleList;        // size_t
};

struct OpaqueNAPIRef
{
    JSValue value;                  // size_t * 2
    LIST_ENTRY(OpaqueNAPIRef) node; // size_t * 2
    uint8_t referenceCount;         // 8
};

struct WeakReference
{
    LIST_ENTRY(WeakReference) node; // size_t * 2
    // 目前还有效的弱引用
    LIST_HEAD(, OpaqueNAPIRef) weakRefList; // size_t
    // NAPIFreeEnv 会 free 所有 NAPIRef，referenceFinalize() 就只需要 free 自身结构体
    bool isEnvFreed;
};

// 1. external -> 不透明指针 + finalizer + 调用一个回调
// 2. Function -> JSValue data 数组 + finalizer
// 3. Constructor -> .[[prototype]] = new External()
// 4. Reference -> 引用计数 + setWeak/clearWeak -> referenceSymbolValue

struct OpaqueNAPIEnv
{
    JSValue referenceSymbolValue;                       // size_t * 2
    NAPIRuntime runtime;                                // size_t
    JSContext *context;                                 // size_t
    LIST_HEAD(, OpaqueNAPIHandleScope) handleScopeList; // size_t
    LIST_HEAD(, WeakReference) weakReferenceList;       // size_t
    LIST_HEAD(, OpaqueNAPIRef) strongRefList;           // size_t
    LIST_HEAD(, OpaqueNAPIRef) valueList;               // size_t
    bool isThrowNull;
};

struct OpaqueNAPIRuntime
{
    JSRuntime *runtime;           // size_t
    JSClassID constructorClassId; // uint32_t
    JSClassID functionClassId;    // uint32_t
    JSClassID externalClassId;    // uint32_t
};

// 这个函数不会修改引用计数和所有权
// NAPIHandleScopeEmpty/NAPIMemoryError
static NAPIErrorStatus addValueToHandleScope(NAPIEnv env, JSValue value, struct Handle **result)
{

    CHECK_ARG(env, Error)
    CHECK_ARG(result, Error)

    RETURN_STATUS_IF_FALSE(!LIST_EMPTY(&env->handleScopeList), NAPIErrorHandleScopeEmpty)
    *result = malloc(sizeof(struct Handle));
    RETURN_STATUS_IF_FALSE(*result, NAPIErrorMemoryError)
    (*result)->value = value;
    NAPIHandleScope handleScope = LIST_FIRST(&env->handleScopeList);
    SLIST_INSERT_HEAD(&handleScope->handleList, *result, node);

    return NAPIErrorOK;
}

static JSValueConst undefinedValue = JS_UNDEFINED;

NAPICommonStatus napi_get_undefined(NAPIEnv env, NAPIValue *result)
{

    CHECK_ARG(env, Common)
    CHECK_ARG(result, Common)

    *result = (NAPIValue)&undefinedValue;

    return NAPICommonOK;
}

static JSValueConst nullValue = JS_NULL;

NAPICommonStatus napi_get_null(NAPIEnv env, NAPIValue *result)
{

    CHECK_ARG(env, Common)
    CHECK_ARG(result, Common)

    *result = (NAPIValue)&nullValue;

    return NAPICommonOK;
}

#ifndef NDEBUG
static char *const JS_GET_GLOBAL_OBJECT_EXCEPTION = "JS_GetGlobalObject() -> JS_EXCEPTION.";
static char *const FUNCTION_CLASS_ID_ZERO = "functionClassId must not be 0.";

static char *const CONSTRUCTOR_CLASS_ID_ZERO = "constructorClassId must not be 0.";
static char *const NAPI_CLOSE_HANDLE_SCOPE_ERROR = "napi_close_handle_scope() return error.";
#endif

// NAPIGenericFailure + addValueToHandleScope
NAPIErrorStatus napi_get_global(NAPIEnv env, NAPIValue *result)
{

    CHECK_ARG(env, Error)
    CHECK_ARG(result, Error)

    // JS_GetGlobalObject 返回已经引用计数 +1
    // 实际上 globalValue 可能为 JS_EXCEPTION，但是由于不是 JS_GetGlobalObject 中发生的异常，因此返回 generic failure
    JSValue globalValue = JS_GetGlobalObject(env->context);
    if (__builtin_expect(JS_IsException(globalValue), false))
    {
        assert(false && JS_GET_GLOBAL_OBJECT_EXCEPTION);

        return NAPIErrorGenericFailure;
    }
    struct Handle *globalHandle;
    NAPIErrorStatus status = addValueToHandleScope(env, globalValue, &globalHandle);
    if (__builtin_expect(status != NAPIErrorOK, false))
    {
        JS_FreeValue(env->context, globalValue);

        return status;
    }
    *result = (NAPIValue)&globalHandle->value;

    return NAPIErrorOK;
}

static JSValue trueValue = JS_TRUE;

static JSValue falseValue = JS_FALSE;

NAPIErrorStatus napi_get_boolean(NAPIEnv env, bool value, NAPIValue *result)
{

    CHECK_ARG(env, Error)
    CHECK_ARG(result, Error)

    *result = (NAPIValue)(value ? &trueValue : &falseValue);

    return NAPIErrorOK;
}

// + addValueToHandleScope
NAPIErrorStatus napi_create_double(NAPIEnv env, double value, NAPIValue *result)
{

    CHECK_ARG(env, Error)
    CHECK_ARG(result, Error)

    JSValue jsValue = JS_NewFloat64(env->context, value);
    struct Handle *handle;
    CHECK_NAPI(addValueToHandleScope(env, jsValue, &handle), Error, Error)
    *result = (NAPIValue)&handle->value;

    return NAPIErrorOK;
}

// NAPIPendingException + addValueToHandleScope
NAPIExceptionStatus napi_create_string_utf8(NAPIEnv env, const char *str, NAPIValue *result)
{

    NAPI_PREAMBLE(env)
    CHECK_ARG(result, Exception)

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
    RETURN_STATUS_IF_FALSE(!JS_IsException(stringValue), NAPIExceptionPendingException)
    struct Handle *stringHandle;
    NAPIErrorStatus status = addValueToHandleScope(env, stringValue, &stringHandle);
    if (__builtin_expect(status != NAPIErrorOK, false))
    {
        JS_FreeValue(env->context, stringValue);

        return (NAPIExceptionStatus)status;
    }
    *result = (NAPIValue)&stringHandle->value;

    return NAPIExceptionOK;
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
// static JSClassID functionClassId = 0;

struct OpaqueNAPICallbackInfo
{
    JSValueConst newTarget; // size_t * 2
    JSValueConst thisArg;   // size_t * 2
    JSValueConst *argv;     // size_t * 2
    void *data;             // size_t
    int argc;
};

static JSValue callAsFunction(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv, int magic,
                              JSValue *funcData)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    NAPIRuntime runtime = JS_GetRuntimeOpaque(rt);
    if (__builtin_expect(!runtime->functionClassId, false))
    {
        assert(false && FUNCTION_CLASS_ID_ZERO);

        return undefinedValue;
    }
    // 固定 1 参数
    // JS_GetOpaque 不会抛出异常
    FunctionInfo *functionInfo = JS_GetOpaque(funcData[0], runtime->functionClassId);
    if (__builtin_expect(!functionInfo || !functionInfo->callback || !functionInfo->baseInfo.env, false))
    {
        assert(false && "callAsFunction() body use JS_GetOpaque() return error.");

        return undefinedValue;
    }
    // thisVal 有可能为 undefined，如果直接调用函数，比如 test() 而不是 this.test() 或者 globalThis.test()
    bool useGlobalValue = false;
    if (JS_IsUndefined(thisVal))
    {
        useGlobalValue = true;
        thisVal = JS_GetGlobalObject(ctx);
        if (__builtin_expect(JS_IsException(thisVal), false))
        {
            assert(false && JS_GET_GLOBAL_OBJECT_EXCEPTION);

            return undefinedValue;
        }
    }
    struct OpaqueNAPICallbackInfo callbackInfo = {undefinedValue, thisVal, argv, functionInfo->baseInfo.data, argc};
    // napi_open_handle_scope 失败需要容错，这里需要初始化为 NULL 判断
    NAPIHandleScope handleScope = NULL;
    // 内存分配失败，由最外层做 HandleScope
    {
        NAPIErrorStatus status = napi_open_handle_scope(functionInfo->baseInfo.env, &handleScope);
        assert(status == NAPIErrorOK);
        (void)status;
    }
    // callback 调用后，返回值应当属于当前 handleScope 管理，否则业务方后果自负
    NAPIValue retVal = functionInfo->callback(functionInfo->baseInfo.env, &callbackInfo);
    if (useGlobalValue)
    {
        // 释放 thisVal = JS_GetGlobalObject(ctx); 的情况
        JS_FreeValue(ctx, thisVal);
    }
    // Check NULL
    JSValue returnValue = undefinedValue;
    if (retVal)
    {
        // 正常返回值应当是由 HandleScope 持有，所以需要引用计数 +1
        // 该代码执行后，一份所有权在 handleScope，或者是 undefinedValue
        // 一份所有权是 JS_DupValue()
        returnValue = JS_DupValue(ctx, *((JSValue *)retVal));
    }
    // handleScope
    // 关闭 handleScope 后，返回值只可能为 undefinedValue，或者 JS_DupValue() -> returnValue
    // 除非 handleScope == NULL，但是这种情况不影响后续代码的执行
    if (handleScope)
    {
        NAPICommonStatus commonStatus = napi_close_handle_scope(functionInfo->baseInfo.env, handleScope);
        if (__builtin_expect(commonStatus != NAPICommonOK, false))
        {
            JS_FreeValue(ctx, returnValue);
            assert(false && NAPI_CLOSE_HANDLE_SCOPE_ERROR);

            return undefinedValue;
        }
    }
    // Check exception
    JSValue exceptionValue = JS_GetException(ctx);
    if (!JS_IsNull(exceptionValue))
    {
        JS_FreeValue(ctx, returnValue);

        // JS_Throw() -> JS_EXCEPTION
        return JS_Throw(ctx, exceptionValue);
    }
    if (functionInfo->baseInfo.env->isThrowNull)
    {
        JS_FreeValue(ctx, returnValue);
        functionInfo->baseInfo.env->isThrowNull = false;

        return JS_EXCEPTION;
    }

    return returnValue;
}

// NAPIMemoryError/NAPIPendingException + addValueToHandleScope + napi_create_string_utf8
NAPIExceptionStatus napi_create_function(NAPIEnv env, const char *utf8name, NAPICallback cb, void *data,
                                         NAPIValue *result)
{
    NAPI_PREAMBLE(env)
    CHECK_ARG(cb, Exception)
    CHECK_ARG(result, Exception)

    // TODO(ChasonTang): napi_open_handle_scope() 并使用 bool 控制两种情况

    NAPIValue nameValue;
    CHECK_NAPI(napi_create_string_utf8(env, utf8name, &nameValue), Exception, Exception)

    // malloc
    FunctionInfo *functionInfo = malloc(sizeof(FunctionInfo));
    RETURN_STATUS_IF_FALSE(functionInfo, NAPIExceptionMemoryError)
    functionInfo->baseInfo.env = env;
    functionInfo->baseInfo.data = data;
    functionInfo->callback = cb;

    if (__builtin_expect(!env->runtime->functionClassId, false))
    {
        assert(false && FUNCTION_CLASS_ID_ZERO);
        free(functionInfo);

        return NAPIExceptionGenericFailure;
    }
    // rc: 1
    JSValue dataValue = JS_NewObjectClass(env->context, (int)env->runtime->functionClassId);
    if (__builtin_expect(JS_IsException(dataValue), false))
    {
        free(functionInfo);

        return NAPIExceptionPendingException;
    }
    // functionInfo 生命周期被 JSValue 托管
    JS_SetOpaque(dataValue, functionInfo);

    // dataValue 引用计数 +1 => rc: 2
    // 默认创建 name: ""
    JSValue functionValue = JS_NewCFunctionData(env->context, callAsFunction, 0, 0, 1, &dataValue);
    // 转移所有权
    JS_FreeValue(env->context, dataValue);
    // 如果 functionValue 创建失败，dataValue rc 不会被 +1，上面的引用计数 -1 => rc 为
    // 0，发生垃圾回收，FunctionInfo 结构体也被回收
    RETURN_STATUS_IF_FALSE(!JS_IsException(functionValue), NAPIExceptionPendingException)
    {
        // JS_DefinePropertyValueStr -> JS_DefinePropertyValue 转移所有权，并自动传入 JS_PROP_HAS_CONFIGURABLE
        int returnStatus =
            JS_DefinePropertyValueStr(env->context, functionValue, "name",
                                      JS_DupValue(env->context, *((JSValue *)nameValue)), JS_PROP_CONFIGURABLE);
        // 没有传入 JS_PROP_THROW 也不代表不会返回 -1，传入 JS_PROP_THROW 的意思是，所有 false 情况会变成 exception
        if (__builtin_expect(returnStatus == -1, false))
        {
            JS_FreeValue(env->context, functionValue);

            return NAPIExceptionPendingException;
        }
    }
    struct Handle *functionHandle;
    NAPIErrorStatus status = addValueToHandleScope(env, functionValue, &functionHandle);
    if (__builtin_expect(status != NAPIErrorOK, false))
    {
        // 由于 dataValue 所有权被 functionValue 持有，所以只需要对 functionValue 做引用计数 -1
        JS_FreeValue(env->context, functionValue);

        return (NAPIExceptionStatus)status;
    }
    *result = (NAPIValue)&functionHandle->value;

    return NAPIExceptionOK;
}

// static JSClassID externalClassId = 0;

NAPICommonStatus napi_typeof(NAPIEnv env, NAPIValue value, NAPIValueType *result)
{

    CHECK_ARG(env, Common)
    CHECK_ARG(value, Common)
    CHECK_ARG(result, Common)

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
    else if (JS_IsSymbol(jsValue))
    {
        *result = NAPISymbol;
    }
    else if (JS_IsFunction(env->context, jsValue))
    {
        *result = NAPIFunction;
    }
    else if (JS_GetOpaque(jsValue, env->runtime->externalClassId))
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
        return NAPICommonInvalidArg;
    }

    return NAPICommonOK;
}

// NAPINumberExpected
NAPIErrorStatus napi_get_value_double(NAPIEnv env, NAPIValue value, double *result)
{

    CHECK_ARG(env, Error)
    CHECK_ARG(value, Error)
    CHECK_ARG(result, Error)

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
        return NAPIErrorNumberExpected;
    }

    return NAPIErrorOK;
}

// NAPIBooleanExpected
NAPIErrorStatus napi_get_value_bool(NAPIEnv env, NAPIValue value, bool *result)
{

    CHECK_ARG(env, Error)
    CHECK_ARG(value, Error)
    CHECK_ARG(result, Error)

    RETURN_STATUS_IF_FALSE(JS_IsBool(*((JSValue *)value)), NAPIErrorBooleanExpected)
    *result = JS_VALUE_GET_BOOL(*((JSValue *)value));

    return NAPIErrorOK;
}

// NAPIPendingException + napi_get_boolean
NAPIExceptionStatus napi_coerce_to_bool(NAPIEnv env, NAPIValue value, NAPIValue *result)
{

    NAPI_PREAMBLE(env)
    CHECK_ARG(value, Exception)
    CHECK_ARG(result, Exception)

    int boolStatus = JS_ToBool(env->context, *((JSValue *)value));
    RETURN_STATUS_IF_FALSE(boolStatus != -1, NAPIExceptionPendingException)
    CHECK_NAPI(napi_get_boolean(env, boolStatus, result), Error, Exception)

    return NAPIExceptionOK;
}

// NAPIPendingException + napi_create_double
NAPIExceptionStatus napi_coerce_to_number(NAPIEnv env, NAPIValue value, NAPIValue *result)
{

    NAPI_PREAMBLE(env)
    CHECK_ARG(value, Exception)
    CHECK_ARG(result, Exception)

    double doubleValue;
    // JS_ToFloat64 只有 -1 0 两种
    int floatStatus = JS_ToFloat64(env->context, &doubleValue, *((JSValue *)value));
    RETURN_STATUS_IF_FALSE(floatStatus != -1, NAPIExceptionPendingException)
    CHECK_NAPI(napi_create_double(env, doubleValue, result), Error, Exception)

    return NAPIExceptionOK;
}

// NAPIPendingException + addValueToHandleScope
NAPIExceptionStatus napi_coerce_to_string(NAPIEnv env, NAPIValue value, NAPIValue *result)
{

    NAPI_PREAMBLE(env)
    CHECK_ARG(value, Exception)
    CHECK_ARG(result, Exception)

    JSValue stringValue = JS_ToString(env->context, *((JSValue *)value));
    RETURN_STATUS_IF_FALSE(!JS_IsException(stringValue), NAPIExceptionPendingException)
    struct Handle *stringHandle;
    NAPIErrorStatus status = addValueToHandleScope(env, stringValue, &stringHandle);
    if (__builtin_expect(status != NAPIErrorOK, false))
    {
        JS_FreeValue(env->context, stringValue);

        return (NAPIExceptionStatus)status;
    }
    *result = (NAPIValue)&stringHandle->value;

    return NAPIExceptionOK;
}

// NAPIPendingException/NAPIGenericFailure
NAPIExceptionStatus napi_set_property(NAPIEnv env, NAPIValue object, NAPIValue key, NAPIValue value)
{

    NAPI_PREAMBLE(env)
    CHECK_ARG(object, Exception)
    CHECK_ARG(key, Exception)
    CHECK_ARG(value, Exception)

    JSAtom atom = JS_ValueToAtom(env->context, *((JSValue *)key));
    RETURN_STATUS_IF_FALSE(atom != JS_ATOM_NULL, NAPIExceptionPendingException)
    // TODO(ChasonTang): 检查 JS_SetProperty -> JS_SetPropertyInternal 传入的 JS_PROP_THROW 和主流 JS 引擎是否一致
    // JS_SetProperty 转移所有权
    int status =
        JS_SetProperty(env->context, *((JSValue *)object), atom, JS_DupValue(env->context, *((JSValue *)value)));
    if (__builtin_expect(!status, false))
    {
        assert(false && "JS_SetProperty() -> false");

        return NAPIExceptionGenericFailure;
    }
    JS_FreeAtom(env->context, atom);
    RETURN_STATUS_IF_FALSE(status != -1, NAPIExceptionPendingException)

    return NAPIExceptionOK;
}

// NAPIPendingException
NAPIExceptionStatus napi_has_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result)
{

    NAPI_PREAMBLE(env)
    CHECK_ARG(object, Exception)
    CHECK_ARG(key, Exception)
    CHECK_ARG(result, Exception)

    JSAtom atom = JS_ValueToAtom(env->context, *((JSValue *)key));
    RETURN_STATUS_IF_FALSE(atom != JS_ATOM_NULL, NAPIExceptionPendingException)
    int status = JS_HasProperty(env->context, *((JSValue *)object), atom);
    JS_FreeAtom(env->context, atom);
    RETURN_STATUS_IF_FALSE(status != -1, NAPIExceptionPendingException)
    *result = status;

    return NAPIExceptionOK;
}

// NAPIPendingException + addValueToHandleScope
NAPIExceptionStatus napi_get_property(NAPIEnv env, NAPIValue object, NAPIValue key, NAPIValue *result)
{

    NAPI_PREAMBLE(env)
    CHECK_ARG(object, Exception)
    CHECK_ARG(key, Exception)
    CHECK_ARG(result, Exception)

    JSAtom atom = JS_ValueToAtom(env->context, *((JSValue *)key));
    RETURN_STATUS_IF_FALSE(atom != JS_ATOM_NULL, NAPIExceptionPendingException)
    // JS_GetProperty -> JS_GetPropertyInternal 传入 throw_ref_error = 0，不代表不会抛出异常
    // 正常 JS 引擎，当引用不存在的时候，不会抛出异常 '%s' is not defined
    JSValue value = JS_GetProperty(env->context, *((JSValue *)object), atom);
    JS_FreeAtom(env->context, atom);
    RETURN_STATUS_IF_FALSE(!JS_IsException(value), NAPIExceptionPendingException)
    struct Handle *handle;
    NAPIErrorStatus status = addValueToHandleScope(env, value, &handle);
    if (__builtin_expect(status != NAPIErrorOK, false))
    {
        JS_FreeValue(env->context, value);

        return (NAPIExceptionStatus)status;
    }
    *result = (NAPIValue)&handle->value;

    return NAPIExceptionOK;
}

// NAPIPendingException
NAPIExceptionStatus napi_delete_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result)
{

    NAPI_PREAMBLE(env)
    CHECK_ARG(object, Exception)
    CHECK_ARG(key, Exception)

    JSAtom atom = JS_ValueToAtom(env->context, *((JSValue *)key));
    RETURN_STATUS_IF_FALSE(atom != JS_ATOM_NULL, NAPIExceptionPendingException)
    // 不抛出 could not delete property 异常
    // 正常 JS 引擎 delete configurable: false 的属性，会直接返回 false，而不是抛出异常
    int status = JS_DeleteProperty(env->context, *((JSValue *)object), atom, JS_PROP_NORMAL);
    JS_FreeAtom(env->context, atom);
    RETURN_STATUS_IF_FALSE(status != -1, NAPIExceptionPendingException)
    if (result)
    {
        *result = status;
    }

    return NAPIExceptionOK;
}

NAPICommonStatus napi_is_array(NAPIEnv env, NAPIValue value, bool *result)
{

    CHECK_ARG(env, Common)
    CHECK_ARG(value, Common)
    CHECK_ARG(result, Common)

    // 正常情况下 napi_is_array 不会检查 Proxy 的情况
    int isArrayStatus = JS_IsArray(env->context, *((JSValue *)value));
    if (isArrayStatus == -1)
    {
        CHECK_NAPI(NAPIClearLastException(env), Common, Common)
    }
    *result = isArrayStatus;

    return NAPICommonOK;
}

static void processPendingTask(NAPIEnv env)
{
    if (__builtin_expect(!env, false))
    {
        return;
    }

    int error = 1;
    do
    {
        JSContext *context;
        error = JS_ExecutePendingJob(JS_GetRuntime(env->context), &context);
        if (error == -1)
        {
            // 正常情况下 JS_ExecutePendingJob 返回 -1
            // 代表引擎内部异常，比如内存分配失败等
            // TODO(ChasonTang): 测试 Promise 抛出异常的情况
            JSValue inlineExceptionValue = JS_GetException(context);
            JS_FreeValue(context, inlineExceptionValue);
        }
    } while (error != 0);
}

// NAPIMemoryError/NAPIPendingException + addValueToHandleScope
NAPIExceptionStatus napi_call_function(NAPIEnv env, NAPIValue thisValue, NAPIValue func, size_t argc,
                                       const NAPIValue *argv, NAPIValue *result)
{

    NAPI_PREAMBLE(env)
    CHECK_ARG(func, Exception)

    // TODO(ChasonTang): napi_open_handle_scope()

    if (!thisValue)
    {
        CHECK_NAPI(napi_get_global(env, &thisValue), Error, Exception)
    }

    JSValue *internalArgv = NULL;
    if (argc > 0)
    {
        RETURN_STATUS_IF_FALSE(argc <= INT_MAX, NAPIExceptionInvalidArg)
        CHECK_ARG(argv, Exception)
        internalArgv = malloc(sizeof(JSValue) * argc);
        RETURN_STATUS_IF_FALSE(internalArgv, NAPIExceptionMemoryError)
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
        else
        {
            JS_Throw(env->context, exceptionValue);
        }

        return NAPIExceptionPendingException;
    }
    processPendingTask(env);
    if (result)
    {
        struct Handle *handle;
        NAPIErrorStatus status = addValueToHandleScope(env, returnValue, &handle);
        if (__builtin_expect(status != NAPIErrorOK, false))
        {
            JS_FreeValue(env->context, returnValue);

            return (NAPIExceptionStatus)status;
        }
        *result = (NAPIValue)&handle->value;
    }
    else
    {
        JS_FreeValue(env->context, returnValue);
    }

    return NAPIExceptionOK;
}

// NAPIPendingException/NAPIMemoryError + addValueToHandleScope
NAPIExceptionStatus napi_new_instance(NAPIEnv env, NAPIValue constructor, size_t argc, const NAPIValue *argv,
                                      NAPIValue *result)
{

    NAPI_PREAMBLE(env)
    CHECK_ARG(constructor, Exception)
    CHECK_ARG(result, Exception)

    JSValue *internalArgv = NULL;
    if (argc > 0)
    {
        RETURN_STATUS_IF_FALSE(argc <= INT_MAX, NAPIExceptionInvalidArg)
        CHECK_ARG(argv, Exception)
        internalArgv = malloc(sizeof(JSValue) * argc);
        RETURN_STATUS_IF_FALSE(internalArgv, NAPIExceptionMemoryError)
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
        else
        {
            JS_Throw(env->context, exceptionValue);
        }

        return NAPIExceptionPendingException;
    }
    processPendingTask(env);
    struct Handle *handle;
    NAPIErrorStatus status = addValueToHandleScope(env, returnValue, &handle);
    if (__builtin_expect(status != NAPIErrorOK, false))
    {
        JS_FreeValue(env->context, returnValue);

        return (NAPIExceptionStatus)status;
    }
    *result = (NAPIValue)&handle->value;

    return NAPIExceptionOK;
}

// NAPIPendingException
NAPIExceptionStatus napi_instanceof(NAPIEnv env, NAPIValue object, NAPIValue constructor, bool *result)
{

    NAPI_PREAMBLE(env)
    CHECK_ARG(object, Exception)
    CHECK_ARG(constructor, Exception)
    CHECK_ARG(result, Exception)

    int status = JS_IsInstanceOf(env->context, *((JSValue *)object), *((JSValue *)constructor));
    RETURN_STATUS_IF_FALSE(status != -1, NAPIExceptionPendingException)
    *result = status;

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

    return NAPICommonOK;
}

NAPICommonStatus napi_get_new_target(NAPIEnv env, NAPICallbackInfo callbackInfo, NAPIValue *result)
{

    CHECK_ARG(env, Common)
    CHECK_ARG(callbackInfo, Common)
    CHECK_ARG(result, Common)

    if (!JS_IsUndefined(callbackInfo->newTarget))
    {
        *result = (NAPIValue)&callbackInfo->newTarget;
    }
    else
    {
        *result = NULL;
    }

    return NAPICommonOK;
}

typedef struct
{
    void *data;                    // size_t
    void *finalizeHint;            // size_t
    NAPIFinalize finalizeCallback; // size_t
} ExternalInfo;

// NAPIMemoryError/NAPIPendingException + addValueToHandleScope
NAPIExceptionStatus napi_create_external(NAPIEnv env, void *data, NAPIFinalize finalizeCB, void *finalizeHint,
                                         NAPIValue *result)
{

    NAPI_PREAMBLE(env)
    CHECK_ARG(result, Exception)

    ExternalInfo *externalInfo = malloc(sizeof(ExternalInfo));
    RETURN_STATUS_IF_FALSE(externalInfo, NAPIExceptionMemoryError)
    externalInfo->data = data;
    externalInfo->finalizeHint = finalizeHint;
    externalInfo->finalizeCallback = NULL;
    if (__builtin_expect(!env->runtime->externalClassId, false))
    {
        assert(false && "externalClassId must not be 0.");
        free(externalInfo);

        return NAPIExceptionGenericFailure;
    }
    JSValue object = JS_NewObjectClass(env->context, (int)env->runtime->externalClassId);
    if (__builtin_expect(JS_IsException(object), false))
    {
        free(externalInfo);

        return NAPIExceptionPendingException;
    }
    JS_SetOpaque(object, externalInfo);
    struct Handle *handle;
    NAPIErrorStatus status = addValueToHandleScope(env, object, &handle);
    if (__builtin_expect(status != NAPIErrorOK, false))
    {
        JS_FreeValue(env->context, object);

        return (NAPIExceptionStatus)status;
    }
    *result = (NAPIValue)&handle->value;
    // 不能先设置回调，万一出错，业务方也会收到回调
    externalInfo->finalizeCallback = finalizeCB;

    return NAPIExceptionOK;
}

NAPIErrorStatus napi_get_value_external(NAPIEnv env, NAPIValue value, void **result)
{

    CHECK_ARG(env, Error)
    CHECK_ARG(value, Error)
    CHECK_ARG(result, Error)

    if (__builtin_expect(!env->runtime->externalClassId, false))
    {
        assert(false && "externalClassId must not be 0.");

        return NAPIErrorGenericFailure;
    }
    ExternalInfo *externalInfo = JS_GetOpaque(*((JSValue *)value), env->runtime->externalClassId);
    *result = externalInfo ? externalInfo->data : NULL;

    return NAPIErrorOK;
}

// static uint8_t contextCount = 0;

static void referenceFinalize(void *finalizeData, void *finalizeHint)
{
    // finalizeHint => env 不可靠
    if (!finalizeData)
    {
        assert(false);

        return;
    }
    struct WeakReference *referenceInfo = finalizeData;
    // isEnvFreed 需要存在，因为有三种情况
    // 1. env 被析构
    // 2. env 还在，也有 referenceInfo，但是没有 weakRef
    // 3. env 还在，也有 referenceInfo，也有 weakRef
    if (!referenceInfo->isEnvFreed)
    {
        NAPIRef reference, temp;
        LIST_FOREACH_SAFE(reference, &referenceInfo->weakRefList, node, temp)
        {
            // 如果进入循环，说明 env 是有效的
            assert(!reference->referenceCount);
            reference->value = undefinedValue;
            LIST_REMOVE(reference, node);
            // 所以这里 finalizeHint 还是有效的 env
            LIST_INSERT_HEAD(&((NAPIEnv)finalizeHint)->valueList, reference, node);
        }
        // 当前不是 last GC
        // LIST_REMOVE 会影响 env 结构体
        LIST_REMOVE(referenceInfo, node);
    }

    free(referenceInfo);
}

// NAPINameExpected/NAPIMemoryError/NAPIGenericFailure + napi_create_string_utf8 + napi_get_property + napi_typeof +
// napi_create_external
// + napi_get_value_external + napi_set_property
static NAPIExceptionStatus setWeak(NAPIEnv env, NAPIValue value, NAPIRef ref)
{
    NAPI_PREAMBLE(env)
    CHECK_ARG(value, Exception)
    CHECK_ARG(ref, Exception)

    RETURN_STATUS_IF_FALSE(JS_IsSymbol(env->referenceSymbolValue), NAPIExceptionNameExpected)

    // TODO(ChasonTang): napi_open_handle_scope()

    NAPIValue referenceValue;
    CHECK_NAPI(napi_get_property(env, value, (NAPIValue)&env->referenceSymbolValue, &referenceValue), Exception,
               Exception)
    NAPIValueType valueType;
    CHECK_NAPI(napi_typeof(env, referenceValue, &valueType), Common, Exception)
    RETURN_STATUS_IF_FALSE(valueType == NAPIUndefined || valueType == NAPIExternal, NAPIExceptionGenericFailure)
    struct WeakReference *referenceInfo;
    if (valueType == NAPIUndefined)
    {
        referenceInfo = malloc(sizeof(struct WeakReference));
        RETURN_STATUS_IF_FALSE(referenceInfo, NAPIExceptionMemoryError)
        referenceInfo->isEnvFreed = false;
        LIST_INIT(&referenceInfo->weakRefList);
        {
            NAPIExceptionStatus status =
                napi_create_external(env, referenceInfo, referenceFinalize, env, &referenceValue);
            if (__builtin_expect(status != NAPIExceptionOK, false))
            {
                free(referenceInfo);

                return status;
            }
        }
        CHECK_NAPI(napi_set_property(env, value, (NAPIValue)&env->referenceSymbolValue, referenceValue), Exception,
                   Exception)
        LIST_INSERT_HEAD(&env->weakReferenceList, referenceInfo, node);
    }
    else
    {
        CHECK_NAPI(napi_get_value_external(env, referenceValue, (void **)&referenceInfo), Error, Exception)
        if (__builtin_expect(!referenceInfo, false))
        {
            assert(false);

            return NAPIExceptionGenericFailure;
        }
    }
    LIST_INSERT_HEAD(&referenceInfo->weakRefList, ref, node);

    return NAPIExceptionOK;
}

// NAPIMemoryError + setWeak
NAPIExceptionStatus napi_create_reference(NAPIEnv env, NAPIValue value, uint32_t initialRefCount, NAPIRef *result)
{

    NAPI_PREAMBLE(env)
    CHECK_ARG(value, Exception)
    CHECK_ARG(result, Exception)

    *result = malloc(sizeof(struct OpaqueNAPIRef));
    RETURN_STATUS_IF_FALSE(*result, NAPIExceptionMemoryError)
    // 标量 && 弱引用
    if (!JS_IsObject(*((JSValue *)value)) && !initialRefCount)
    {
        (*result)->referenceCount = 0;
        (*result)->value = undefinedValue;
        LIST_INSERT_HEAD(&env->valueList, *result, node);

        return NAPIExceptionOK;
    }
    // 对象 || 强引用
    (*result)->value = *((JSValue *)value);
    (*result)->referenceCount = initialRefCount;
    // 强引用
    if (initialRefCount)
    {
        (*result)->value = JS_DupValue(env->context, (*result)->value);
        LIST_INSERT_HEAD(&env->strongRefList, *result, node);

        return NAPIExceptionOK;
    }
    // 对象 && 弱引用
    // setWeak
    NAPIExceptionStatus status = setWeak(env, value, *result);
    if (__builtin_expect(status != NAPIExceptionOK, false))
    {
        free(*result);

        return status;
    }

    return NAPIExceptionOK;
}

// NAPINameExpected/NAPIGenericFailure + napi_create_string_utf8 + napi_get_property + napi_get_value_external +
// napi_delete_property
static NAPIExceptionStatus clearWeak(NAPIEnv env, NAPIRef ref)
{
    NAPI_PREAMBLE(env)
    CHECK_ARG(ref, Exception)

    RETURN_STATUS_IF_FALSE(JS_IsSymbol(env->referenceSymbolValue), NAPIExceptionNameExpected)

    // TODO(ChasonTang): napi_open_handle_scope()

    NAPIValue externalValue;
    CHECK_NAPI(napi_get_property(env, (NAPIValue)&ref->value, (NAPIValue)&env->referenceSymbolValue, &externalValue),
               Exception, Exception)
    struct WeakReference *referenceInfo;
    CHECK_NAPI(napi_get_value_external(env, externalValue, (void **)&referenceInfo), Error, Exception)
    if (__builtin_expect(!referenceInfo, false))
    {
        assert(false);

        return NAPIExceptionGenericFailure;
    }
    bool needDelete = LIST_FIRST(&referenceInfo->weakRefList) == ref && !LIST_NEXT(ref, node);
    LIST_REMOVE(ref, node);
    if (needDelete)
    {
        bool deleteResult;
        // 一定不会在这里触发 GC
        CHECK_NAPI(
            napi_delete_property(env, (NAPIValue)&ref->value, (NAPIValue)&env->referenceSymbolValue, &deleteResult),
            Exception, Exception)
        RETURN_STATUS_IF_FALSE(deleteResult, NAPIExceptionGenericFailure)
    }

    return NAPIExceptionOK;
}

// NAPIGenericFailure + clearWeak
NAPIExceptionStatus napi_delete_reference(NAPIEnv env, NAPIRef ref)
{

    NAPI_PREAMBLE(env)
    CHECK_ARG(ref, Exception)

    // 标量 && 弱引用（被 GC 也会这样）
    if (!JS_IsObject(ref->value) && !ref->referenceCount)
    {
        LIST_REMOVE(ref, node);
        free(ref);

        return NAPIExceptionOK;
    }
    // 对象 || 强引用
    if (ref->referenceCount)
    {
        LIST_REMOVE(ref, node);
        JS_FreeValue(env->context, ref->value);
        free(ref);

        return NAPIExceptionOK;
    }
    // 对象 && 弱引用
    CHECK_NAPI(clearWeak(env, ref), Exception, Exception)
    free(ref);

    return NAPIExceptionOK;
}

// NAPIGenericFailure + clearWeak
NAPIExceptionStatus napi_reference_ref(NAPIEnv env, NAPIRef ref, uint32_t *result)
{

    NAPI_PREAMBLE(env)
    CHECK_ARG(ref, Exception)

    if (!ref->referenceCount)
    {
        // 弱引用
        JSValue value = JS_DupValue(env->context, ref->value);
        if (JS_IsObject(ref->value))
        {
            CHECK_NAPI(clearWeak(env, ref), Exception, Exception)
        }
        else
        {
            LIST_REMOVE(ref, node);
        }
        LIST_INSERT_HEAD(&env->strongRefList, ref, node);
        ref->value = value;
    }
    uint8_t count = ++ref->referenceCount;
    if (result)
    {
        *result = count;
    }

    return NAPIExceptionOK;
}

// NAPIGenericFailure + setWeak
NAPIExceptionStatus napi_reference_unref(NAPIEnv env, NAPIRef ref, uint32_t *result)
{

    NAPI_PREAMBLE(env)
    CHECK_ARG(ref, Exception)

    RETURN_STATUS_IF_FALSE(ref->referenceCount, NAPIExceptionGenericFailure)

    if (ref->referenceCount == 1)
    {
        LIST_REMOVE(ref, node);
        if (JS_IsObject(ref->value))
        {
            CHECK_NAPI(setWeak(env, (NAPIValue)&ref->value, ref), Exception, Exception)
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

    return NAPIExceptionOK;
}

NAPIErrorStatus napi_get_reference_value(NAPIEnv env, NAPIRef ref, NAPIValue *result)
{

    CHECK_ARG(env, Error)
    CHECK_ARG(ref, Error)
    CHECK_ARG(result, Error)

    if (!ref->referenceCount && JS_IsUndefined(ref->value))
    {
        *result = NULL;
    }
    else
    {
        JSValue strongValue = JS_DupValue(env->context, ref->value);
        struct Handle *handleScope;
        NAPIErrorStatus errorStatus = addValueToHandleScope(env, strongValue, &handleScope);
        if (__builtin_expect(errorStatus != NAPIErrorOK, false))
        {
            JS_FreeValue(env->context, strongValue);

            return errorStatus;
        }
        *result = (NAPIValue)&handleScope->value;
    }

    return NAPIErrorOK;
}

// NAPIMemoryError
NAPIErrorStatus napi_open_handle_scope(NAPIEnv env, NAPIHandleScope *result)
{

    CHECK_ARG(env, Error)
    CHECK_ARG(result, Error)

    NAPIHandleScope handleScope = malloc(sizeof(struct OpaqueNAPIHandleScope));
    RETURN_STATUS_IF_FALSE(handleScope, NAPIErrorMemoryError)
    *result = handleScope;
    SLIST_INIT(&(*result)->handleList);
    LIST_INSERT_HEAD(&env->handleScopeList, *result, node);

    return NAPIErrorOK;
}

NAPICommonStatus napi_close_handle_scope(NAPIEnv env, NAPIHandleScope scope)
{

    CHECK_ARG(env, Common)
    CHECK_ARG(scope, Common)

    // 先入后出 stack 规则
    assert(LIST_FIRST(&env->handleScopeList) == scope &&
           "napi_close_handle_scope() or napi_close_escapable_handle_scope() should follow FILO rule.");
    struct Handle *handle, *tempHandle;
    SLIST_FOREACH_SAFE(handle, &scope->handleList, node, tempHandle)
    {
        JS_FreeValue(env->context, handle->value);
        free(handle);
    }
    // 这里和前面的 assert 要求 env->handleScopeList 必须是 LIST 双向链表
    LIST_REMOVE(scope, node);
    free(scope);

    return NAPICommonOK;
}

struct OpaqueNAPIEscapableHandleScope
{
    struct OpaqueNAPIHandleScope handleScope;
    bool escapeCalled;
};

// NAPIMemoryError
NAPIErrorStatus napi_open_escapable_handle_scope(NAPIEnv env, NAPIEscapableHandleScope *result)
{

    CHECK_ARG(env, Error)
    CHECK_ARG(result, Error)

    // 万一前面的 handleScope 被 close 了，会导致当前 EscapableHandleScope 变成最上层
    // handleScope，这里的判断就没有意义了
    //    RETURN_STATUS_IF_FALSE(LIST_FIRST(&env->handleScopeList), NAPIHandleScopeMismatch);
    *result = malloc(sizeof(struct OpaqueNAPIEscapableHandleScope));
    RETURN_STATUS_IF_FALSE(*result, NAPIErrorMemoryError)
    (*result)->escapeCalled = false;
    SLIST_INIT(&(*result)->handleScope.handleList);
    LIST_INSERT_HEAD(&env->handleScopeList, &(*result)->handleScope, node);

    return NAPIErrorOK;
}

NAPICommonStatus napi_close_escapable_handle_scope(NAPIEnv env, NAPIEscapableHandleScope scope)
{
    return napi_close_handle_scope(env, (NAPIHandleScope)scope);
}

// NAPIMemoryError/NAPIEscapeCalledTwice/NAPIHandleScopeEmpty
NAPIErrorStatus napi_escape_handle(NAPIEnv env, NAPIEscapableHandleScope scope, NAPIValue escapee, NAPIValue *result)
{

    CHECK_ARG(env, Error)
    CHECK_ARG(scope, Error)
    CHECK_ARG(escapee, Error)
    CHECK_ARG(result, Error)

    RETURN_STATUS_IF_FALSE(!scope->escapeCalled, NAPIErrorEscapeCalledTwice)

    NAPIHandleScope handleScope = LIST_NEXT(&scope->handleScope, node);
    RETURN_STATUS_IF_FALSE(handleScope, NAPIErrorHandleScopeEmpty)
    struct Handle *handle = malloc(sizeof(struct Handle));
    RETURN_STATUS_IF_FALSE(handle, NAPIErrorMemoryError)
    scope->escapeCalled = true;
    handle->value = JS_DupValue(env->context, *((JSValue *)escapee));
    SLIST_INSERT_HEAD(&handleScope->handleList, handle, node);
    *result = (NAPIValue)&handle->value;

    return NAPIErrorOK;
}

NAPIExceptionStatus napi_throw(NAPIEnv env, NAPIValue error)
{

    NAPI_PREAMBLE(env)
    CHECK_ARG(error, Exception)

    if (!JS_IsNull(*((JSValue *)error)))
    {
        JS_Throw(env->context, JS_DupValue(env->context, *((JSValue *)error)));
    }
    else
    {
        env->isThrowNull = true;
    }

    return NAPIExceptionOK;
}

// + addValueToHandleScope
NAPIErrorStatus napi_get_and_clear_last_exception(NAPIEnv env, NAPIValue *result)
{

    // 这里不能用 NAPI_PREAMBLE(env)
    CHECK_ARG(env, Error)
    CHECK_ARG(result, Error)

    // JS_GetException
    CHECK_ARG(env->context, Error)

    JSValue exceptionValue = JS_GetException(env->context);
    if (JS_IsNull(exceptionValue) && !env->isThrowNull)
    {
        *result = (NAPIValue)&undefinedValue;

        return NAPIErrorOK;
    }
    if (env->isThrowNull)
    {
        env->isThrowNull = false;
        *result = (NAPIValue)&nullValue;

        return NAPIErrorOK;
    }
    struct Handle *exceptionHandle;
    NAPIErrorStatus status = addValueToHandleScope(env, exceptionValue, &exceptionHandle);
    if (__builtin_expect(status != NAPIErrorOK, false))
    {
        JS_Throw(env->context, exceptionValue);

        return status;
    }
    *result = (NAPIValue)&exceptionHandle->value;

    return NAPIErrorOK;
}

NAPICommonStatus NAPIClearLastException(NAPIEnv env)
{

    // 这里不能用 NAPI_PREAMBLE(env)
    CHECK_ARG(env, Common)

    JS_FreeValue(env->context, JS_GetException(env->context));

    return NAPICommonOK;
}

// NAPIPendingException + addValueToHandleScope
NAPIExceptionStatus NAPIRunScript(NAPIEnv env, const char *script, const char *sourceUrl, NAPIValue *result)
{

    NAPI_PREAMBLE(env)

    // script 传入 NULL 会崩
    if (!script)
    {
        script = "";
    }
    // utf8SourceUrl 传入 NULL 会导致崩溃，JS_NewAtom 调用 strlen，因此不能传入 NULL
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
        else
        {
            JS_Throw(env->context, exceptionValue);
        }

        return NAPIExceptionPendingException;
    }
    processPendingTask(env);
    if (result)
    {
        struct Handle *returnHandle;
        NAPIErrorStatus status = addValueToHandleScope(env, returnValue, &returnHandle);
        if (__builtin_expect(status != NAPIErrorOK, false))
        {
            JS_FreeValue(env->context, returnValue);

            return (NAPIExceptionStatus)status;
        }
        *result = (NAPIValue)&returnHandle->value;
    }
    else
    {
        JS_FreeValue(env->context, returnValue);
    }

    return NAPIExceptionOK;
}

static void functionFinalizer(JSRuntime *rt, JSValue val)
{
    NAPIRuntime runtime = JS_GetRuntimeOpaque(rt);
    if (__builtin_expect(!runtime->functionClassId, false))
    {
        assert(false && FUNCTION_CLASS_ID_ZERO);

        return;
    }
    FunctionInfo *functionInfo = JS_GetOpaque(val, runtime->functionClassId);
    free(functionInfo);
}

static void externalFinalizer(JSRuntime *rt, JSValue val)
{
    NAPIRuntime runtime = JS_GetRuntimeOpaque(rt);
    if (__builtin_expect(!runtime->externalClassId, false))
    {
        assert(false && FUNCTION_CLASS_ID_ZERO);

        return;
    }
    ExternalInfo *externalInfo = JS_GetOpaque(val, runtime->externalClassId);
    if (externalInfo && externalInfo->finalizeCallback)
    {
        externalInfo->finalizeCallback(externalInfo->data, externalInfo->finalizeHint);
    }
    free(externalInfo);
}

// static JSRuntime *runtime = NULL;

typedef struct
{
    FunctionInfo functionInfo;
    JSClassID classId; // uint32_t
} ConstructorInfo;

// static JSClassID constructorClassId = 0;

static void constructorFinalizer(JSRuntime *rt, JSValue val)
{
    NAPIRuntime runtime = JS_GetRuntimeOpaque(rt);
    if (__builtin_expect(!runtime->constructorClassId, false))
    {
        assert(false && CONSTRUCTOR_CLASS_ID_ZERO);

        return;
    }
    ConstructorInfo *constructorInfo = JS_GetOpaque(val, runtime->constructorClassId);
    free(constructorInfo);
}

static JSValue callAsConstructor(JSContext *ctx, JSValueConst newTarget, int argc, JSValueConst *argv)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    NAPIRuntime runtime = JS_GetRuntimeOpaque(rt);
    JSValue prototypeValue = JS_GetPropertyStr(ctx, newTarget, "prototype");
    if (JS_IsException(prototypeValue))
    {
        assert(false && "callAsConstructor() use JS_GetPropertyStr() to get 'prototype' raise a exception.");

        return prototypeValue;
    }
    if (__builtin_expect(!runtime->constructorClassId, false))
    {
        assert(false && CONSTRUCTOR_CLASS_ID_ZERO);

        return undefinedValue;
    }
    ConstructorInfo *constructorInfo = JS_GetOpaque(prototypeValue, runtime->constructorClassId);
    JS_FreeValue(ctx, prototypeValue);
    if (__builtin_expect(!constructorInfo || !constructorInfo->functionInfo.baseInfo.env ||
                             !constructorInfo->functionInfo.callback || !constructorInfo->classId,
                         false))
    {
        assert(false);

        return undefinedValue;
    }
    JSValue thisValue = JS_NewObjectClass(ctx, (int)constructorInfo->classId);
    if (__builtin_expect(JS_IsException(thisValue), false))
    {
        return thisValue;
    }
    struct OpaqueNAPICallbackInfo callbackInfo = {newTarget, thisValue, argv,
                                                  constructorInfo->functionInfo.baseInfo.data, argc};
    NAPIHandleScope handleScope = NULL;
    {
        NAPIErrorStatus status = napi_open_handle_scope(constructorInfo->functionInfo.baseInfo.env, &handleScope);
        assert(status == NAPIErrorOK);
        (void)status;
    }
    NAPIValue retVal =
        constructorInfo->functionInfo.callback(constructorInfo->functionInfo.baseInfo.env, &callbackInfo);
    if (retVal && JS_IsObject(*((JSValue *)retVal)))
    {
        JSValue returnValue = JS_DupValue(ctx, *((JSValue *)retVal));
        JS_FreeValue(ctx, thisValue);
        thisValue = returnValue;
    }
    if (handleScope)
    {
        NAPICommonStatus commonStatus =
            napi_close_handle_scope(constructorInfo->functionInfo.baseInfo.env, handleScope);
        if (__builtin_expect(commonStatus != NAPICommonOK, false))
        {
            JS_FreeValue(ctx, thisValue);
            assert(false && NAPI_CLOSE_HANDLE_SCOPE_ERROR);

            // TODO(ChasonTang): 验证 undefined 的情况
            return undefinedValue;
        }
    }
    // 异常处理
    JSValue exceptionValue = JS_GetException(ctx);
    if (!JS_IsNull(exceptionValue))
    {
        JS_FreeValue(ctx, thisValue);

        return JS_Throw(ctx, exceptionValue);
    }
    if (constructorInfo->functionInfo.baseInfo.env->isThrowNull)
    {
        JS_FreeValue(ctx, thisValue);
        constructorInfo->functionInfo.baseInfo.env->isThrowNull = false;

        return JS_EXCEPTION;
    }

    return thisValue;
}

// NAPIMemoryError/NAPIPendingException + addValueToHandleScope
NAPIExceptionStatus NAPIDefineClass(NAPIEnv env, const char *utf8name, NAPICallback constructor, void *data,
                                    NAPIValue *result)
{

    NAPI_PREAMBLE(env)
    CHECK_ARG(constructor, Exception)
    CHECK_ARG(result, Exception)

    ConstructorInfo *constructorInfo = malloc(sizeof(ConstructorInfo));
    RETURN_STATUS_IF_FALSE(constructorInfo, NAPIExceptionMemoryError)
    constructorInfo->functionInfo.baseInfo.env = env;
    constructorInfo->functionInfo.baseInfo.data = data;
    constructorInfo->functionInfo.callback = constructor;
    constructorInfo->classId = 0;
    JS_NewClassID(&constructorInfo->classId);
    JSClassDef classDef = {utf8name ?: "", NULL, NULL, NULL, NULL};
    int status = JS_NewClass(env->runtime->runtime, constructorInfo->classId, &classDef);
    if (__builtin_expect(status == -1, false))
    {
        free(constructorInfo);

        return NAPIExceptionPendingException;
    }
    if (__builtin_expect(!env->runtime->constructorClassId, false))
    {
        assert(false && CONSTRUCTOR_CLASS_ID_ZERO);

        return NAPIExceptionGenericFailure;
    }
    JSValue prototype = JS_NewObjectClass(env->context, (int)env->runtime->constructorClassId);
    if (__builtin_expect(JS_IsException(prototype), false))
    {
        free(constructorInfo);

        return NAPIExceptionPendingException;
    }
    JS_SetOpaque(prototype, constructorInfo);
    // JS_NewCFunction2 -> JS_NewCFunction3 会正确处理 utf8name 为空情况
    JSValue constructorValue = JS_NewCFunction2(env->context, callAsConstructor, utf8name, 0, JS_CFUNC_constructor, 0);
    if (__builtin_expect(JS_IsException(constructorValue), false))
    {
        // prototype 会自己 free ConstructorInfo 结构体
        JS_FreeValue(env->context, prototype);

        return NAPIExceptionPendingException;
    }
    struct Handle *handle;
    NAPIErrorStatus addStatus = addValueToHandleScope(env, constructorValue, &handle);
    if (__builtin_expect(addStatus != NAPIErrorOK, false))
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

    return NAPIExceptionOK;
}

NAPIErrorStatus NAPICreateRuntime(NAPIRuntime *runtime)
{
    CHECK_ARG(runtime, Error)

    *runtime = malloc(sizeof(struct OpaqueNAPIRuntime));
    RETURN_STATUS_IF_FALSE(*runtime, NAPIErrorMemoryError)
    (*runtime)->runtime = JS_NewRuntime();
    // JS_NewClassID only accept 0.
    // So we initialize classId field to 0.
    (*runtime)->constructorClassId = 0;
    (*runtime)->functionClassId = 0;
    (*runtime)->externalClassId = 0;
    if (!(*runtime)->runtime)
    {
        free(*runtime);

        return NAPIErrorMemoryError;
    }
    JS_SetRuntimeOpaque((*runtime)->runtime, *runtime);
    JS_SetMaxStackSize((*runtime)->runtime, 4 * JS_DEFAULT_STACK_SIZE);
    // 一定成功
    JS_NewClassID(&(*runtime)->constructorClassId);
    JS_NewClassID(&(*runtime)->functionClassId);
    JS_NewClassID(&(*runtime)->externalClassId);
    JSClassDef classDef = {"External", externalFinalizer, NULL, NULL, NULL};
    // JS_NewClass -> JS_NewClass1 返回值只有 -1 和 0
    int status = JS_NewClass((*runtime)->runtime, (*runtime)->externalClassId, &classDef);
    if (__builtin_expect(status == -1, false))
    {
        JS_FreeRuntime((*runtime)->runtime);
        free(*runtime);

        return NAPIErrorGenericFailure;
    }

    classDef.class_name = "FunctionData";
    classDef.finalizer = functionFinalizer;
    status = JS_NewClass((*runtime)->runtime, (*runtime)->functionClassId, &classDef);
    if (__builtin_expect(status == -1, false))
    {
        JS_FreeRuntime((*runtime)->runtime);
        free(*runtime);

        return NAPIErrorGenericFailure;
    }

    classDef.class_name = "ConstructorPrototype";
    classDef.finalizer = constructorFinalizer;
    status = JS_NewClass((*runtime)->runtime, (*runtime)->constructorClassId, &classDef);
    if (__builtin_expect(status == -1, false))
    {
        JS_FreeRuntime((*runtime)->runtime);
        free(*runtime);

        return NAPIErrorGenericFailure;
    }

    return NAPIErrorOK;
}

// NAPIGenericFailure/NAPIMemoryError
NAPIErrorStatus NAPICreateEnv(NAPIEnv *env, NAPIRuntime runtime)
{
    CHECK_ARG(env, Error)
    CHECK_ARG(runtime, Error)

    // Resource - NAPIEnv
    *env = malloc(sizeof(struct OpaqueNAPIEnv));
    RETURN_STATUS_IF_FALSE(*env, NAPIErrorMemoryError)
    (*env)->runtime = runtime;

    // Resource - JSContext
    JSContext *context = JS_NewContext(runtime->runtime);
    //    js_std_add_helpers(context, 0, NULL);
    if (__builtin_expect(!context, false))
    {
        free(*env);

        return NAPIErrorMemoryError;
    }
    JSValue prototype = JS_NewObject(context);
    if (__builtin_expect(JS_IsException(prototype), false))
    {
        JS_FreeContext(context);
        free(*env);

        return NAPIErrorGenericFailure;
    }
    JS_SetClassProto(context, runtime->externalClassId, prototype);
    prototype = JS_NewObject(context);
    if (__builtin_expect(JS_IsException(prototype), false))
    {
        JS_FreeContext(context);
        free(*env);

        return NAPIErrorGenericFailure;
    }
    JS_SetClassProto(context, runtime->constructorClassId, prototype);
    const char *string = "(function () { return Symbol(\"reference\") })();";
    (*env)->referenceSymbolValue =
        JS_Eval(context, string, strlen(string), "https://n-api.com/qjs_reference_symbol.js", JS_EVAL_TYPE_GLOBAL);
    if (__builtin_expect(JS_IsException((*env)->referenceSymbolValue), false))
    {
        JS_FreeContext(context);
        free(*env);

        return NAPIErrorGenericFailure;
    }
    (*env)->context = context;
    (*env)->isThrowNull = false;
    LIST_INIT(&(*env)->handleScopeList);
    LIST_INIT(&(*env)->weakReferenceList);
    LIST_INIT(&(*env)->valueList);
    LIST_INIT(&(*env)->strongRefList);

    return NAPIErrorOK;
}

NAPICommonStatus NAPIFreeEnv(NAPIEnv env)
{
    CHECK_ARG(env, Common)

    NAPIHandleScope handleScope, tempHandleScope;
    LIST_FOREACH_SAFE(handleScope, &env->handleScopeList, node, tempHandleScope)
    {
        struct Handle *handle, *tempHandle;
        SLIST_FOREACH_SAFE(handle, &handleScope->handleList, node, tempHandle)
        {
            JS_FreeValue(env->context, handle->value);
            free(handle);
        }
        // 这里和前面的 assert 要求 env->handleScopeList 必须是 LIST 双向链表
        LIST_REMOVE(handleScope, node);
        free(handleScope);
    }
    NAPIRef ref, temp;
    LIST_FOREACH_SAFE(ref, &env->strongRefList, node, temp)
    {
        LIST_REMOVE(ref, node);
        JS_FreeValue(env->context, ref->value);
        free(ref);
    }
    struct WeakReference *referenceInfo, *tempReferenceInfo;
    LIST_FOREACH_SAFE(referenceInfo, &env->weakReferenceList, node, tempReferenceInfo)
    {
        LIST_FOREACH_SAFE(ref, &referenceInfo->weakRefList, node, temp)
        {
            LIST_REMOVE(ref, node);
            free(ref);
        }
        LIST_REMOVE(referenceInfo, node);
        referenceInfo->isEnvFreed = true;
        // referenceInfo 本身不销毁，等到 GC 阶段销毁
    }
    // 到这一步，所有弱引用已经全部释放完成
    LIST_FOREACH_SAFE(ref, &env->valueList, node, temp)
    {
        LIST_REMOVE(ref, node);
        free(ref);
    }
    JS_FreeValue(env->context, env->referenceSymbolValue);
    JS_FreeContext(env->context);
    free(env);

    return NAPICommonOK;
}

NAPICommonStatus NAPIFreeRuntime(NAPIRuntime runtime)
{
    CHECK_ARG(runtime, Common)

    JS_FreeRuntime(runtime->runtime);

    return NAPICommonOK;
}

NAPIErrorStatus NAPIGetValueStringUTF8(NAPIEnv env, NAPIValue value, const char **result)
{

    CHECK_ARG(env, Error)
    CHECK_ARG(value, Error)
    CHECK_ARG(result, Error)

    RETURN_STATUS_IF_FALSE(JS_IsString(*((JSValue *)value)), NAPIErrorStringExpected);

    *result = JS_ToCString(env->context, *((JSValue *)value));

    return NAPIErrorOK;
}

NAPICommonStatus NAPIFreeUTF8String(NAPIEnv env, const char *cString)
{

    CHECK_ARG(env, Common)

    JS_FreeCString(env->context, cString);

    return NAPICommonOK;
}

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

NAPI_EXPORT NAPIExceptionStatus NAPICompileToByteBuffer(NAPIEnv env, const char *script, const char *sourceUrl,
                                                        const uint8_t **byteBuffer, size_t *bufferSize)
{
    NAPI_PREAMBLE(env)
    CHECK_ARG(byteBuffer, Exception)

    if (!script)
    {
        script = "";
    }
    if (!sourceUrl)
    {
        sourceUrl = "";
    }
    JSValue returnValue =
        JS_Eval(env->context, script, strlen(script), sourceUrl, JS_EVAL_FLAG_COMPILE_ONLY | JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(returnValue))
    {
        goto exceptionHandler;
    }
    *byteBuffer = JS_WriteObject(env->context, bufferSize, returnValue, JS_WRITE_OBJ_BYTECODE);
    JS_FreeValue(env->context, returnValue);
    if (!*byteBuffer)
    {
        goto exceptionHandler;
    }

    return NAPIExceptionOK;

exceptionHandler : {
    JSValue exceptionValue = JS_GetException(env->context);
    if (JS_IsNull(exceptionValue))
    {
        env->isThrowNull = true;
    }
    else
    {
        JS_Throw(env->context, exceptionValue);
    }
}

    return NAPIExceptionPendingException;
}

NAPI_EXPORT NAPICommonStatus NAPIFreeByteBuffer(NAPIEnv env, const uint8_t *byteBuffer)
{
    CHECK_ARG(env, Common)

    js_free(env->context, (void *)byteBuffer);

    return NAPICommonOK;
}

NAPI_EXPORT NAPIExceptionStatus NAPIRunByteBuffer(NAPIEnv env, const uint8_t *byteBuffer, size_t bufferSize,
                                                  NAPIValue *result)
{
    NAPI_PREAMBLE(env)

    JSValue functionValue = JS_ReadObject(env->context, byteBuffer, bufferSize, JS_READ_OBJ_BYTECODE);
    if (JS_IsException(functionValue))
    {
        goto exceptionHandler;
    }

    JSValue returnValue = JS_EvalFunction(env->context, functionValue);
    if (JS_IsException(returnValue))
    {
        goto exceptionHandlerWithProcess;
    }
    processPendingTask(env);
    if (result)
    {
        struct Handle *returnHandle;
        NAPIErrorStatus status = addValueToHandleScope(env, returnValue, &returnHandle);
        if (__builtin_expect(status != NAPIErrorOK, false))
        {
            JS_FreeValue(env->context, returnValue);

            return (NAPIExceptionStatus)status;
        }
        *result = (NAPIValue)&returnHandle->value;
    }
    else
    {
        JS_FreeValue(env->context, returnValue);
    }

    return NAPIExceptionOK;

exceptionHandler : {
    JSValue exceptionValue = JS_GetException(env->context);
    if (JS_IsNull(exceptionValue))
    {
        env->isThrowNull = true;
    }
    else
    {
        JS_Throw(env->context, exceptionValue);
    }
}
    return NAPIExceptionPendingException;

exceptionHandlerWithProcess : {
    JSValue exceptionValue = JS_GetException(env->context);
    processPendingTask(env);
    if (JS_IsNull(exceptionValue))
    {
        env->isThrowNull = true;
    }
    else
    {
        JS_Throw(env->context, exceptionValue);
    }
}

    return NAPIExceptionPendingException;
}