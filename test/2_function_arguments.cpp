#include <common.h>
#include <js_native_api_test.h>

EXTERN_C_START

static NAPIValue Add(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 2;
    NAPIValue args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 2, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPIValueType valuetype1;
    NAPI_CALL(env, napi_typeof(env, args[1], &valuetype1));

    NAPI_ASSERT(env, valuetype0 == NAPINumber && valuetype1 == NAPINumber, "Wrong argument type. Numbers expected.");

    double value0;
    NAPI_CALL(env, napi_get_value_double(env, args[0], &value0));

    double value1;
    NAPI_CALL(env, napi_get_value_double(env, args[1], &value1));

    NAPIValue sum;
    NAPI_CALL(env, napi_create_double(env, value0 + value1, &sum));

    return sum;
}

EXTERN_C_END

TEST_F(Test, FunctionArguments)
{
    NAPIPropertyDescriptor desc = DECLARE_NAPI_PROPERTY("add", Add);
    ASSERT_EQ(napi_define_properties(globalEnv, exports, 1, &desc), NAPIOK);

    NAPIValue result;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(
                  globalEnv, "(()=>{\"use strict\";globalThis.assert.strictEqual(globalThis.addon.add(3,5),8)})();",
                  "https://www.didi.com/2_function_arguments.js", &result),
              NAPIOK);
}