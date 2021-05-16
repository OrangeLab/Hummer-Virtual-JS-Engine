#include <js_native_api_test.h>
#include <common.h>

EXTERN_C_START

static NAPIValue CreateObject(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPIValue obj;
    NAPI_CALL(env, napi_create_object(env, &obj));

    NAPI_CALL(env, napi_set_named_property(env, obj, "msg", args[0]));

    return obj;
}

EXTERN_C_END

TEST(ObjectFactory, CreateObject) {
    NAPIValue global;
    ASSERT_EQ(napi_get_global(globalEnv, &global), NAPIOK);

    NAPIValue exports;
    ASSERT_EQ(napi_create_function(globalEnv, "exports", -1, CreateObject, nullptr, &exports), NAPIOK);
    ASSERT_EQ(napi_set_named_property(globalEnv, global, "addon", exports), NAPIOK);

    NAPIValue result;
    ASSERT_EQ(
            NAPIRunScriptWithSourceUrl(globalEnv,
                                       "(()=>{\"use strict\";var l=globalThis.addon(\"hello\"),s=globalThis.addon(\"world\");globalThis.assert.strictEqual(`${l.msg} ${s.msg}`,\"hello world\")})();",
                                       "https://www.didi.com/4_object_factory.js",
                                       &result), NAPIOK);
}