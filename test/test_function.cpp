#include <common.h>
#include <js_native_api_test.h>

EXTERN_C_START

static NAPIValue TestCallFunction(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 10;
    NAPIValue args[10];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc > 0, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == NAPIFunction, "Wrong type of arguments. Expects a function as first argument.");

    NAPIValue *argv = args + 1;
    argc = argc - 1;

    NAPIValue global;
    NAPI_CALL(env, napi_get_global(env, &global));

    NAPIValue result;
    NAPI_CALL(env, napi_call_function(env, global, args[0], argc, argv, &result));

    return result;
}

static NAPIValue TestFunctionName(NAPIEnv /*env*/, NAPICallbackInfo /*info*/)
{
    return nullptr;
}

EXTERN_C_END

TEST_F(Test, TestFunction)
{
    NAPIValue fn1;
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, NAPI_AUTO_LENGTH, TestCallFunction, nullptr, &fn1), NAPIOK);

    NAPIValue fn2;
    ASSERT_EQ(napi_create_function(globalEnv, "Name", NAPI_AUTO_LENGTH, TestFunctionName, nullptr, &fn2), NAPIOK);

    NAPIValue fn3;
    ASSERT_EQ(napi_create_function(globalEnv, "Name_extra", -1, TestFunctionName, nullptr, &fn3), NAPIOK);

    ASSERT_EQ(napi_set_named_property(globalEnv, exports, "TestCall", fn1), NAPIOK);
    ASSERT_EQ(napi_set_named_property(globalEnv, exports, "TestName", fn2), NAPIOK);
    ASSERT_EQ(napi_set_named_property(globalEnv, exports, "TestNameShort", fn3), NAPIOK);

    NAPIValue result;
    ASSERT_EQ(NAPIRunScript(globalEnv,
                            "(()=>{\"use strict\";function l(l){return "
                            "l+1}globalThis.assert.strictEqual(globalThis.addon.TestCall((function(){return "
                            "1})),1),globalThis.assert.strictEqual(globalThis.addon.TestCall((function(){return "
                            "null})),null),globalThis.assert.strictEqual(globalThis.addon.TestCall(l,1),2),globalThis."
                            "assert.strictEqual(globalThis.addon.TestCall((function(a){return "
                            "l(a)}),1),2),globalThis.assert.strictEqual(globalThis.addon.TestName.name,\"Name\"),"
                            "globalThis.assert.strictEqual(globalThis.addon.TestNameShort.name,\"Name_extra\")})();",
                            "https://www.napi.com/test_function.js", &result),
              NAPIOK);
}