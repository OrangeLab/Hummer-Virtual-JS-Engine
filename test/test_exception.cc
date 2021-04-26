#include <gtest/gtest.h>
#include <napi/js_native_api.h>
#include <napi_assert.h>

EXTERN_C_START

static bool exceptionWasPending = false;

static NAPIValue returnException(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPIValue global;
    NAPI_CALL(env, napi_get_global(env, &global));

    NAPIValue result;
    NAPIStatus status = napi_call_function(env, global, args[0], 0, 0, &result);
    if (status == NAPIPendingException) {
        NAPIValue ex;
        NAPI_CALL(env, napi_get_and_clear_last_exception(env, &ex));
        return ex;
    }

    return NULL;
}

static NAPIValue allowException(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPIValue global;
    NAPI_CALL(env, napi_get_global(env, &global));

    NAPIValue result;
    napi_call_function(env, global, args[0], 0, 0, &result);
    // Ignore status and check napi_is_exception_pending() instead.

    NAPI_CALL(env, napi_is_exception_pending(env, &exceptionWasPending));
    return NULL;
}

static NAPIValue wasPending(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue result;
    NAPI_CALL(env, napi_get_boolean(env, exceptionWasPending, &result));

    return result;
}

static void finalizer(NAPIEnv env, void * /*data*/, void * /*hint*/) {
    NAPI_CALL_RETURN_VOID(env,
                          napi_throw_error(env, NULL, "Error during Finalize"));
}

static NAPIValue createExternal(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue external;

    NAPI_CALL(env,
              napi_create_external(env, NULL, finalizer, NULL, &external));

    return external;
}

static NAPIValue requireFunction(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue exports;
    NAPI_CALL(env, napi_create_object(env, &exports));

    NAPIPropertyDescriptor descriptors[] = {
            DECLARE_NAPI_PROPERTY("returnException", returnException),
            DECLARE_NAPI_PROPERTY("allowException", allowException),
            DECLARE_NAPI_PROPERTY("wasPending", wasPending),
            DECLARE_NAPI_PROPERTY("createExternal", createExternal),
    };
    NAPI_CALL(env, napi_define_properties(
            env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors));

    NAPIValue error, code, message;
    NAPI_CALL(env, napi_create_string_utf8(env, "Error during Init",
                                           NAPI_AUTO_LENGTH, &message));
    NAPI_CALL(env, napi_create_string_utf8(env, "", NAPI_AUTO_LENGTH, &code));
    NAPI_CALL(env, napi_create_error(env, code, message, &error));
    NAPI_CALL(env, napi_set_named_property(env, error, "binding", exports));
    NAPI_CALL(env, napi_throw(env, error));

    return exports;
}

EXTERN_C_END

TEST(TestException, Test) {
    NAPIEnv env = nullptr;
    ASSERT_EQ(NAPICreateEnv(&env), NAPIOK);

    NAPIValue global = nullptr;
    ASSERT_EQ(napi_get_global(env, &global), NAPIOK);

    NAPIValue requireValue;
    ASSERT_EQ(napi_create_function(env, NULL, -1, requireFunction, NULL, &requireValue), NAPIOK);
    ASSERT_EQ(napi_set_named_property(env, global, "require", requireValue), NAPIOK);

    EXPECT_EQ(initAssert(env, global), NAPIOK);

    NAPIValue result = nullptr;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(env, "(function () {\n"
                                              "    const theError = new Error('Some error');\n"
                                              "\n"
                                              "    // The test module throws an error during Init, but in order for its exports to\n"
                                              "    // not be lost, it attaches them to the error's \"bindings\" property. This way,\n"
                                              "    // we can make sure that exceptions thrown during the module initialization\n"
                                              "    // phase are propagated through require() into JavaScript.\n"
                                              "    // https://github.com/nodejs/node/issues/19437\n"
                                              "    const test_exception = (function () {\n"
                                              "        let resultingException;\n"
                                              "        try {\n"
                                              "            globalThis.require();\n"
                                              "        } catch (anException) {\n"
                                              "            resultingException = anException;\n"
                                              "        }\n"
                                              "        assert.strictEqual(resultingException.message, 'Error during Init');\n"
                                              "        return resultingException.binding;\n"
                                              "    })();\n"
                                              "\n"
                                              "    {\n"
                                              "        const throwTheError = () => { throw theError; };\n"
                                              "\n"
                                              "        // Test that the native side successfully captures the exception\n"
                                              "        let returnedError = test_exception.returnException(throwTheError);\n"
                                              "        assert.strictEqual(returnedError, theError);\n"
                                              "\n"
                                              "        // Test that the native side passes the exception through\n"
                                              "        assert.throws(\n"
                                              "            () => { test_exception.allowException(throwTheError); }\n"
                                              "        );\n"
                                              "\n"
                                              "        // Test that the exception thrown above was marked as pending\n"
                                              "        // before it was handled on the JS side\n"
                                              "        const exception_pending = test_exception.wasPending();\n"
                                              "        assert.strictEqual(exception_pending, true);\n"
                                              "    }\n"
                                              "})();",
                                         "https://n-api.com/test_exception_test.js",
                                         &result), NAPIOK);
    NAPIValueType valueType;
    EXPECT_EQ(napi_typeof(env, result, &valueType), NAPIOK);
    EXPECT_EQ(valueType, NAPIUndefined);

    ASSERT_EQ(NAPIFreeEnv(env), NAPIOK);
}