#include <gtest/gtest.h>
#include <napi/js_native_api.h>
#include <napi_assert.h>

EXTERN_C_START

static double value_ = 1;

static NAPIValue GetValue(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 0;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, NULL, NULL, NULL));

    NAPI_ASSERT(env, argc == 0, "Wrong number of arguments");

    NAPIValue number;
    NAPI_CALL(env, napi_create_double(env, value_, &number));

    return number;
}

static NAPIValue SetValue(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc == 1, "Wrong number of arguments");

    NAPI_CALL(env, napi_get_value_double(env, args[0], &value_));

    return NULL;
}

static NAPIValue Echo(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc == 1, "Wrong number of arguments");

    return args[0];
}

static NAPIValue HasNamedProperty(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 2;
    NAPIValue args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc == 2, "Wrong number of arguments");

    // Extract the name of the property to check
    char buffer[128];
    size_t copied;
    NAPI_CALL(env,
              napi_get_value_string_utf8(env, args[1], buffer, sizeof(buffer), &copied));

    // do the check and create the boolean return value
    bool value;
    NAPIValue result;
    NAPI_CALL(env, napi_has_named_property(env, args[0], buffer, &value));
    NAPI_CALL(env, napi_get_boolean(env, value, &result));

    return result;
}

EXTERN_C_END

TEST(TestProperties, Test) {
    NAPIEnv env = nullptr;
    ASSERT_EQ(NAPICreateEnv(&env), NAPIOK);

    NAPIValue global = nullptr;
    ASSERT_EQ(napi_get_global(env, &global), NAPIOK);

    NAPIValue exports;
    ASSERT_EQ(napi_create_object(env, &exports), NAPIOK);
    ASSERT_EQ(napi_set_named_property(env, global, "exports", exports), NAPIOK);

    NAPIValue number;
    ASSERT_EQ(napi_create_double(env, value_, &number), NAPIOK);

    NAPIValue name_value;
    ASSERT_EQ(napi_create_string_utf8(env,
                                      "NameKeyValue",
                                      NAPI_AUTO_LENGTH,
                                      &name_value), NAPIOK);

    NAPIValue symbol_description;
    NAPIValue name_symbol;
    ASSERT_EQ(napi_create_string_utf8(env,
                                      "NameKeySymbol",
                                      NAPI_AUTO_LENGTH,
                                      &symbol_description), NAPIOK);
    ASSERT_EQ(napi_create_symbol(env,
                                 symbol_description,
                                 &name_symbol), NAPIOK);

    NAPIPropertyDescriptor properties[] = {
            {"echo",               0,           Echo,             0,        0,        0,      NAPIEnumerable,        0},
            {"readwriteValue",     0,           0,                0,        0,        number, static_cast<NAPIPropertyAttributes>(
                                                                                                      NAPIEnumerable |
                                                                                                      NAPIWritable), 0},
            {"readonlyValue",      0,           0,                0,        0,        number, NAPIEnumerable,        0},
            {"hiddenValue",        0,           0,                0,        0,        number, NAPIDefault,           0},
            {NULL,                 name_value,  0,                0,        0,        number, NAPIEnumerable,        0},
            {NULL,                 name_symbol, 0,                0,        0,        number, NAPIEnumerable,        0},
            {"readwriteAccessor1", 0,           0,                GetValue, SetValue, 0,      NAPIDefault,           0},
            {"readwriteAccessor2", 0,           0,                GetValue, SetValue, 0,      NAPIWritable,          0},
            {"readonlyAccessor1",  0,           0,                GetValue, NULL,     0,      NAPIDefault,           0},
            {"readonlyAccessor2",  0,           0,                GetValue, NULL,     0,      NAPIWritable,          0},
            {"hasNamedProperty",   0,           HasNamedProperty, 0,        0,        0,      NAPIDefault,           0},
    };

    ASSERT_EQ(napi_define_properties(
            env, exports, sizeof(properties) / sizeof(*properties), properties), NAPIOK);

    EXPECT_EQ(initAssert(env, global), NAPIOK);

    NAPIValue result = nullptr;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(env, "(function () {\n"
                                              "    assert.strictEqual(globalThis.exports.echo('hello'), 'hello');\n"
                                              "\n"
                                              "    globalThis.exports.readwriteValue = 1;\n"
                                              "    assert.strictEqual(globalThis.exports.readwriteValue, 1);\n"
                                              "    globalThis.exports.readwriteValue = 2;\n"
                                              "    assert.strictEqual(globalThis.exports.readwriteValue, 2);\n"
                                              "\n"
                                              "    assert(globalThis.exports.hiddenValue);\n"
                                              "\n"
                                              "    // Properties with napi_enumerable attribute should be enumerable.\n"
                                              "    const propertyNames = [];\n"
                                              "    for (const name in globalThis.exports) {\n"
                                              "        propertyNames.push(name);\n"
                                              "    }\n"
                                              "    assert(propertyNames.includes('echo'));\n"
                                              "    assert(propertyNames.includes('readwriteValue'));\n"
                                              "    assert(propertyNames.includes('readonlyValue'));\n"
                                              "    assert(!propertyNames.includes('hiddenValue'));\n"
                                              "    assert(propertyNames.includes('NameKeyValue'));\n"
                                              "    assert(!propertyNames.includes('readwriteAccessor1'));\n"
                                              "    assert(!propertyNames.includes('readwriteAccessor2'));\n"
                                              "    assert(!propertyNames.includes('readonlyAccessor1'));\n"
                                              "    assert(!propertyNames.includes('readonlyAccessor2'));\n"
                                              "\n"
                                              "    // Validate property created with symbol\n"
                                              "    const start = 'Symbol('.length;\n"
                                              "    const end = start + 'NameKeySymbol'.length;\n"
                                              "    const symbolDescription =\n"
                                              "        String(Object.getOwnPropertySymbols(globalThis.exports)[0]).slice(start, end);\n"
                                              "    assert.strictEqual(symbolDescription, 'NameKeySymbol');\n"
                                              "\n"
                                              "    // The napi_writable attribute should be ignored for accessors.\n"
                                              "    const readwriteAccessor1Descriptor =\n"
                                              "        Object.getOwnPropertyDescriptor(globalThis.exports, 'readwriteAccessor1');\n"
                                              "    const readonlyAccessor1Descriptor =\n"
                                              "        Object.getOwnPropertyDescriptor(globalThis.exports, 'readonlyAccessor1');\n"
                                              "    assert(readwriteAccessor1Descriptor.get != null);\n"
                                              "    assert(readwriteAccessor1Descriptor.set != null);\n"
                                              "    assert(readwriteAccessor1Descriptor.value === undefined);\n"
                                              "    assert(readonlyAccessor1Descriptor.get != null);\n"
                                              "    assert(readonlyAccessor1Descriptor.set === undefined);\n"
                                              "    assert(readonlyAccessor1Descriptor.value === undefined);\n"
                                              "    globalThis.exports.readwriteAccessor1 = 1;\n"
                                              "    assert.strictEqual(globalThis.exports.readwriteAccessor1, 1);\n"
                                              "    assert.strictEqual(globalThis.exports.readonlyAccessor1, 1);\n"
                                              "    globalThis.exports.readwriteAccessor2 = 2;\n"
                                              "    assert.strictEqual(globalThis.exports.readwriteAccessor2, 2);\n"
                                              "    assert.strictEqual(globalThis.exports.readonlyAccessor2, 2);\n"
                                              "\n"
                                              "    assert.strictEqual(globalThis.exports.hasNamedProperty(globalThis.exports, 'echo'), true);\n"
                                              "    assert.strictEqual(globalThis.exports.hasNamedProperty(globalThis.exports, 'hiddenValue'),\n"
                                              "        true);\n"
                                              "    assert.strictEqual(globalThis.exports.hasNamedProperty(globalThis.exports, 'doesnotexist'),\n"
                                              "        false);\n"
                                              "})();",
                                         "https://n-api.com/test_properties.js",
                                         &result), NAPIOK);
    NAPIValueType valueType;
    EXPECT_EQ(napi_typeof(env, result, &valueType), NAPIOK);
    EXPECT_EQ(valueType, NAPIUndefined);

    ASSERT_EQ(NAPIFreeEnv(env), NAPIOK);
}