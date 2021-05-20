#include <quickjs.h>
#include <napi/js_native_api.h>
#include <assert.h>
#include <stdlib.h>

#define CHECK_ENV(env)             \
    do                             \
    {                              \
        if (!(env))         \
        {                          \
            return NAPIInvalidArg; \
        }                          \
    } while (0)

static inline NAPIStatus setLastErrorCode(NAPIEnv env, NAPIStatus errorCode);

#define RETURN_STATUS_IF_FALSE(env, condition, status) \
    do                                                 \
    {                                                  \
        if (!(condition))                              \
        {                                              \
            return setLastErrorCode(env, status);  \
        }                                              \
    } while (0)

#define CHECK_ARG(env, arg) RETURN_STATUS_IF_FALSE(env, arg, NAPIInvalidArg)

struct OpaqueNAPIEnv {
    JSContext *context;
    NAPIExtendedErrorInfo lastError;
};

NAPIStatus setLastErrorCode(NAPIEnv env, NAPIStatus errorCode) {
    CHECK_ENV(env);
    env->lastError.errorCode = errorCode;

    return errorCode;
}

static JSRuntime *runtime = NULL;

static uint8_t contextCount = 0;

NAPIStatus NAPICreateEnv(NAPIEnv *env) {
    if (!env) {
        return NAPIInvalidArg;
    }
    if ((runtime && !contextCount) || (!runtime && contextCount)) {
        assert(false);

        return NAPIGenericFailure;
    }
    *env = malloc(sizeof(struct OpaqueNAPIEnv));
    if (!*env) {
        return NAPIMemoryError;
    }
    if (!runtime) {
        runtime = JS_NewRuntime();
        if (!runtime) {
            free(*env);

            return NAPIMemoryError;
        }
    }
    JSContext *context = JS_NewContext(runtime);
    if (!context) {
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

    return NAPIOK;
}

NAPIStatus NAPIFreeEnv(NAPIEnv env) {
    CHECK_ENV(env);

    JS_FreeContext(env->context);
    if (--contextCount == 0 && runtime) {
        // virtualMachine 不能为 NULL
        JS_FreeRuntime(runtime);
        runtime = NULL;
    }
    free(env);

    return NAPIOK;
}

NAPIStatus
NAPIRunScriptWithSourceUrl(NAPIEnv env, const char *utf8Script, const char *utf8SourceUrl, NAPIValue *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, utf8Script);
    CHECK_ARG(env, result);

    JSValue JS_Eval()
}