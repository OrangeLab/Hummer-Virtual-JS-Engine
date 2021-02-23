//
//  js_native_api_jsc.c
//  NAPI
//
//  Created by 唐佳诚 on 2021/2/23.
//

#include <assert.h>
#include <stdio.h>
#include <stdnoreturn.h>
#include <stdlib.h>
#include <JavaScriptCore/JavaScriptCore.h>

#include "js_native_api_jsc.h"

#define NAPI_EXPERIMENTAL

#include "js_native_api.h"

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

typedef struct {
    const char *file_line;  // filename:line
    const char *message;
    const char *function;
} AssertionInfo;

_Noreturn static void Assert(const AssertionInfo *info) {
    if (info) {
        fprintf(stderr,
                "%s:%s%s Assertion `%s' failed.\n",
                info->file_line,
                info->function,
                (*info).function ? ":" : "",
                info->message);
        fflush(stderr);
    }

    abort();
}

#define ERROR_AND_ABORT(expr)                                                 \
  do {                                                                        \
    /* Make sure that this struct does not end up in inline code, but      */ \
    /* rather in a read-only data section when modifying this code.        */ \
    static const AssertionInfo args = {                                 \
      __FILE__ ":" STRINGIFY(__LINE__), #expr, PRETTY_FUNCTION_NAME           \
    };                                                                        \
    Assert(&args);                                                       \
  } while (0)

#ifdef __GNUC__
#define LIKELY(expr) __builtin_expect(!!(expr), 1)
#define UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#define PRETTY_FUNCTION_NAME __PRETTY_FUNCTION__
#else
#define LIKELY(expr) expr
#define UNLIKELY(expr) expr
#define PRETTY_FUNCTION_NAME ""
#endif

#define CHECK(expr)                                                           \
  do {                                                                        \
    if (UNLIKELY(!(expr))) {                                                  \
      ERROR_AND_ABORT(expr);                                                  \
    }                                                                         \
  } while (0)

//#define CHECK_LE(a, b) CHECK((a) <= (b))
#define CHECK_LT(a, b) CHECK((a) < (b))

struct napi_env__ {
    JSContextRef const contextRef;
    napi_extended_error_info last_error;
};

static inline napi_status napi_set_last_error_code(napi_env env, napi_status error_code) {
    env->last_error.error_code = error_code;

    return error_code;
}

#define RETURN_STATUS_IF_FALSE(env, condition, status)                  \
  do {                                                                  \
    if (!(condition)) {                                                 \
      return napi_set_last_error_code((env), (status));                      \
    }                                                                   \
  } while (0)

#define CHECK_ARG(env, arg) \
  RETURN_STATUS_IF_FALSE((env), ((arg) != NULL), napi_invalid_arg)

#define CHECK_ENV(env)          \
  do {                          \
    if ((env) == NULL) {        \
      return napi_invalid_arg;  \
    }                           \
  } while (0)

// Warning: Keep in-sync with napi_status enum

static const char *error_messages[] = {
        NULL,
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
        "Main thread would deadlock"
};

napi_status napi_get_last_error_info(napi_env env, const napi_extended_error_info **result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    // The value of the constant below must be updated to reference the last
    // message in the `napi_status` enum each time a new error message is added.
    // We don't have a napi_status_last as this would result in an ABI
    // change each time a message was added.
//    const int last_status = napi_would_deadlock;

    static_assert(sizeof(error_messages) / sizeof(*error_messages) == napi_status_last, "Count of error messages must match count of error values");
    CHECK_LT(env->last_error.error_code, napi_status_last);

    // Wait until someone requests the last error information to fetch the error
    // message string
    env->last_error.error_message = error_messages[env->last_error.error_code];

    *result = &(env->last_error);

    return napi_ok;
}

napi_status napi_get_undefined(napi_env env, napi_value *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    // JSValueMakeUndefined 不能传入 NULL，否则触发 release_assert
    CHECK_ARG(env, env->contextRef);

    *result = (napi_value) JSValueMakeUndefined(env->contextRef);

    return napi_ok;
}

napi_status napi_get_null(napi_env env, napi_value *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    // JSValueMakeNull 不能传入 NULL，否则触发 release_assert
    CHECK_ARG(env, env->contextRef);

    *result = (napi_value) JSValueMakeNull(env->contextRef);

    return napi_ok;
}

napi_status napi_get_global(napi_env env, napi_value *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    // JSContextGetGlobalObject 不能传入 NULL，否则触发 release_assert
    CHECK_ARG(env, env->contextRef);

    *result = (napi_value) JSContextGetGlobalObject(env->contextRef);

    return napi_ok;
}

napi_status napi_get_boolean(napi_env env, bool value, napi_value *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    // JSValueMakeBoolean 不能传入 NULL，否则触发 release_assert
    CHECK_ARG(env, env->contextRef);

    *result = (napi_value) JSValueMakeBoolean(env->contextRef, value);

    return napi_ok;
}

napi_status napi_create_object(napi_env env, napi_value *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    // JSObjectMake 不能传入 NULL，否则触发 release_assert
    CHECK_ARG(env, env->contextRef);

    // TODO(ChasonTang): 低版本 JavaScriptCore 是否允许 jsClass 为 NULL
    *result = (napi_value) JSObjectMake(env->contextRef, NULL, NULL);

    return napi_ok;
}

//napi_status napi_create_array(napi_env env, napi_value *result) {
//    CHECK_ENV(env);
//    CHECK_ARG(env, result);
//
//    // JSObjectMakeArray 不能传入 NULL，否则触发 release_assert
//    CHECK_ARG(env, env->contextRef);
//
//    JSValueRef exception = NULL;
//    *result = (napi_value) JSObjectMakeArray(env->contextRef, 0, NULL, &exception);
//
//    return napi_clear_last_error(env);
//}