#ifndef napi_assert_h
#define napi_assert_h

EXTERN_C_START

#include <assert.h>
#include <common.h>

// This does not call napi_set_last_error because the expression
// is assumed to be a NAPI function call that already did.
#define CHECK_NAPI(expr)            \
    do                              \
    {                               \
        NAPIStatus status = expr; \
        if (status != NAPIOK)       \
        {                           \
            return status;          \
        }                           \
    } while (0)

static NAPIValue
assertFunction(NAPIEnv
               env,
               NAPICallbackInfo info
) {
    size_t argc = 1;
    NAPIValue args[1];
    assert(napi_get_cb_info(env, info, &argc, args, NULL, NULL) == NAPIOK);

// "Wrong number of arguments"
    assert(argc >= 1);

    NAPIValue result;
    assert(napi_coerce_to_bool(env, args[0], &result) == NAPIOK);
    bool resultBoolean = false;
    assert(napi_get_value_bool(env, result, &resultBoolean) == NAPIOK);
    assert(resultBoolean);

    return NULL;
}

static NAPIValue strictEqual(NAPIEnv
                             env,
                             NAPICallbackInfo info
) {
    size_t argc = 2;
    NAPIValue args[2];
    assert(napi_get_cb_info(env, info, &argc, args, NULL, NULL) == NAPIOK);

// "Wrong number of arguments"
    assert(argc >= 2);

    bool result = false;
    assert(napi_strict_equals(env, args[0], args[1], &result) == NAPIOK);
// !==
    assert(result);

    return NULL;
}

static NAPIValue notStrictEqual(NAPIEnv
                                env,
                                NAPICallbackInfo info
) {
    size_t argc = 2;
    NAPIValue args[2];
    assert(napi_get_cb_info(env, info, &argc, args, NULL, NULL) == NAPIOK);

// "Wrong number of arguments"
    assert(argc >= 2);

    bool result = false;
    assert(napi_strict_equals(env, args[0], args[1], &result) == NAPIOK);
// ===
    assert(!result);

    return NULL;
}

static NAPIValue throws(NAPIEnv
                        env,
                        NAPICallbackInfo info
) {
    size_t argc = 1;
    NAPIValue args[1];
    assert(napi_get_cb_info(env, info, &argc, args, NULL, NULL) == NAPIOK);

// "Wrong number of arguments"
    assert(argc >= 1);

    NAPIValueType valueType;
    assert(napi_typeof(env, args[0], &valueType) == NAPIOK);
// Argument is not function
    assert(valueType == NAPIFunction);

    NAPIValue global;
    assert(napi_get_global(env, &global) == NAPIOK);

// throw
    assert(napi_call_function(env, global, args[0], 0, NULL, NULL) == NAPIPendingException);
    NAPIValue exception = NULL;
    assert(napi_get_and_clear_last_exception(env, &exception) == NAPIOK);
// throw
    assert(exception);

    return NULL;
}

static inline NAPIStatus initAssert(NAPIEnv
                                    env,
                                    NAPIValue global
) {
    NAPIValue value;
    const char *assertString = "assert";
    CHECK_NAPI(napi_create_function(env, assertString, -1, assertFunction, NULL, &value)
    );
    CHECK_NAPI(napi_set_named_property(env, global, "assert", value)
    );

    NAPIPropertyDescriptor desc[] = {
            DECLARE_NAPI_PROPERTY("strictEqual", strictEqual),
            DECLARE_NAPI_PROPERTY("notStrictEqual", notStrictEqual),
            DECLARE_NAPI_PROPERTY("throws", throws)
    };
    CHECK_NAPI(napi_define_properties(env, value, 3, desc)
    );

    return
            NAPIOK;
}

EXTERN_C_END

#endif // napi_assert_h