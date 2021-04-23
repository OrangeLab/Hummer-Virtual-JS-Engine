#include <gtest/gtest.h>
#include <napi/js_native_api.h>
#include <common.h>

EXTERN_C_START

static NAPIValue runCallback(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 2;
    NAPIValue args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc == 1,
                "Wrong number of arguments. Expects a single argument.");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));
    NAPI_ASSERT(env, valuetype0 == NAPIFunction,
                "Wrong type of arguments. Expects a function as first argument.");

    NAPIValueType valuetype1;
    NAPI_CALL(env, napi_typeof(env, args[1], &valuetype1));
    NAPI_ASSERT(env, valuetype1 == NAPIUndefined,
                "Additional arguments should be undefined.");

    NAPIValue argv[1];
    const char *str = "hello world";
    size_t strLen = strlen(str);
    NAPI_CALL(env, napi_create_string_utf8(env, str, strLen, argv));

    NAPIValue global;
    NAPI_CALL(env, napi_get_global(env, &global));

    NAPIValue cb = args[0];
    NAPI_CALL(env, napi_call_function(env, global, cb, 1, argv, nullptr));

    return NULL;
}

static NAPIValue runCallbackWithRecv(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 2;
    NAPIValue args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPIValue cb = args[0];
    NAPIValue recv = args[1];
    NAPI_CALL(env, napi_call_function(env, recv, cb, 0, nullptr, nullptr));

    return NULL;
}

EXTERN_C_END

TEST(Callbacks, RunCallback) {
    NAPIEnv env = nullptr;
    ASSERT_EQ(NAPICreateEnv(&env), NAPIOK);

    NAPIValue global = nullptr;
    ASSERT_EQ(napi_get_global(env, &global), NAPIOK);

    EXPECT_EQ(initAssert(env, global), NAPIOK);

    NAPIPropertyDescriptor desc[2] = {
            DECLARE_NAPI_PROPERTY("runCallback", runCallback),
            DECLARE_NAPI_PROPERTY("runCallbackWithRecv", runCallbackWithRecv),
    };
    EXPECT_EQ(napi_define_properties(env, global, 2, desc), NAPIOK);

    NAPIValue result = nullptr;
    EXPECT_EQ(NAPIRunScriptWithSourceUrl(env, "(function () {\n"
                                              "    globalThis.runCallback(function (msg) {\n"
                                              "        assert(msg === 'hello world');\n"
                                              "    });\n"
                                              "\n"
                                              "    function testRecv(desiredRecv) {\n"
                                              "        globalThis.runCallbackWithRecv(function () {\n"
                                              "            assert(this === desiredRecv);\n"
                                              "        }, desiredRecv);\n"
                                              "    }\n"
                                              "\n"
                                              "    testRecv(undefined);\n"
                                              "    testRecv(null);\n"
                                              "    testRecv(5);\n"
                                              "    testRecv(true);\n"
                                              "    testRecv('Hello');\n"
                                              "    testRecv([]);\n"
                                              "    testRecv({});\n"
                                              "})();", "https://n-api.com/3_callbacks.js",
                                         &result), NAPIOK);
    NAPIValueType valueType;
    EXPECT_EQ(napi_typeof(env, result, &valueType), NAPIOK);
    EXPECT_EQ(valueType, NAPIUndefined);

    ASSERT_EQ(NAPIFreeEnv(env), NAPIOK);
}