#include <gtest/gtest.h>
#include <napi/js_native_api.h>
#include <napi_assert.h>

EXTERN_C_START

static NAPIDeferred deferred = NULL;

static NAPIValue createPromise(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue promise;

    // We do not overwrite an existing deferred.
    if (deferred != NULL) {
        return NULL;
    }

    NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));

    return promise;
}

static NAPIValue
concludeCurrentPromise(NAPIEnv env, NAPICallbackInfo info) {
    NAPIValue argv[2];
    size_t argc = 2;
    bool resolution;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));
    NAPI_CALL(env, napi_get_value_bool(env, argv[1], &resolution));
    if (resolution) {
        NAPI_CALL(env, napi_resolve_deferred(env, deferred, argv[0]));
    } else {
        NAPI_CALL(env, napi_reject_deferred(env, deferred, argv[0]));
    }

    deferred = NULL;

    return NULL;
}

static NAPIValue isPromise(NAPIEnv env, NAPICallbackInfo info) {
    NAPIValue promise, result;
    size_t argc = 1;
    bool is_promise;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &promise, NULL, NULL));
    NAPI_CALL(env, napi_is_promise(env, promise, &is_promise));
    NAPI_CALL(env, napi_get_boolean(env, is_promise, &result));

    return result;
}

EXTERN_C_END

TEST(TestPromise, Test) {
    NAPIEnv env = nullptr;
    ASSERT_EQ(NAPICreateEnv(&env), NAPIOK);

    NAPIValue global = nullptr;
    ASSERT_EQ(napi_get_global(env, &global), NAPIOK);

    NAPIValue exports;
    ASSERT_EQ(napi_create_object(env, &exports), NAPIOK);
    ASSERT_EQ(napi_set_named_property(env, global, "exports", exports), NAPIOK);

    NAPIPropertyDescriptor descriptors[] = {
            DECLARE_NAPI_PROPERTY("createPromise", createPromise),
            DECLARE_NAPI_PROPERTY("concludeCurrentPromise", concludeCurrentPromise),
            DECLARE_NAPI_PROPERTY("isPromise", isPromise),
    };

    ASSERT_EQ(napi_define_properties(
            env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors), NAPIOK);

    EXPECT_EQ(initAssert(env, global), NAPIOK);

    NAPIValue result = nullptr;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(env, "(function () {\n"
                                              "    const promiseTypeTestPromise = globalThis.exports.createPromise();\n"
                                              "    assert.strictEqual(globalThis.exports.isPromise(promiseTypeTestPromise), true);\n"
                                              "    globalThis.exports.concludeCurrentPromise(undefined, true);\n"
                                              "\n"
                                              "    const rejectPromise = Promise.reject(-1);\n"
                                              "    const expected_reason = -1;\n"
                                              "    assert.strictEqual(globalThis.exports.isPromise(rejectPromise), true);\n"
                                              "    rejectPromise.catch((reason) => {\n"
                                              "        assert.strictEqual(reason, expected_reason);\n"
                                              "    });\n"
                                              "\n"
                                              "    assert.strictEqual(globalThis.exports.isPromise(2.4), false);\n"
                                              "    assert.strictEqual(globalThis.exports.isPromise('I promise!'), false);\n"
                                              "    assert.strictEqual(globalThis.exports.isPromise(undefined), false);\n"
                                              "    assert.strictEqual(globalThis.exports.isPromise(null), false);\n"
                                              "    assert.strictEqual(globalThis.exports.isPromise({}), false);\n"
                                              "})();",
                                         "https://n-api.com/test_promise.js",
                                         &result), NAPIOK);
    NAPIValueType valueType;
    EXPECT_EQ(napi_typeof(env, result, &valueType), NAPIOK);
    EXPECT_EQ(valueType, NAPIUndefined);

    ASSERT_EQ(NAPIFreeEnv(env), NAPIOK);
}