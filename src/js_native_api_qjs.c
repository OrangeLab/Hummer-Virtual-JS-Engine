#include <assert.h>
#include <napi/js_native_api.h>
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

#define NAPI_PREAMBLE(env)                                                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        CHECK_ENV(env);                                                                                                \
        RETURN_STATUS_IF_FALSE(                                                                                        \
            env, JS_IsUndefined(JS_GetException((env)->context)) || JS_IsNull(JS_GetException((env)->context)),        \
            NAPIPendingException);                                                                                     \
        clearLastError(env);                                                                                           \
    } while (0)

#define CHECK_ARG(env, arg) RETURN_STATUS_IF_FALSE(env, arg, NAPIInvalidArg)

struct Handle
{
    JSValue value;            // 128/64
    SLIST_ENTRY(Handle) node; // 64/32
};

struct OpaqueNAPIHandleScope
{
    SLIST_ENTRY(OpaqueNAPIHandleScope) node; // 64/32
    SLIST_HEAD(, Handle) handleList;         // 64/32
};

struct OpaqueNAPIEnv
{
    JSContext *context;                                  // 64/32
    SLIST_HEAD(, OpaqueNAPIHandleScope) handleScopeList; // 64/32
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

static inline bool addValueToHandleScope(NAPIEnv env, JSValue value)
{
    if (!env)
    {
        return false;
    }
    if (SLIST_EMPTY(&(*env).handleScopeList))
    {
        return false;
    }
    struct Handle *handle = malloc(sizeof(struct Handle));
    if (!handle)
    {
        return false;
    }
    handle->value = value;
    NAPIHandleScope handleScope = SLIST_FIRST(&(env)->handleScopeList);
    SLIST_INSERT_HEAD(&handleScope->handleList, handle, node);

    return true;
}

typedef union {
    NAPIValue store;
    JSValue value;
} ConvertUnion;

NAPIStatus napi_get_global(NAPIEnv env, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    // JS_GetGlobalObject 不能传入 NULL
    CHECK_ARG(env, env->context);

    // 转移所有权
    ConvertUnion convertUnion = {.value = JS_GetGlobalObject(env->context)};
    RETURN_STATUS_IF_FALSE(env, addValueToHandleScope(env, convertUnion.value), NAPIMemoryError);
    *result = convertUnion.store;

    return clearLastError(env);
}

NAPIStatus napi_create_object(NAPIEnv env, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    // JS_NewObject 不能传入 NULL
    CHECK_ARG(env, env->context);

    ConvertUnion convertUnion = {.value = JS_NewObject(env->context)};
    RETURN_STATUS_IF_FALSE(env, addValueToHandleScope(env, convertUnion.value), NAPIMemoryError);
    *result = convertUnion.store;

    return clearLastError(env);
}

NAPIStatus napi_set_named_property(NAPIEnv env, NAPIValue object, const char *utf8name, NAPIValue value)
{
    NAPI_PREAMBLE(env);
    //    CHECK_ARG(env, value);

    ConvertUnion objectConvertUnion = {object};
    RETURN_STATUS_IF_FALSE(env, JS_IsObject(objectConvertUnion.value), NAPIObjectExpected);

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

    if (SLIST_FIRST(&env->handleScopeList) != scope)
    {
        return setLastErrorCode(env, NAPIHandleScopeMismatch);
    }
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

NAPIStatus NAPIRunScriptWithSourceUrl(NAPIEnv env, const char *utf8Script, const char *utf8SourceUrl, NAPIValue *result)
{
    CHECK_ENV(env);
    CHECK_ARG(env, utf8Script);
    CHECK_ARG(env, result);

    // JS_Eval/JS_GetRuntime 不能传入 NULL
    CHECK_ARG(env, env->context);

    // TODO(ChasonTang): utf8SourceUrl 不知道是否传入 NULL 会导致崩溃？
    ConvertUnion convertUnion = {
        .value = JS_Eval(env->context, utf8Script, strlen(utf8Script), utf8SourceUrl, JS_EVAL_TYPE_GLOBAL)};
    for (int err = 1; err > 0;)
    {
        JSContext *context;
        err = JS_ExecutePendingJob(JS_GetRuntime(env->context), &context);
        if (err)
        {
            // 转移所有权
            JSValue exceptionValue = JS_GetException(context);
            JS_FreeValue(context, exceptionValue);
        }
    }
    RETURN_STATUS_IF_FALSE(env, !JS_IsException(convertUnion.value), NAPIPendingException);
    RETURN_STATUS_IF_FALSE(env, addValueToHandleScope(env, convertUnion.value), NAPIMemoryError);
    *result = convertUnion.store;

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
        NAPIStatus status = napi_close_handle_scope(env, handleScope);
        if (status != NAPIOK)
        {
            return status;
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