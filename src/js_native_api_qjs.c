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

// JS_GetException 会转移所有权
// JS_Throw 会获取所有权，所以如果存在异常，先获取所有权，再交还所有权，最终所有权依旧在 context 中
#define NAPI_PREAMBLE(env)                                                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        CHECK_ENV(env);                                                                                                \
        JSValue exceptionValue = JS_GetException((env)->context);                                                      \
        if (!JS_IsUndefined(exceptionValue) && !JS_IsNull(exceptionValue))                                             \
        {                                                                                                              \
            JS_Throw((env)->context, exceptionValue);                                                                  \
                                                                                                                       \
            return setLastErrorCode(env, NAPIPendingException);                                                        \
        }                                                                                                              \
        JS_FreeValue((env)->context, exceptionValue);                                                                  \
        clearLastError(env);                                                                                           \
    } while (0)

#define CHECK_ARG(env, arg) RETURN_STATUS_IF_FALSE(env, arg, NAPIInvalidArg)

struct Handle
{
    JSValue value;            // 64/128
    SLIST_ENTRY(Handle) node; // 32/64
};

struct OpaqueNAPIHandleScope
{
    SLIST_ENTRY(OpaqueNAPIHandleScope) node; // 32/64
    SLIST_HEAD(, Handle) handleList;         // 32/64
};

// typedef struct
//{
//     const char *errorMessage; // 32/64
//     void *engineReserved; // 32/64
//     uint32_t engineErrorCode; // 32
//     NAPIStatus errorCode;
// } NAPIExtendedErrorInfo;
struct OpaqueNAPIEnv
{
    JSContext *context;                                  // 32/64
    SLIST_HEAD(, OpaqueNAPIHandleScope) handleScopeList; // 32/64
    NAPIExtendedErrorInfo lastError;
};

// static inline
NAPIStatus setLastErrorCode(NAPIEnv env, NAPIStatus errorCode)
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

// 这个函数不会修改引用计数和所有权
static inline struct Handle *addValueToHandleScope(NAPIEnv env, JSValue value)
{
    if (!env)
    {
        return NULL;
    }
    if (SLIST_EMPTY(&(*env).handleScopeList))
    {
        return NULL;
    }
    struct Handle *handle = malloc(sizeof(struct Handle));
    if (!handle)
    {
        return NULL;
    }
    handle->value = value;
    NAPIHandleScope handleScope = SLIST_FIRST(&(env)->handleScopeList);
    SLIST_INSERT_HEAD(&handleScope->handleList, handle, node);

    return handle;
}

NAPIStatus napi_get_global(NAPIEnv env, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    // JS_GetGlobalObject 不能传入 NULL
    CHECK_ARG(env, env->context);

    // 转移所有权
    JSValue globalValue = JS_GetGlobalObject(env->context);
    struct Handle *globalHandle = addValueToHandleScope(env, globalValue);
    if (!globalHandle)
    {
        JS_FreeValue(env->context, globalValue);

        return setLastErrorCode(env, NAPIMemoryError);
    }
    *result = (NAPIValue)&globalHandle->value;

    return clearLastError(env);
}

NAPIStatus napi_create_object(NAPIEnv env, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    // JS_NewObject 不能传入 NULL
    CHECK_ARG(env, env->context);

    // 初始引用计数为 1
    JSValue objectValue = JS_NewObject(env->context);
    struct Handle *objectHandle = addValueToHandleScope(env, objectValue);
    if (!objectHandle)
    {
        JS_FreeValue(env->context, objectValue);

        return setLastErrorCode(env, NAPIMemoryError);
    }
    *result = (NAPIValue)&objectHandle->value;

    return clearLastError(env);
}

NAPIStatus napi_create_double(NAPIEnv env, double value, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    // JS_NewFloat64 实际上不关心 env->context
    JSValue jsValue = JS_NewFloat64(env->context, value);
    struct Handle *handle = addValueToHandleScope(env, jsValue);
    if (!handle)
    {
        // 实际上不起作用
        JS_FreeValue(env->context, jsValue);

        return setLastErrorCode(env, NAPIMemoryError);
    }
    *result = (NAPIValue)&handle->value;

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

    RETURN_STATUS_IF_FALSE(env, JS_IsObject(*((JSValue *)object)), NAPIObjectExpected);
    // TODO(ChasonTang): utf8name 传入 NULL 是否会崩溃？
    // JS_SetPropertyStr 会获取所有权，因此需要 JS_DupValue，并且最好使用返回值传递
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
    struct Handle *exceptionHandle = addValueToHandleScope(env, exceptionValue);
    if (!exceptionHandle)
    {
        JS_FreeValue(env->context, exceptionValue);

        return setLastErrorCode(env, NAPIMemoryError);
    }
    *result = (NAPIValue)&exceptionHandle->value;

    return clearLastError(env);
}

NAPIStatus NAPIRunScriptWithSourceUrl(NAPIEnv env, const char *utf8Script, const char *utf8SourceUrl, NAPIValue *result)
{
    CHECK_ENV(env);
    JSValue exceptionValue = JS_GetException((env)->context);
    if (!JS_IsUndefined(exceptionValue) && !JS_IsNull(exceptionValue))
    {
        JS_Throw((env)->context, exceptionValue);

        return setLastErrorCode(env, NAPIPendingException);
    }
    JS_FreeValue(env->context, exceptionValue);
    clearLastError(env);
    //    NAPI_PREAMBLE(env);
    CHECK_ARG(env, utf8Script);
    CHECK_ARG(env, result);

    // JS_Eval/JS_GetRuntime 不能传入 NULL
    CHECK_ARG(env, env->context);

    // TODO(ChasonTang): utf8SourceUrl 传入 NULL 是否会导致崩溃？
    JSValue returnValue = JS_Eval(env->context, utf8Script, strlen(utf8Script), utf8SourceUrl, JS_EVAL_TYPE_GLOBAL);
    for (int err = 1; err > 0;)
    {
        JSContext *context;
        err = JS_ExecutePendingJob(JS_GetRuntime(env->context), &context);
        if (err)
        {
            // 正常情况下 JS_ExecutePendingJob 返回 -1 代表引擎内部异常，比如内存分配失败等，未来如果真产生了 JS
            // 异常，也直接吃掉
            JSValue exceptionValue = JS_GetException(context);
            JS_FreeValue(context, exceptionValue);
        }
    }
    //    if (JS_IsException(returnValue))
    //    {
    //        js_std_dump_error(env->context);
    //
    //        return setLastErrorCode(env, NAPIPendingException);
    //    }
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(returnValue), NAPIPendingException);
    struct Handle *returnHandle = addValueToHandleScope(env, returnValue);
    if (!returnHandle)
    {
        JS_FreeValue(env->context, returnValue);

        return setLastErrorCode(env, NAPIMemoryError);
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