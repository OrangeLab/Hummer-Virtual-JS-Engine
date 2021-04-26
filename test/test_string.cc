#include <gtest/gtest.h>
#include <napi/js_native_api.h>
#include <napi_assert.h>

EXTERN_C_START

static NAPIValue TestUtf8(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValueType valuetype;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));

    NAPI_ASSERT(env, valuetype == NAPIString,
                "Wrong type of argment. Expects a string.");

    char buffer[128];
    size_t buffer_size = 128;
    size_t copied;

    NAPI_CALL(env,
              napi_get_value_string_utf8(env, args[0], buffer, buffer_size, &copied));

    NAPIValue output;
    NAPI_CALL(env, napi_create_string_utf8(env, buffer, -1, &output));

    return output;
}

static NAPIValue TestUtf16(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValueType valuetype;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));

    NAPI_ASSERT(env, valuetype == NAPIString,
                "Wrong type of argment. Expects a string.");

    char16_t buffer[128];
    size_t buffer_size = 128;
    size_t copied;

    NAPI_CALL(env,
              napi_get_value_string_utf16(env, args[0], buffer, buffer_size, &copied));

    NAPIValue output;
    NAPI_CALL(env, napi_create_string_utf16(env, buffer, copied, &output));

    return output;
}

static NAPIValue TestUtf8Insufficient(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValueType valuetype;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));

    NAPI_ASSERT(env, valuetype == NAPIString,
                "Wrong type of argment. Expects a string.");

    char buffer[4];
    size_t buffer_size = 4;
    size_t copied;

    NAPI_CALL(env,
              napi_get_value_string_utf8(env, args[0], buffer, buffer_size, &copied));

    NAPIValue output;
    NAPI_CALL(env, napi_create_string_utf8(env, buffer, -1, &output));

    return output;
}

static NAPIValue TestUtf16Insufficient(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValueType valuetype;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));

    NAPI_ASSERT(env, valuetype == NAPIString,
                "Wrong type of argment. Expects a string.");

    char16_t buffer[4];
    size_t buffer_size = 4;
    size_t copied;

    NAPI_CALL(env,
              napi_get_value_string_utf16(env, args[0], buffer, buffer_size, &copied));

    NAPIValue output;
    NAPI_CALL(env, napi_create_string_utf16(env, buffer, copied, &output));

    return output;
}

static NAPIValue Utf16Length(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValueType valuetype;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));

    NAPI_ASSERT(env, valuetype == NAPIString,
                "Wrong type of argment. Expects a string.");

    size_t length;
    NAPI_CALL(env, napi_get_value_string_utf16(env, args[0], NULL, 0, &length));

    NAPIValue output;
    NAPI_CALL(env, napi_create_uint32(env, (uint32_t) length, &output));

    return output;
}

static NAPIValue Utf8Length(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValueType valuetype;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));

    NAPI_ASSERT(env, valuetype == NAPIString,
                "Wrong type of argment. Expects a string.");

    size_t length;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], NULL, 0, &length));

    NAPIValue output;
    NAPI_CALL(env, napi_create_uint32(env, (uint32_t) length, &output));

    return output;
}

static NAPIValue TestMemoryCorruption(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc == 1, "Wrong number of arguments");

    char buf[10] = {0};
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], buf, 0, NULL));

    char zero[10] = {0};
    if (memcmp(buf, zero, sizeof(buf)) != 0) {
        NAPI_CALL(env, napi_throw_error(env, NULL, "Buffer overwritten"));
    }

    return NULL;
}

EXTERN_C_END

TEST(TestString, Test) {
    NAPIEnv env = nullptr;
    ASSERT_EQ(NAPICreateEnv(&env), NAPIOK);

    NAPIValue global = nullptr;
    ASSERT_EQ(napi_get_global(env, &global), NAPIOK);

    NAPIValue exports;
    ASSERT_EQ(napi_create_object(env, &exports), NAPIOK);
    ASSERT_EQ(napi_set_named_property(env, global, "exports", exports), NAPIOK);

    NAPIPropertyDescriptor properties[] = {
            DECLARE_NAPI_PROPERTY("TestUtf8", TestUtf8),
            DECLARE_NAPI_PROPERTY("TestUtf8Insufficient", TestUtf8Insufficient),
            DECLARE_NAPI_PROPERTY("TestUtf16", TestUtf16),
            DECLARE_NAPI_PROPERTY("TestUtf16Insufficient", TestUtf16Insufficient),
            DECLARE_NAPI_PROPERTY("Utf16Length", Utf16Length),
            DECLARE_NAPI_PROPERTY("Utf8Length", Utf8Length),
            DECLARE_NAPI_PROPERTY("TestMemoryCorruption", TestMemoryCorruption),
    };

    ASSERT_EQ(napi_define_properties(
            env, exports, sizeof(properties) / sizeof(*properties), properties), NAPIOK);

    EXPECT_EQ(initAssert(env, global), NAPIOK);

    NAPIValue result = nullptr;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(env, "(function () {\n"
                                              "    const empty = '';\n"
                                              "    assert.strictEqual(globalThis.exports.TestUtf8(empty), empty);\n"
                                              "    assert.strictEqual(globalThis.exports.TestUtf16(empty), empty);\n"
                                              "    assert.strictEqual(globalThis.exports.Utf16Length(empty), 0);\n"
                                              "    assert.strictEqual(globalThis.exports.Utf8Length(empty), 0);\n"
                                              "\n"
                                              "    const str1 = 'hello world';\n"
                                              "    assert.strictEqual(globalThis.exports.TestUtf8(str1), str1);\n"
                                              "    assert.strictEqual(globalThis.exports.TestUtf16(str1), str1);\n"
                                              "    assert.strictEqual(globalThis.exports.TestUtf8Insufficient(str1), str1.slice(0, 3));\n"
                                              "    assert.strictEqual(globalThis.exports.TestUtf16Insufficient(str1), str1.slice(0, 3));\n"
                                              "    assert.strictEqual(globalThis.exports.Utf16Length(str1), 11);\n"
                                              "    assert.strictEqual(globalThis.exports.Utf8Length(str1), 11);\n"
                                              "\n"
                                              "    const str2 = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';\n"
                                              "    assert.strictEqual(globalThis.exports.TestUtf8(str2), str2);\n"
                                              "    assert.strictEqual(globalThis.exports.TestUtf16(str2), str2);\n"
                                              "    assert.strictEqual(globalThis.exports.TestUtf8Insufficient(str2), str2.slice(0, 3));\n"
                                              "    assert.strictEqual(globalThis.exports.TestUtf16Insufficient(str2), str2.slice(0, 3));\n"
                                              "    assert.strictEqual(globalThis.exports.Utf16Length(str2), 62);\n"
                                              "    assert.strictEqual(globalThis.exports.Utf8Length(str2), 62);\n"
                                              "\n"
                                              "    const str3 = '?!@#$%^&*()_+-=[]{}/.,<>\\'\"\\\\';\n"
                                              "    assert.strictEqual(globalThis.exports.TestUtf8(str3), str3);\n"
                                              "    assert.strictEqual(globalThis.exports.TestUtf16(str3), str3);\n"
                                              "    assert.strictEqual(globalThis.exports.TestUtf8Insufficient(str3), str3.slice(0, 3));\n"
                                              "    assert.strictEqual(globalThis.exports.TestUtf16Insufficient(str3), str3.slice(0, 3));\n"
                                              "    assert.strictEqual(globalThis.exports.Utf16Length(str3), 27);\n"
                                              "    assert.strictEqual(globalThis.exports.Utf8Length(str3), 27);\n"
                                              "\n"
                                              "    const str4 = '¡¢£¤¥¦§¨©ª«¬\u00AD®¯°±²³´µ¶·¸¹º»¼½¾¿';\n"
                                              "    assert.strictEqual(globalThis.exports.TestUtf8(str4), str4);\n"
                                              "    assert.strictEqual(globalThis.exports.TestUtf16(str4), str4);\n"
                                              "    assert.strictEqual(globalThis.exports.TestUtf8Insufficient(str4), str4.slice(0, 1));\n"
                                              "    assert.strictEqual(globalThis.exports.TestUtf16Insufficient(str4), str4.slice(0, 3));\n"
                                              "    assert.strictEqual(globalThis.exports.Utf16Length(str4), 31);\n"
                                              "    assert.strictEqual(globalThis.exports.Utf8Length(str4), 62);\n"
                                              "\n"
                                              "    const str5 = 'ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþ';\n"
                                              "    assert.strictEqual(globalThis.exports.TestUtf8(str5), str5);\n"
                                              "    assert.strictEqual(globalThis.exports.TestUtf16(str5), str5);\n"
                                              "    assert.strictEqual(globalThis.exports.TestUtf8Insufficient(str5), str5.slice(0, 1));\n"
                                              "    assert.strictEqual(globalThis.exports.TestUtf16Insufficient(str5), str5.slice(0, 3));\n"
                                              "    assert.strictEqual(globalThis.exports.Utf16Length(str5), 63);\n"
                                              "    assert.strictEqual(globalThis.exports.Utf8Length(str5), 126);\n"
                                              "\n"
                                              "    const str6 = '\\u{2003}\\u{2101}\\u{2001}\\u{202}\\u{2011}';\n"
                                              "    assert.strictEqual(globalThis.exports.TestUtf8(str6), str6);\n"
                                              "    assert.strictEqual(globalThis.exports.TestUtf16(str6), str6);\n"
                                              "    assert.strictEqual(globalThis.exports.TestUtf8Insufficient(str6), str6.slice(0, 1));\n"
                                              "    assert.strictEqual(globalThis.exports.TestUtf16Insufficient(str6), str6.slice(0, 3));\n"
                                              "    assert.strictEqual(globalThis.exports.Utf16Length(str6), 5);\n"
                                              "    assert.strictEqual(globalThis.exports.Utf8Length(str6), 14);\n"
                                              "\n"
                                              "    globalThis.exports.TestMemoryCorruption(' '.repeat(64 * 1024));\n"
                                              "})();",
                                         "https://n-api.com/test_string.js",
                                         &result), NAPIOK);
    NAPIValueType valueType;
    EXPECT_EQ(napi_typeof(env, result, &valueType), NAPIOK);
    EXPECT_EQ(valueType, NAPIUndefined);

    ASSERT_EQ(NAPIFreeEnv(env), NAPIOK);
}