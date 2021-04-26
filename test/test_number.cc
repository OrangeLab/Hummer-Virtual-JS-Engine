#include <gtest/gtest.h>
#include <napi/js_native_api.h>
#include <napi_assert.h>

EXTERN_C_START

static NAPIValue TestFunction(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == NAPINumber,
                "Wrong type of arguments. Expects a number as first argument.");

    double input;
    NAPI_CALL(env, napi_get_value_double(env, args[0], &input));

    NAPIValue output;
    NAPI_CALL(env, napi_create_double(env, input, &output));

    return output;
}

static NAPIValue TestUint32Truncation(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == NAPINumber,
                "Wrong type of arguments. Expects a number as first argument.");

    uint32_t input;
    NAPI_CALL(env, napi_get_value_uint32(env, args[0], &input));

    NAPIValue output;
    NAPI_CALL(env, napi_create_uint32(env, input, &output));

    return output;
}

static NAPIValue TestInt32Truncation(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == NAPINumber,
                "Wrong type of arguments. Expects a number as first argument.");

    int32_t input;
    NAPI_CALL(env, napi_get_value_int32(env, args[0], &input));

    NAPIValue output;
    NAPI_CALL(env, napi_create_int32(env, input, &output));

    return output;
}

static NAPIValue TestInt64Truncation(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == NAPINumber,
                "Wrong type of arguments. Expects a number as first argument.");

    int64_t input;
    NAPI_CALL(env, napi_get_value_int64(env, args[0], &input));

    NAPIValue output;
    NAPI_CALL(env, napi_create_int64(env, input, &output));

    return output;
}

EXTERN_C_END

TEST(TestNumber, Test) {
    NAPIEnv env = nullptr;
    ASSERT_EQ(NAPICreateEnv(&env), NAPIOK);

    NAPIValue global = nullptr;
    ASSERT_EQ(napi_get_global(env, &global), NAPIOK);

    NAPIValue exports;
    ASSERT_EQ(napi_create_object(env, &exports), NAPIOK);
    ASSERT_EQ(napi_set_named_property(env, global, "exports", exports), NAPIOK);

    NAPIPropertyDescriptor descriptors[] = {
            DECLARE_NAPI_PROPERTY("Test", TestFunction),
            DECLARE_NAPI_PROPERTY("TestInt32Truncation", TestInt32Truncation),
            DECLARE_NAPI_PROPERTY("TestUint32Truncation", TestUint32Truncation),
            DECLARE_NAPI_PROPERTY("TestInt64Truncation", TestInt64Truncation),
    };

    ASSERT_EQ(napi_define_properties(
            env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors), NAPIOK);

    EXPECT_EQ(initAssert(env, global), NAPIOK);

    NAPIValue result = nullptr;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(env, "(function () {\n"
                                              "    // Testing api calls for number\n"
                                              "    function testNumber(num) {\n"
                                              "        assert.strictEqual(num, globalThis.exports.Test(num));\n"
                                              "    }\n"
                                              "\n"
                                              "    testNumber(0);\n"
                                              "    testNumber(-0);\n"
                                              "    testNumber(1);\n"
                                              "    testNumber(-1);\n"
                                              "    testNumber(100);\n"
                                              "    testNumber(2121);\n"
                                              "    testNumber(-1233);\n"
                                              "    testNumber(986583);\n"
                                              "    testNumber(-976675);\n"
                                              "\n"
                                              "    testNumber(\n"
                                              "        98765432213456789876546896323445679887645323232436587988766545658);\n"
                                              "    testNumber(\n"
                                              "        -4350987086545760976737453646576078997096876957864353245245769809);\n"
                                              "    testNumber(Number.MIN_SAFE_INTEGER);\n"
                                              "    testNumber(Number.MAX_SAFE_INTEGER);\n"
                                              "    testNumber(Number.MAX_SAFE_INTEGER + 10);\n"
                                              "\n"
                                              "    testNumber(Number.MIN_VALUE);\n"
                                              "    testNumber(Number.MAX_VALUE);\n"
                                              "    testNumber(Number.MAX_VALUE + 10);\n"
                                              "\n"
                                              "    testNumber(Number.POSITIVE_INFINITY);\n"
                                              "    testNumber(Number.NEGATIVE_INFINITY);\n"
                                              "    testNumber(Number.NaN);\n"
                                              "\n"
                                              "    function testUint32(input, expected = input) {\n"
                                              "        assert.strictEqual(expected, globalThis.exports.TestUint32Truncation(input));\n"
                                              "    }\n"
                                              "\n"
                                              "    // Test zero\n"
                                              "    testUint32(0.0, 0);\n"
                                              "    testUint32(-0.0, 0);\n"
                                              "\n"
                                              "    // Test overflow scenarios\n"
                                              "    testUint32(4294967295);\n"
                                              "    testUint32(4294967296, 0);\n"
                                              "    testUint32(4294967297, 1);\n"
                                              "    testUint32(17 * 4294967296 + 1, 1);\n"
                                              "    testUint32(-1, 0xffffffff);\n"
                                              "\n"
                                              "    // Validate documented behavior when value is retrieved as 32-bit integer with\n"
                                              "    // `napi_get_value_int32`\n"
                                              "    function testInt32(input, expected = input) {\n"
                                              "        assert.strictEqual(expected, globalThis.exports.TestInt32Truncation(input));\n"
                                              "    }\n"
                                              "\n"
                                              "    // Test zero\n"
                                              "    testInt32(0.0, 0);\n"
                                              "    testInt32(-0.0, 0);\n"
                                              "\n"
                                              "    // Test min/max int32 range\n"
                                              "    testInt32(-Math.pow(2, 31));\n"
                                              "    testInt32(Math.pow(2, 31) - 1);\n"
                                              "\n"
                                              "    // Test overflow scenarios\n"
                                              "    testInt32(4294967297, 1);\n"
                                              "    testInt32(4294967296, 0);\n"
                                              "    testInt32(4294967295, -1);\n"
                                              "    testInt32(4294967296 * 5 + 3, 3);\n"
                                              "\n"
                                              "    // Test min/max safe integer range\n"
                                              "    testInt32(Number.MIN_SAFE_INTEGER, 1);\n"
                                              "    testInt32(Number.MAX_SAFE_INTEGER, -1);\n"
                                              "\n"
                                              "    // Test within int64_t range (with precision loss)\n"
                                              "    testInt32(-Math.pow(2, 63) + (Math.pow(2, 9) + 1), 1024);\n"
                                              "    testInt32(Math.pow(2, 63) - (Math.pow(2, 9) + 1), -1024);\n"
                                              "\n"
                                              "    // Test min/max double value\n"
                                              "    testInt32(-Number.MIN_VALUE, 0);\n"
                                              "    testInt32(Number.MIN_VALUE, 0);\n"
                                              "    testInt32(-Number.MAX_VALUE, 0);\n"
                                              "    testInt32(Number.MAX_VALUE, 0);\n"
                                              "\n"
                                              "    // Test outside int64_t range\n"
                                              "    testInt32(-Math.pow(2, 63) + (Math.pow(2, 9)), 0);\n"
                                              "    testInt32(Math.pow(2, 63) - (Math.pow(2, 9)), 0);\n"
                                              "\n"
                                              "    // Test non-finite numbers\n"
                                              "    testInt32(Number.POSITIVE_INFINITY, 0);\n"
                                              "    testInt32(Number.NEGATIVE_INFINITY, 0);\n"
                                              "    testInt32(Number.NaN, 0);\n"
                                              "\n"
                                              "    // Validate documented behavior when value is retrieved as 64-bit integer with\n"
                                              "    // `napi_get_value_int64`\n"
                                              "    function testInt64(input, expected = input) {\n"
                                              "        assert.strictEqual(expected, globalThis.exports.TestInt64Truncation(input));\n"
                                              "    }\n"
                                              "\n"
                                              "    // Both V8 and ChakraCore return a sentinel value of `0x8000000000000000` when\n"
                                              "    // the conversion goes out of range, but V8 treats it as unsigned in some cases.\n"
                                              "    const RANGEERROR_POSITIVE = Math.pow(2, 63);\n"
                                              "    const RANGEERROR_NEGATIVE = -Math.pow(2, 63);\n"
                                              "\n"
                                              "    // Test zero\n"
                                              "    testInt64(0.0, 0);\n"
                                              "    testInt64(-0.0, 0);\n"
                                              "\n"
                                              "    // Test min/max safe integer range\n"
                                              "    testInt64(Number.MIN_SAFE_INTEGER);\n"
                                              "    testInt64(Number.MAX_SAFE_INTEGER);\n"
                                              "\n"
                                              "    // Test within int64_t range (with precision loss)\n"
                                              "    testInt64(-Math.pow(2, 63) + (Math.pow(2, 9) + 1));\n"
                                              "    testInt64(Math.pow(2, 63) - (Math.pow(2, 9) + 1));\n"
                                              "\n"
                                              "    // Test min/max double value\n"
                                              "    testInt64(-Number.MIN_VALUE, 0);\n"
                                              "    testInt64(Number.MIN_VALUE, 0);\n"
                                              "    testInt64(-Number.MAX_VALUE, RANGEERROR_NEGATIVE);\n"
                                              "    testInt64(Number.MAX_VALUE, RANGEERROR_POSITIVE);\n"
                                              "\n"
                                              "    // Test outside int64_t range\n"
                                              "    testInt64(-Math.pow(2, 63) + (Math.pow(2, 9)), RANGEERROR_NEGATIVE);\n"
                                              "    testInt64(Math.pow(2, 63) - (Math.pow(2, 9)), RANGEERROR_POSITIVE);\n"
                                              "\n"
                                              "    // Test non-finite numbers\n"
                                              "    testInt64(Number.POSITIVE_INFINITY, 0);\n"
                                              "    testInt64(Number.NEGATIVE_INFINITY, 0);\n"
                                              "    testInt64(Number.NaN, 0);\n"
                                              "})();",
                                         "https://n-api.com/test_number.js",
                                         &result), NAPIOK);
    NAPIValueType valueType;
    EXPECT_EQ(napi_typeof(env, result, &valueType), NAPIOK);
    EXPECT_EQ(valueType, NAPIUndefined);

    ASSERT_EQ(NAPIFreeEnv(env), NAPIOK);
}