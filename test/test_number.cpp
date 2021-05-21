#include <common.h>
#include <js_native_api_test.h>

EXTERN_C_START

static NAPIValue TestFunction(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == NAPINumber, "Wrong type of arguments. Expects a number as first argument.");

    double input;
    NAPI_CALL(env, napi_get_value_double(env, args[0], &input));

    NAPIValue output;
    NAPI_CALL(env, napi_create_double(env, input, &output));

    return output;
}

static NAPIValue TestUint32Truncation(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == NAPINumber, "Wrong type of arguments. Expects a number as first argument.");

    uint32_t input;
    NAPI_CALL(env, napi_get_value_uint32(env, args[0], &input));

    NAPIValue output;
    NAPI_CALL(env, napi_create_uint32(env, input, &output));

    return output;
}

static NAPIValue TestInt32Truncation(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == NAPINumber, "Wrong type of arguments. Expects a number as first argument.");

    int32_t input;
    NAPI_CALL(env, napi_get_value_int32(env, args[0], &input));

    NAPIValue output;
    NAPI_CALL(env, napi_create_int32(env, input, &output));

    return output;
}

static NAPIValue TestInt64Truncation(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == NAPINumber, "Wrong type of arguments. Expects a number as first argument.");

    int64_t input;
    NAPI_CALL(env, napi_get_value_int64(env, args[0], &input));

    NAPIValue output;
    NAPI_CALL(env, napi_create_int64(env, input, &output));

    return output;
}

EXTERN_C_END

TEST_F(Test, TestNumber)
{
    NAPIPropertyDescriptor descriptors[] = {
        DECLARE_NAPI_PROPERTY("Test", TestFunction),
        DECLARE_NAPI_PROPERTY("TestInt32Truncation", TestInt32Truncation),
        DECLARE_NAPI_PROPERTY("TestUint32Truncation", TestUint32Truncation),
        DECLARE_NAPI_PROPERTY("TestInt64Truncation", TestInt64Truncation),
    };

    ASSERT_EQ(napi_define_properties(globalEnv, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors),
              NAPIOK);

    NAPIValue result;
    ASSERT_EQ(
        NAPIRunScriptWithSourceUrl(
            globalEnv,
            "(()=>{\"use strict\";function N(N){globalThis.assert.strictEqual(N,globalThis.addon.Test(N))}function "
            "t(N){var t=arguments.length>1&&void "
            "0!==arguments[1]?arguments[1]:N;globalThis.assert.strictEqual(t,globalThis.addon.TestUint32Truncation(N))}"
            "function a(N){var t=arguments.length>1&&void "
            "0!==arguments[1]?arguments[1]:N;globalThis.assert.strictEqual(t,globalThis.addon.TestInt32Truncation(N))}"
            "function E(N){var t=arguments.length>1&&void "
            "0!==arguments[1]?arguments[1]:N;globalThis.assert.strictEqual(t,globalThis.addon.TestInt64Truncation(N))}"
            "N(0),N(-0),N(1),N(-1),N(100),N(2121),N(-1233),N(986583),N(-976675),N(987654322134568e50),N(-"
            "4350987086545761e48),N(Number.MIN_SAFE_INTEGER),N(Number.MAX_SAFE_INTEGER),N(Number.MAX_SAFE_INTEGER+10),"
            "N(Number.MIN_VALUE),N(Number.MAX_VALUE),N(Number.MAX_VALUE+10),N(Number.POSITIVE_INFINITY),N(Number."
            "NEGATIVE_INFINITY),N(Number.NaN),t(0,0),t(-0,0),t(4294967295),t(4294967296,0),t(4294967297,1),t("
            "73014444033,1),t(-1,4294967295),a(0,0),a(-0,0),a(-Math.pow(2,31)),a(Math.pow(2,31)-1),a(4294967297,1),a("
            "4294967296,0),a(4294967295,-1),a(21474836483,3),a(Number.MIN_SAFE_INTEGER,1),a(Number.MAX_SAFE_INTEGER,-1)"
            ",a(-Math.pow(2,63)+(Math.pow(2,9)+1),1024),a(Math.pow(2,63)-(Math.pow(2,9)+1),-1024),a(-Number.MIN_VALUE,"
            "0),a(Number.MIN_VALUE,0),a(-Number.MAX_VALUE,0),a(Number.MAX_VALUE,0),a(-Math.pow(2,63)+Math.pow(2,9),0),"
            "a(Math.pow(2,63)-Math.pow(2,9),0),a(Number.POSITIVE_INFINITY,0),a(Number.NEGATIVE_INFINITY,0),a(Number."
            "NaN,0);var "
            "I=Math.pow(2,63),r=-Math.pow(2,63);E(0,0),E(-0,0),E(Number.MIN_SAFE_INTEGER),E(Number.MAX_SAFE_INTEGER),E("
            "-Math.pow(2,63)+(Math.pow(2,9)+1)),E(Math.pow(2,63)-(Math.pow(2,9)+1)),E(-Number.MIN_VALUE,0),E(Number."
            "MIN_VALUE,0),E(-Number.MAX_VALUE,r),E(Number.MAX_VALUE,I),E(-Math.pow(2,63)+Math.pow(2,9),r),E(Math.pow(2,"
            "63)-Math.pow(2,9),I),E(Number.POSITIVE_INFINITY,0),E(Number.NEGATIVE_INFINITY,0),E(Number.NaN,0)})();",
            "https://www.didi.com/test_number.js", &result),
        NAPIOK);
}