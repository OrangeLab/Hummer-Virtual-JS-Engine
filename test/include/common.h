#ifndef common_h
#define common_h

#include <napi/js_native_api_types.h>

EXTERN_C_START

#include <napi/js_native_api.h>

// Empty value so that macros here are able to return NULL or void
#define NAPI_RETVAL_NOTHING // Intentionally blank #define

#define GET_AND_THROW_LAST_ERROR(env, status)                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        if ((status) != NAPIPendingException)                                                                          \
        {                                                                                                              \
            napi_throw_error((env), NULL, "empty error message");                                                      \
        }                                                                                                              \
    } while (0)

#define NAPI_ASSERT_BASE(env, assertion, message, ret_val)                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(assertion))                                                                                              \
        {                                                                                                              \
            napi_throw_error((env), NULL, "assertion (" #assertion ") failed: " message);                              \
            return ret_val;                                                                                            \
        }                                                                                                              \
    } while (0)

// Returns NULL on failed assertion.
// This is meant to be used inside napi_callback methods.
#define NAPI_ASSERT(env, assertion, message) NAPI_ASSERT_BASE(env, assertion, message, NULL)

// Returns empty on failed assertion.
// This is meant to be used inside functions with void return type.
#define NAPI_ASSERT_RETURN_VOID(env, assertion, message) NAPI_ASSERT_BASE(env, assertion, message, NAPI_RETVAL_NOTHING)

#define NAPI_CALL_BASE(env, the_call, ret_val)                                                                         \
    do                                                                                                                 \
    {                                                                                                                  \
        NAPIStatus status = the_call;                                                                                  \
        if (status != NAPIOK)                                                                                          \
        {                                                                                                              \
            GET_AND_THROW_LAST_ERROR((env), status);                                                                   \
            return ret_val;                                                                                            \
        }                                                                                                              \
    } while (0)

// Returns NULL if the_call doesn't return napi_ok.
#define NAPI_CALL(env, the_call) NAPI_CALL_BASE(env, the_call, NULL)

// Returns empty if the_call doesn't return napi_ok.
#define NAPI_CALL_RETURN_VOID(env, the_call) NAPI_CALL_BASE(env, the_call, NAPI_RETVAL_NOTHING)

#define DECLARE_NAPI_PROPERTY(name, func)                                                                              \
    {                                                                                                                  \
        (name), NULL, (func), NULL, NULL, NULL, NAPIDefault, NULL                                                      \
    }

EXTERN_C_END

#endif /* common_h */
