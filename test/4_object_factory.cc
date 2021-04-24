#include <gtest/gtest.h>
#include <napi/js_native_api.h>
#include <common.h>

EXTERN_C_START

static NAPIValue createObject(NAPIEnv env, NAPICallbackInfo info) {
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
    NAPIEnv env = nullptr;
    ASSERT_EQ(NAPICreateEnv(&env), NAPIOK);

    NAPIValue global = nullptr;
    ASSERT_EQ(napi_get_global(env, &global), NAPIOK);

    NAPIValue exports = nullptr;
    const char *exportsString = "exports";
    ASSERT_EQ(napi_create_function(env, exportsString, -1, createObject, nullptr, &exports), NAPIOK);
    EXPECT_EQ(napi_set_named_property(env, global, exportsString, exports), NAPIOK);

    EXPECT_EQ(initAssert(env, global), NAPIOK);

    NAPIValue result = nullptr;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(env, "(function () {\n"
                                              "    const obj1 = globalThis.exports('hello');\n"
                                              "    const obj2 = globalThis.exports('world');\n"
                                              "    assert.strictEqual(`${obj1.msg} ${obj2.msg}`, 'hello world');\n"
                                              "})();", "https://n-api.com/4_object_factory.js",
                                         &result), NAPIOK);
    NAPIValueType valueType;
    EXPECT_EQ(napi_typeof(env, result, &valueType), NAPIOK);
    EXPECT_EQ(valueType, NAPIUndefined);

    ASSERT_EQ(NAPIFreeEnv(env), NAPIOK);
}