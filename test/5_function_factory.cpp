#include <js_native_api_test.h>
#include <common.h>

EXTERN_C_START

static NAPIValue MyFunction(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue str;
    NAPI_CALL(env, napi_create_string_utf8(env, "hello world", -1, &str));
    return str;
}

static NAPIValue CreateFunction(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue fn;
    NAPI_CALL(env,
              napi_create_function(env, "theFunction", -1, MyFunction, nullptr, &fn));
    return fn;
}

EXTERN_C_END

TEST(FunctionFactory, CreateFunction) {
    NAPIValue global;
    ASSERT_EQ(napi_get_global(globalEnv, &global), NAPIOK);

    NAPIValue exports;
    ASSERT_EQ(napi_create_function(globalEnv, "exports", -1, CreateFunction, nullptr, &exports), NAPIOK);
    ASSERT_EQ(napi_set_named_property(globalEnv, global, "addon", exports), NAPIOK);

    NAPIValue result;
    ASSERT_EQ(
            NAPIRunScriptWithSourceUrl(globalEnv,
                                       "(()=>{\"use strict\";var l=globalThis.addon();globalThis.assert.strictEqual(l(),\"hello world\")})();",
                                       "https://www.didi.com/5_function_factory.js",
                                       &result), NAPIOK);
}