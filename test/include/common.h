#ifndef common_h
#define common_h

#include <napi/js_native_api_types.h>

EXTERN_C_START

#define GET_AND_THROW_LAST_ERROR(env)                                    \
  do {                                                                   \
    const NAPIExtendedErrorInfo *errorInfo;                          \
    napi_get_last_error_info((env), &errorInfo);                        \
    bool isPending;                                                     \
    napi_is_exception_pending((env), &isPending);                       \
    /* If an exception is already pending, don't rethrow it */           \
    if (!isPending) {                                                   \
      const char* errorMessage = errorInfo->errorMessage != NULL ?    \
        errorInfo->errorMessage :                                      \
        "empty error message";                                           \
      napi_throw_error((env), NULL, errorMessage);                      \
    }                                                                    \
  } while (0)

#define NAPI_CALL_BASE(env, theCall, retVal)                           \
  do {                                                                   \
    if ((theCall) != NAPIOK) {                                         \
      GET_AND_THROW_LAST_ERROR((env));                                   \
      return retVal;                                                    \
    }                                                                    \
  } while (0)

// Returns NULL if the_call doesn't return napi_ok.
#define NAPI_CALL(env, theCall)                                         \
  NAPI_CALL_BASE(env, theCall, NULL)

#define NAPI_ASSERT_BASE(env, assertion, message, retVal)               \
  do {                                                                   \
    if (!(assertion)) {                                                  \
      napi_throw_error(                                                  \
          (env),                                                         \
        NULL,                                                            \
          "assertion (" #assertion ") failed: " message);                \
      return retVal;                                                    \
    }                                                                    \
  } while (0)

// Returns NULL on failed assertion.
// This is meant to be used inside napi_callback methods.
#define NAPI_ASSERT(env, assertion, message)                             \
  NAPI_ASSERT_BASE(env, assertion, message, NULL)

#define DECLARE_NAPI_PROPERTY(name, func)                                \
  { (name), NULL, (func), NULL, NULL, NULL, NAPIDefault, NULL }

static NAPIValue assertFunction(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValue value;
    NAPI_CALL(env, napi_coerce_to_bool(env, args[0], &value));

    bool result = false;
    NAPI_CALL(env, napi_get_value_bool(env, value, &result));
    NAPI_ASSERT(env, result, "assert(false)");

    return NULL;
}

static inline NAPIStatus initAssert(NAPIEnv env, NAPIValue global) {
    NAPIPropertyDescriptor desc = DECLARE_NAPI_PROPERTY("assert", assertFunction);

    return napi_define_properties(env, global, 1, &desc);
}

EXTERN_C_END

#endif /* common_h */
