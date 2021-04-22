#include <gtest/gtest.h>
#include <napi/js_native_api.h>

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

static NAPIValue add(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 2;
    NAPIValue args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 2, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPIValueType valuetype1;
    NAPI_CALL(env, napi_typeof(env, args[1], &valuetype1));

    NAPI_ASSERT(env, valuetype0 == NAPINumber && valuetype1 == NAPINumber,
                "Wrong argument type. Numbers expected.");

    double value0;
    NAPI_CALL(env, napi_get_value_double(env, args[0], &value0));

    double value1;
    NAPI_CALL(env, napi_get_value_double(env, args[1], &value1));

    NAPIValue sum;
    NAPI_CALL(env, napi_create_double(env, value0 + value1, &sum));

    return sum;
}

static NAPIValue assertFunction(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValue value;
    NAPI_CALL(env, napi_coerce_to_bool(env, args[0], &value));

    bool result = false;
    NAPI_CALL(env, napi_get_value_bool(env, value, &result));
    assert(result);

    return nullptr;
}

EXTERN_C_END

TEST(FunctionArguments, add) {
    NAPIEnv env = nullptr;
    ASSERT_EQ(NAPICreateEnv(&env), NAPIOK);

    NAPIValue global = nullptr;
    ASSERT_EQ(napi_get_global(env, &global), NAPIOK);

    NAPIPropertyDescriptor desc = DECLARE_NAPI_PROPERTY("assert", assertFunction);
    ASSERT_EQ(napi_define_properties(env, global, 1, &desc), NAPIOK);

    desc = DECLARE_NAPI_PROPERTY("add", add);
    ASSERT_EQ(napi_define_properties(env, global, 1, &desc), NAPIOK);

    ASSERT_EQ(NAPIRunScriptWithSourceUrl(env, "assert(add(3, 5) === 8);", "https://n-api.com/2_function_arguments.js",
                                         nullptr), NAPIOK);

    ASSERT_EQ(NAPIFreeEnv(env), NAPIOK);
}