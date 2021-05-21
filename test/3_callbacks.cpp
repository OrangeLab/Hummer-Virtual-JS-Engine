#include <common.h>
#include <js_native_api_test.h>

EXTERN_C_START

static NAPIValue RunCallback(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 2;
    NAPIValue args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc == 1, "Wrong number of arguments. Expects a single argument.");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));
    NAPI_ASSERT(env, valuetype0 == NAPIFunction, "Wrong type of arguments. Expects a function as first argument.");

    NAPIValueType valuetype1;
    NAPI_CALL(env, napi_typeof(env, args[1], &valuetype1));
    NAPI_ASSERT(env, valuetype1 == NAPIUndefined, "Additional arguments should be undefined.");

    NAPIValue argv[1];
    const char *str = "hello world";
    // 目前 JavaScriptCore 只支持 NAPI_AUTO_LENGTH
    //    size_t strLen = strlen(str);
    NAPI_CALL(env, napi_create_string_utf8(env, str, -1, argv));

    NAPIValue global;
    NAPI_CALL(env, napi_get_global(env, &global));

    NAPIValue cb = args[0];
    NAPI_CALL(env, napi_call_function(env, global, cb, 1, argv, nullptr));

    return getUndefined(env);
}

static NAPIValue RunCallbackWithRecv(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 2;
    NAPIValue args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPIValue cb = args[0];
    NAPIValue recv = args[1];
    NAPI_CALL(env, napi_call_function(env, recv, cb, 0, nullptr, nullptr));

    return getUndefined(env);
}

EXTERN_C_END

TEST_F(Test, Callbacks)
{
    NAPIPropertyDescriptor desc[2] = {
        DECLARE_NAPI_PROPERTY("RunCallback", RunCallback),
        DECLARE_NAPI_PROPERTY("RunCallbackWithRecv", RunCallbackWithRecv),
    };
    ASSERT_EQ(napi_define_properties(globalEnv, exports, 2, desc), NAPIOK);

    NAPIValue result;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(globalEnv,
                                         "(()=>{\"use strict\";function "
                                         "l(l){globalThis.addon.RunCallbackWithRecv((function(){globalThis.assert."
                                         "strictEqual(this,l)}),l)}globalThis.addon.RunCallback((function(l){"
                                         "globalThis.assert.strictEqual(l,\"hello world\")})),l([]),l({})})();",
                                         "https://www.didi.com/3_callbacks.js", &result),
              NAPIOK);
}