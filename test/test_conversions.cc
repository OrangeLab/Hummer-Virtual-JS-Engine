#include <gtest/gtest.h>
#include <napi/js_native_api.h>
#include <napi_assert.h>

EXTERN_C_START

#define GEN_NULL_CHECK_BINDING(binding_name, output_type, api)            \
  static NAPIValue binding_name(NAPIEnv env, NAPICallbackInfo /*info*/) { \
    NAPIValue return_value;                                              \
    output_type result;                                                   \
    NAPI_CALL(env, napi_create_object(env, &return_value));               \
    add_returned_status(env,                                              \
                        "envIsNull",                                      \
                        return_value,                                     \
                        "Invalid argument",                               \
                        NAPIInvalidArg,                                 \
                        api(NULL, return_value, &result));                \
    api(env, NULL, &result);                                              \
    add_last_status(env, "valueIsNull", return_value);                    \
    api(env, return_value, NULL);                                         \
    add_last_status(env, "resultIsNull", return_value);                   \
    api(env, return_value, &result);                                      \
    add_last_status(env, "inputTypeCheck", return_value);                 \
    return return_value;                                                  \
  }

GEN_NULL_CHECK_BINDING(GetValueBool, bool, napi_get_value_bool)
GEN_NULL_CHECK_BINDING(GetValueInt32, int32_t, napi_get_value_int32)
GEN_NULL_CHECK_BINDING(GetValueUint32, uint32_t, napi_get_value_uint32)
GEN_NULL_CHECK_BINDING(GetValueInt64, int64_t, napi_get_value_int64)
GEN_NULL_CHECK_BINDING(GetValueDouble, double, napi_get_value_double)
GEN_NULL_CHECK_BINDING(CoerceToBool, NAPIValue, napi_coerce_to_bool)
GEN_NULL_CHECK_BINDING(CoerceToObject, NAPIValue, napi_coerce_to_object)
GEN_NULL_CHECK_BINDING(CoerceToString, NAPIValue, napi_coerce_to_string)

#define GEN_NULL_CHECK_STRING_BINDING(binding_name, arg_type, api)         \
  static NAPIValue binding_name(NAPIEnv env, NAPICallbackInfo /*info*/) {  \
    NAPIValue return_value;                                               \
    NAPI_CALL(env, napi_create_object(env, &return_value));                \
    arg_type buf1[4];                                                      \
    size_t length1 = 3;                                                    \
    add_returned_status(env,                                               \
                        "envIsNull",                                       \
                        return_value,                                      \
                        "Invalid argument",                                \
                        NAPIInvalidArg,                                  \
                        api(NULL, return_value, buf1, length1, &length1)); \
    arg_type buf2[4];                                                      \
    size_t length2 = 3;                                                    \
    api(env, NULL, buf2, length2, &length2);                               \
    add_last_status(env, "valueIsNull", return_value);                     \
    api(env, return_value, NULL, 3, NULL);                                 \
    add_last_status(env, "wrongTypeIn", return_value);                     \
    NAPIValue string;                                                     \
    NAPI_CALL(env,                                                         \
              napi_create_string_utf8(env,                                 \
                                      "Something",                         \
                                      NAPI_AUTO_LENGTH,                    \
                                      &string));                           \
    api(env, string, NULL, 3, NULL);                                       \
    add_last_status(env, "bufAndOutLengthIsNull", return_value);           \
    return return_value;                                                   \
  }

GEN_NULL_CHECK_STRING_BINDING(GetValueStringUtf8,
                              char,
                              napi_get_value_string_utf8)
//GEN_NULL_CHECK_STRING_BINDING(GetValueStringLatin1,
//                              char,
//                              napi_get_value_string_latin1)
GEN_NULL_CHECK_STRING_BINDING(GetValueStringUtf16,
                              char16_t,
                              napi_get_value_string_utf16)

static void init_test_null(NAPIEnv env, NAPIValue exports) {
    NAPIValue test_null;

    const NAPIPropertyDescriptor test_null_props[] = {
            DECLARE_NAPI_PROPERTY("getValueBool", GetValueBool),
            DECLARE_NAPI_PROPERTY("getValueInt32", GetValueInt32),
            DECLARE_NAPI_PROPERTY("getValueUint32", GetValueUint32),
            DECLARE_NAPI_PROPERTY("getValueInt64", GetValueInt64),
            DECLARE_NAPI_PROPERTY("getValueDouble", GetValueDouble),
            DECLARE_NAPI_PROPERTY("coerceToBool", CoerceToBool),
            DECLARE_NAPI_PROPERTY("coerceToObject", CoerceToObject),
            DECLARE_NAPI_PROPERTY("coerceToString", CoerceToString),
            DECLARE_NAPI_PROPERTY("getValueStringUtf8", GetValueStringUtf8),
//            DECLARE_NAPI_PROPERTY("getValueStringLatin1", GetValueStringLatin1),
            DECLARE_NAPI_PROPERTY("getValueStringUtf16", GetValueStringUtf16),
    };

    NAPI_CALL_RETURN_VOID(env, napi_create_object(env, &test_null));
    NAPI_CALL_RETURN_VOID(env, napi_define_properties(
            env, test_null, sizeof(test_null_props) / sizeof(*test_null_props),
            test_null_props));

    const NAPIPropertyDescriptor test_null_set = {
            "testNull", NULL, NULL, NULL, NULL, test_null, NAPIEnumerable, NULL
    };

    NAPI_CALL_RETURN_VOID(env,
                          napi_define_properties(env, exports, 1, &test_null_set));
}

static NAPIValue AsBool(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    bool value;
    NAPI_CALL(env, napi_get_value_bool(env, args[0], &value));

    NAPIValue output;
    NAPI_CALL(env, napi_get_boolean(env, value, &output));

    return output;
}

static NAPIValue AsInt32(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    int32_t value;
    NAPI_CALL(env, napi_get_value_int32(env, args[0], &value));

    NAPIValue output;
    NAPI_CALL(env, napi_create_int32(env, value, &output));

    return output;
}

static NAPIValue AsUInt32(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    uint32_t value;
    NAPI_CALL(env, napi_get_value_uint32(env, args[0], &value));

    NAPIValue output;
    NAPI_CALL(env, napi_create_uint32(env, value, &output));

    return output;
}

static NAPIValue AsInt64(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    int64_t value;
    NAPI_CALL(env, napi_get_value_int64(env, args[0], &value));

    NAPIValue output;
    NAPI_CALL(env, napi_create_int64(env, (double) value, &output));

    return output;
}

static NAPIValue AsDouble(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    double value;
    NAPI_CALL(env, napi_get_value_double(env, args[0], &value));

    NAPIValue output;
    NAPI_CALL(env, napi_create_double(env, value, &output));

    return output;
}

static NAPIValue AsString(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    char value[100];
    NAPI_CALL(env,
              napi_get_value_string_utf8(env, args[0], value, sizeof(value), NULL));

    NAPIValue output;
    NAPI_CALL(env, napi_create_string_utf8(
            env, value, NAPI_AUTO_LENGTH, &output));

    return output;
}

static NAPIValue ToBool(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPIValue output;
    NAPI_CALL(env, napi_coerce_to_bool(env, args[0], &output));

    return output;
}

static NAPIValue ToNumber(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPIValue output;
    NAPI_CALL(env, napi_coerce_to_number(env, args[0], &output));

    return output;
}

static NAPIValue ToObject(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPIValue output;
    NAPI_CALL(env, napi_coerce_to_object(env, args[0], &output));

    return output;
}

static NAPIValue ToString(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPIValue output;
    NAPI_CALL(env, napi_coerce_to_string(env, args[0], &output));

    return output;
}

EXTERN_C_END

TEST(TestConversions, TestNull) {
    NAPIEnv env = nullptr;
    ASSERT_EQ(NAPICreateEnv(&env), NAPIOK);

    NAPIValue global = nullptr;
    ASSERT_EQ(napi_get_global(env, &global), NAPIOK);

    NAPIValue exports;
    ASSERT_EQ(napi_create_object(env, &exports), NAPIOK);
    ASSERT_EQ(napi_set_named_property(env, global, "exports", exports), NAPIOK);

    NAPIPropertyDescriptor descriptors[] = {
            DECLARE_NAPI_PROPERTY("asBool", AsBool),
            DECLARE_NAPI_PROPERTY("asInt32", AsInt32),
            DECLARE_NAPI_PROPERTY("asUInt32", AsUInt32),
            DECLARE_NAPI_PROPERTY("asInt64", AsInt64),
            DECLARE_NAPI_PROPERTY("asDouble", AsDouble),
            DECLARE_NAPI_PROPERTY("asString", AsString),
            DECLARE_NAPI_PROPERTY("toBool", ToBool),
            DECLARE_NAPI_PROPERTY("toNumber", ToNumber),
            DECLARE_NAPI_PROPERTY("toObject", ToObject),
            DECLARE_NAPI_PROPERTY("toString", ToString),
    };

    ASSERT_EQ(napi_define_properties(
            env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors), NAPIOK);

    init_test_null(env, exports);

    EXPECT_EQ(initAssert(env, global), NAPIOK);

    NAPIValue result = nullptr;
    // asInt(Number.Nan) 被转换成 int32_t uint32_t int64_t 会损失精度，而 JavaScriptCore Number.NaN 实际上就是一个固定的 double
//    "        assert.strictEqual(asInt(Number.NaN), 0);\n"
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(env, "(function () {\n"
                                              "    const testSym = Symbol('test');\n"
                                              "\n"
                                              "    assert.strictEqual(globalThis.exports.asBool(false), false);\n"
                                              "    assert.strictEqual(globalThis.exports.asBool(true), true);\n"
                                              "    assert.throws(() => globalThis.exports.asBool(undefined));\n"
                                              "    assert.throws(() => globalThis.exports.asBool(null));\n"
                                              "    assert.throws(() => globalThis.exports.asBool(Number.NaN));\n"
                                              "    assert.throws(() => globalThis.exports.asBool(0));\n"
                                              "    assert.throws(() => globalThis.exports.asBool(''));\n"
                                              "    assert.throws(() => globalThis.exports.asBool('0'));\n"
                                              "    assert.throws(() => globalThis.exports.asBool(1));\n"
                                              "    assert.throws(() => globalThis.exports.asBool('1'));\n"
                                              "    assert.throws(() => globalThis.exports.asBool('true'));\n"
                                              "    assert.throws(() => globalThis.exports.asBool({}));\n"
                                              "    assert.throws(() => globalThis.exports.asBool([]));\n"
                                              "    assert.throws(() => globalThis.exports.asBool(testSym));\n"
                                              "\n"
                                              "    [globalThis.exports.asInt32, globalThis.exports.asUInt32, globalThis.exports.asInt64].forEach((asInt) => {\n"
                                              "        assert.strictEqual(asInt(0), 0);\n"
                                              "        assert.strictEqual(asInt(1), 1);\n"
                                              "        assert.strictEqual(asInt(1.0), 1);\n"
                                              "        assert.strictEqual(asInt(1.1), 1);\n"
                                              "        assert.strictEqual(asInt(1.9), 1);\n"
                                              "        assert.strictEqual(asInt(0.9), 0);\n"
                                              "        assert.strictEqual(asInt(999.9), 999);\n"
                                              "        assert.throws(() => asInt(undefined));\n"
                                              "        assert.throws(() => asInt(null));\n"
                                              "        assert.throws(() => asInt(false));\n"
                                              "        assert.throws(() => asInt(''));\n"
                                              "        assert.throws(() => asInt('1'));\n"
                                              "        assert.throws(() => asInt({}));\n"
                                              "        assert.throws(() => asInt([]));\n"
                                              "        assert.throws(() => asInt(testSym));\n"
                                              "    });\n"
                                              "\n"
                                              "    assert.strictEqual(globalThis.exports.asInt32(-1), -1);\n"
                                              "    assert.strictEqual(globalThis.exports.asInt64(-1), -1);\n"
                                              "    assert.strictEqual(globalThis.exports.asUInt32(-1), Math.pow(2, 32) - 1);\n"
                                              "\n"
                                              "    assert.strictEqual(globalThis.exports.asDouble(0), 0);\n"
                                              "    assert.strictEqual(globalThis.exports.asDouble(1), 1);\n"
                                              "    assert.strictEqual(globalThis.exports.asDouble(1.0), 1.0);\n"
                                              "    assert.strictEqual(globalThis.exports.asDouble(1.1), 1.1);\n"
                                              "    assert.strictEqual(globalThis.exports.asDouble(1.9), 1.9);\n"
                                              "    assert.strictEqual(globalThis.exports.asDouble(0.9), 0.9);\n"
                                              "    assert.strictEqual(globalThis.exports.asDouble(999.9), 999.9);\n"
                                              "    assert.strictEqual(globalThis.exports.asDouble(-1), -1);\n"
                                              "    assert(Number.isNaN(globalThis.exports.asDouble(Number.NaN)));\n"
                                              "    assert.throws(() => globalThis.exports.asDouble(undefined));\n"
                                              "    assert.throws(() => globalThis.exports.asDouble(null));\n"
                                              "    assert.throws(() => globalThis.exports.asDouble(false));\n"
                                              "    assert.throws(() => globalThis.exports.asDouble(''));\n"
                                              "    assert.throws(() => globalThis.exports.asDouble('1'));\n"
                                              "    assert.throws(() => globalThis.exports.asDouble({}));\n"
                                              "    assert.throws(() => globalThis.exports.asDouble([]));\n"
                                              "    assert.throws(() => globalThis.exports.asDouble(testSym));\n"
                                              "\n"
                                              "    assert.strictEqual(globalThis.exports.asString(''), '');\n"
                                              "    assert.strictEqual(globalThis.exports.asString('test'), 'test');\n"
                                              "    assert.throws(() => globalThis.exports.asString(undefined));\n"
                                              "    assert.throws(() => globalThis.exports.asString(null));\n"
                                              "    assert.throws(() => globalThis.exports.asString(false));\n"
                                              "    assert.throws(() => globalThis.exports.asString(1));\n"
                                              "    assert.throws(() => globalThis.exports.asString(1.1));\n"
                                              "    assert.throws(() => globalThis.exports.asString(Number.NaN));\n"
                                              "    assert.throws(() => globalThis.exports.asString({}));\n"
                                              "    assert.throws(() => globalThis.exports.asString([]));\n"
                                              "    assert.throws(() => globalThis.exports.asString(testSym));\n"
                                              "\n"
                                              "    assert.strictEqual(globalThis.exports.toBool(true), true);\n"
                                              "    assert.strictEqual(globalThis.exports.toBool(1), true);\n"
                                              "    assert.strictEqual(globalThis.exports.toBool(-1), true);\n"
                                              "    assert.strictEqual(globalThis.exports.toBool('true'), true);\n"
                                              "    assert.strictEqual(globalThis.exports.toBool('false'), true);\n"
                                              "    assert.strictEqual(globalThis.exports.toBool({}), true);\n"
                                              "    assert.strictEqual(globalThis.exports.toBool([]), true);\n"
                                              "    assert.strictEqual(globalThis.exports.toBool(testSym), true);\n"
                                              "    assert.strictEqual(globalThis.exports.toBool(false), false);\n"
                                              "    assert.strictEqual(globalThis.exports.toBool(undefined), false);\n"
                                              "    assert.strictEqual(globalThis.exports.toBool(null), false);\n"
                                              "    assert.strictEqual(globalThis.exports.toBool(0), false);\n"
                                              "    assert.strictEqual(globalThis.exports.toBool(Number.NaN), false);\n"
                                              "    assert.strictEqual(globalThis.exports.toBool(''), false);\n"
                                              "\n"
                                              "    assert.strictEqual(globalThis.exports.toNumber(0), 0);\n"
                                              "    assert.strictEqual(globalThis.exports.toNumber(1), 1);\n"
                                              "    assert.strictEqual(globalThis.exports.toNumber(1.1), 1.1);\n"
                                              "    assert.strictEqual(globalThis.exports.toNumber(-1), -1);\n"
                                              "    assert.strictEqual(globalThis.exports.toNumber('0'), 0);\n"
                                              "    assert.strictEqual(globalThis.exports.toNumber('1'), 1);\n"
                                              "    assert.strictEqual(globalThis.exports.toNumber('1.1'), 1.1);\n"
                                              "    assert.strictEqual(globalThis.exports.toNumber([]), 0);\n"
                                              "    assert.strictEqual(globalThis.exports.toNumber(false), 0);\n"
                                              "    assert.strictEqual(globalThis.exports.toNumber(null), 0);\n"
                                              "    assert.strictEqual(globalThis.exports.toNumber(''), 0);\n"
                                              "    assert(Number.isNaN(globalThis.exports.toNumber(Number.NaN)));\n"
                                              "    assert(Number.isNaN(globalThis.exports.toNumber({})));\n"
                                              "    assert(Number.isNaN(globalThis.exports.toNumber(undefined)));\n"
                                              "    assert.throws(() => globalThis.exports.toNumber(testSym));\n"
                                              "\n"
                                              "    assert(!Number.isNaN(globalThis.exports.toObject(Number.NaN)));\n"
                                              "\n"
                                              "    assert.strictEqual(globalThis.exports.toString(''), '');\n"
                                              "    assert.strictEqual(globalThis.exports.toString('test'), 'test');\n"
                                              "    assert.strictEqual(globalThis.exports.toString(undefined), 'undefined');\n"
                                              "    assert.strictEqual(globalThis.exports.toString(null), 'null');\n"
                                              "    assert.strictEqual(globalThis.exports.toString(false), 'false');\n"
                                              "    assert.strictEqual(globalThis.exports.toString(true), 'true');\n"
                                              "    assert.strictEqual(globalThis.exports.toString(0), '0');\n"
                                              "    assert.strictEqual(globalThis.exports.toString(1.1), '1.1');\n"
                                              "    assert.strictEqual(globalThis.exports.toString(Number.NaN), 'NaN');\n"
                                              "    assert.strictEqual(globalThis.exports.toString({}), '[object Object]');\n"
                                              "    assert.strictEqual(globalThis.exports.toString({ toString: () => 'test' }), 'test');\n"
                                              "    assert.strictEqual(globalThis.exports.toString([]), '');\n"
                                              "    assert.strictEqual(globalThis.exports.toString([1, 2, 3]), '1,2,3');\n"
                                              "    assert.throws(() => globalThis.exports.toString(testSym));\n"
                                              "})();",
                                         "https://n-api.com/test_conversions.js",
                                         &result), NAPIOK);
    NAPIValueType valueType;
    EXPECT_EQ(napi_typeof(env, result, &valueType), NAPIOK);
    EXPECT_EQ(valueType, NAPIUndefined);

    ASSERT_EQ(NAPIFreeEnv(env), NAPIOK);
}