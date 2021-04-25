#ifndef common_h
#define common_h

EXTERN_C_START

#include <napi/js_native_api.h>

// Empty value so that macros here are able to return NULL or void
#define NAPI_RETVAL_NOTHING  // Intentionally blank #define

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

// Returns empty if the_call doesn't return napi_ok.
#define NAPI_CALL_RETURN_VOID(env, the_call)                             \
  NAPI_CALL_BASE(env, the_call, NAPI_RETVAL_NOTHING)

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

#define DECLARE_NAPI_GETTER(name, func)                                  \
  { (name), NULL, NULL, (func), NULL, NULL, NAPIDefault, NULL }

void add_returned_status(NAPIEnv env,
                         const char *key,
                         NAPIValue object,
                         const char *expected_message,
                         NAPIStatus expected_status,
                         NAPIStatus actual_status); // expected_message 没做修改，添加 const

void add_last_status(NAPIEnv env, const char *key, NAPIValue return_value);

EXTERN_C_END

#endif /* common_h */
