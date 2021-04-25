#include <gtest/gtest.h>
#include <napi/js_native_api.h>
#include <napi_assert.h>

EXTERN_C_START

static double value_ = 1;
static double static_value_ = 10;

static NAPIValue TestDefineClass(NAPIEnv env,
                                 NAPICallbackInfo /*info*/) {
    NAPIStatus status;
    NAPIValue result, return_value;

    NAPIPropertyDescriptor property_descriptor = {
            "TestDefineClass",
            nullptr,
            TestDefineClass,
            nullptr,
            nullptr,
            nullptr,
            static_cast<NAPIPropertyAttributes>(NAPIEnumerable | NAPIStatic),
            nullptr};

    NAPI_CALL(env, napi_create_object(env, &return_value));

    status = napi_define_class(nullptr,
                               "TrackedFunction",
                               NAPI_AUTO_LENGTH,
                               TestDefineClass,
                               nullptr,
                               1,
                               &property_descriptor,
                               &result);

    add_returned_status(env,
                        "envIsnullptr",
                        return_value,
                        "Invalid argument",
                        NAPIInvalidArg,
                        status);

    napi_define_class(env,
                      nullptr,
                      NAPI_AUTO_LENGTH,
                      TestDefineClass,
                      nullptr,
                      1,
                      &property_descriptor,
                      &result);

    add_last_status(env, "nameIsnullptr", return_value);

    napi_define_class(env,
                      "TrackedFunction",
                      NAPI_AUTO_LENGTH,
                      nullptr,
                      nullptr,
                      1,
                      &property_descriptor,
                      &result);

    add_last_status(env, "cbIsnullptr", return_value);

    napi_define_class(env,
                      "TrackedFunction",
                      NAPI_AUTO_LENGTH,
                      TestDefineClass,
                      nullptr,
                      1,
                      &property_descriptor,
                      &result);

    add_last_status(env, "cbDataIsnullptr", return_value);

    napi_define_class(env,
                      "TrackedFunction",
                      NAPI_AUTO_LENGTH,
                      TestDefineClass,
                      nullptr,
                      1,
                      nullptr,
                      &result);

    add_last_status(env, "propertiesIsnullptr", return_value);


    napi_define_class(env,
                      "TrackedFunction",
                      NAPI_AUTO_LENGTH,
                      TestDefineClass,
                      nullptr,
                      1,
                      &property_descriptor,
                      nullptr);

    add_last_status(env, "resultIsnullptr", return_value);

    return return_value;
}

static NAPIValue GetValue(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 0;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, nullptr));

    NAPI_ASSERT(env, argc == 0, "Wrong number of arguments");

    NAPIValue number;
    NAPI_CALL(env, napi_create_double(env, value_, &number));

    return number;
}

static NAPIValue SetValue(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc == 1, "Wrong number of arguments");

    NAPI_CALL(env, napi_get_value_double(env, args[0], &value_));

    return nullptr;
}

static NAPIValue Echo(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc == 1, "Wrong number of arguments");

    return args[0];
}

static NAPIValue New(NAPIEnv env, NAPICallbackInfo info) {
    NAPIValue _this;
    NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &_this, nullptr));

    return _this;
}

static NAPIValue GetStaticValue(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 0;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, nullptr));

    NAPI_ASSERT(env, argc == 0, "Wrong number of arguments");

    NAPIValue number;
    NAPI_CALL(env, napi_create_double(env, static_value_, &number));

    return number;
}


static NAPIValue NewExtra(NAPIEnv env, NAPICallbackInfo info) {
    NAPIValue _this;
    NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &_this, nullptr));

    return _this;
}

EXTERN_C_END

TEST(TestConstructor, Test) {
    NAPIEnv env = nullptr;
    ASSERT_EQ(NAPICreateEnv(&env), NAPIOK);

    NAPIValue global = nullptr;
    ASSERT_EQ(napi_get_global(env, &global), NAPIOK);

    NAPIValue number, cons;
    ASSERT_EQ(napi_create_double(env, value_, &number), NAPIOK);

    // JavaScriptCore 必须使用 NAPI_AUTO_LENGTH
    ASSERT_EQ(napi_define_class(
            env, "MyObject_Extra", -1, NewExtra, nullptr, 0, nullptr, &cons), NAPIOK);

    NAPIPropertyDescriptor properties[] = {
            {"echo",                    nullptr, Echo,            nullptr, nullptr, nullptr, NAPIEnumerable,        nullptr},
            {"readwriteValue",          nullptr, nullptr,         nullptr, nullptr, number,
                                                                                             static_cast<NAPIPropertyAttributes>(
                                                                                                     NAPIEnumerable |
                                                                                                     NAPIWritable), nullptr},
            {"readonlyValue",           nullptr, nullptr,         nullptr, nullptr, number,  NAPIEnumerable,
                                                                                                                    nullptr},
            {"hiddenValue",             nullptr, nullptr,         nullptr, nullptr, number,  NAPIDefault,           nullptr},
            {"readwriteAccessor1",      nullptr, nullptr, GetValue, SetValue,       nullptr, NAPIDefault,
                                                                                                                    nullptr},
            {"readwriteAccessor2",      nullptr, nullptr, GetValue, SetValue,       nullptr,
                                                                                             NAPIWritable,          nullptr},
            {"readonlyAccessor1",       nullptr, nullptr, GetValue,        nullptr, nullptr, NAPIDefault,
                                                                                                                    nullptr},
            {"readonlyAccessor2",       nullptr, nullptr, GetValue,        nullptr, nullptr, NAPIWritable,
                                                                                                                    nullptr},
            {"staticReadonlyAccessor1", nullptr, nullptr, GetStaticValue,  nullptr, nullptr,
                                                                                             static_cast<NAPIPropertyAttributes>(
                                                                                                     NAPIDefault |
                                                                                                     NAPIStatic),   nullptr},
            {"constructorName",         nullptr, nullptr,         nullptr, nullptr, cons,
                                                                                             static_cast<NAPIPropertyAttributes>(
                                                                                                     NAPIEnumerable |
                                                                                                     NAPIStatic),   nullptr},
            {"TestDefineClass",         nullptr, TestDefineClass, nullptr, nullptr, nullptr,
                                                                                             static_cast<NAPIPropertyAttributes>(
                                                                                                     NAPIEnumerable |
                                                                                                     NAPIStatic),   nullptr},
    };

    ASSERT_EQ(napi_define_class(env, "MyObject", NAPI_AUTO_LENGTH, New,
                                nullptr, sizeof(properties) / sizeof(*properties), properties, &cons), NAPIOK);

    EXPECT_EQ(napi_set_named_property(env, global, "TestConstructor", cons), NAPIOK);

    EXPECT_EQ(initAssert(env, global), NAPIOK);

    NAPIValue result = nullptr;
//    "    assert.throws(() => { test_object.readonlyValue = 3; });\n"
//    "    assert.throws(() => { test_object.readonlyAccessor1 = 3; });\n"
//    "    assert.throws(() => { test_object.readonlyAccessor2 = 3; });\n"
//    assert.deepStrictEqual(TestConstructor.TestDefineClass(), {
//            envIsNull: 'Invalid argument',
//            nameIsNull: 'Invalid argument',
//            cbIsNull: 'Invalid argument',
//            cbDataIsNull: 'napi_ok',
//            propertiesIsNull: 'Invalid argument',
//            resultIsNull: 'Invalid argument'
//    });
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(env, "(function () {\n"
                                              "    const test_object = new globalThis.TestConstructor();\n"
                                              "\n"
                                              "    assert.strictEqual(test_object.echo('hello'), 'hello');\n"
                                              "\n"
                                              "    test_object.readwriteValue = 1;\n"
                                              "    assert.strictEqual(test_object.readwriteValue, 1);\n"
                                              "    test_object.readwriteValue = 2;\n"
                                              "    assert.strictEqual(test_object.readwriteValue, 2);\n"
                                              "\n"
                                              "    assert(test_object.hiddenValue);\n"
                                              "\n"
                                              "    // Properties with napi_enumerable attribute should be enumerable.\n"
                                              "    const propertyNames = [];\n"
                                              "    for (const name in test_object) {\n"
                                              "        propertyNames.push(name);\n"
                                              "    }\n"
                                              "    assert(propertyNames.includes('echo'));\n"
                                              "    assert(propertyNames.includes('readwriteValue'));\n"
                                              "    assert(propertyNames.includes('readonlyValue'));\n"
                                              "    assert(!propertyNames.includes('hiddenValue'));\n"
                                              "    assert(!propertyNames.includes('readwriteAccessor1'));\n"
                                              "    assert(!propertyNames.includes('readwriteAccessor2'));\n"
                                              "    assert(!propertyNames.includes('readonlyAccessor1'));\n"
                                              "    assert(!propertyNames.includes('readonlyAccessor2'));\n"
                                              "\n"
                                              "    // The napi_writable attribute should be ignored for accessors.\n"
                                              "    test_object.readwriteAccessor1 = 1;\n"
                                              "    assert.strictEqual(test_object.readwriteAccessor1, 1);\n"
                                              "    assert.strictEqual(test_object.readonlyAccessor1, 1);\n"
                                              "    test_object.readwriteAccessor2 = 2;\n"
                                              "    assert.strictEqual(test_object.readwriteAccessor2, 2);\n"
                                              "    assert.strictEqual(test_object.readonlyAccessor2, 2);\n"
                                              "\n"
                                              "    // Validate that static properties are on the class as opposed\n"
                                              "    // to the instance\n"
                                              "    assert.strictEqual(globalThis.TestConstructor.staticReadonlyAccessor1, 10);\n"
                                              "    assert.strictEqual(test_object.staticReadonlyAccessor1, undefined);\n"
                                              "})();",
                                         "https://n-api.com/test_constructor_test.js",
                                         &result), NAPIOK);
    NAPIValueType valueType;
    EXPECT_EQ(napi_typeof(env, result, &valueType), NAPIOK);
    EXPECT_EQ(valueType, NAPIUndefined);

    result = nullptr;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(env, "(function () {\n"
                                              "    assert.strictEqual(globalThis.TestConstructor.name, 'MyObject');\n"
                                              "})();",
                                         "https://n-api.com/test_constructor_test2.js",
                                         &result), NAPIOK);
    EXPECT_EQ(napi_typeof(env, result, &valueType), NAPIOK);
    EXPECT_EQ(valueType, NAPIUndefined);

    ASSERT_EQ(NAPIFreeEnv(env), NAPIOK);
}