#include <gtest/gtest.h>
#include <napi/js_native_api.h>
#include <common.h>

EXTERN_C_START

static NAPIValue myFunction(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue str;
    NAPI_CALL(env, napi_create_string_utf8(env, "hello world", -1, &str));
    return str;
}

static NAPIValue createFunction(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue fn;
    NAPI_CALL(env,
              napi_create_function(env, "theFunction", -1, myFunction, nullptr, &fn));
    return fn;
}

EXTERN_C_END

TEST(FunctionFactory, CreateFunction) {
    NAPIEnv env = nullptr;
    ASSERT_EQ(NAPICreateEnv(&env), NAPIOK);

    NAPIValue global = nullptr;
    ASSERT_EQ(napi_get_global(env, &global), NAPIOK);

    NAPIValue exports;
    const char *exportsString = "exports";
    ASSERT_EQ(napi_create_function(env, exportsString, -1, createFunction, nullptr, &exports), NAPIOK);
    ASSERT_EQ(napi_set_named_property(env, global, exportsString, exports), NAPIOK);

    EXPECT_EQ(initAssert(env, global), NAPIOK);

    NAPIValue result = nullptr;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(env, "(function () {\n"
                                              "    const fn = globalThis.exports();\n"
                                              "    assert.strictEqual(fn(), 'hello world'); // 'hello world'\n"
                                              "})();",
                                         "https://n-api.com/5_function_factory.js",
                                         &result), NAPIOK);
    NAPIValueType valueType;
    EXPECT_EQ(napi_typeof(env, result, &valueType), NAPIOK);
    EXPECT_EQ(valueType, NAPIUndefined);

    ASSERT_EQ(NAPIFreeEnv(env), NAPIOK);
}