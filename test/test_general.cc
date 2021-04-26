#include <gtest/gtest.h>
#include <napi/js_native_api.h>
#include <napi_assert.h>

EXTERN_C_START

static NAPIValue testStrictEquals(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 2;
    NAPIValue args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    bool bool_result;
    NAPIValue result;
    NAPI_CALL(env, napi_strict_equals(env, args[0], args[1], &bool_result));
    NAPI_CALL(env, napi_get_boolean(env, bool_result, &result));

    return result;
}

static NAPIValue testGetPrototype(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPIValue result;
    NAPI_CALL(env, napi_get_prototype(env, args[0], &result));

    return result;
}

static NAPIValue doInstanceOf(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 2;
    NAPIValue args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    bool instanceof;
    NAPI_CALL(env, napi_instanceof(env, args[0], args[1], &instanceof));

    NAPIValue result;
    NAPI_CALL(env, napi_get_boolean(env, instanceof, &result));

    return result;
}

static NAPIValue getNull(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue result;
    NAPI_CALL(env, napi_get_null(env, &result));
    return result;
}

static NAPIValue getUndefined(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue result;
    NAPI_CALL(env, napi_get_undefined(env, &result));
    return result;
}

static NAPIValue createNapiError(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue value;
    NAPI_CALL(env, napi_create_string_utf8(env, "xyz", -1, &value));

    double double_value;
    NAPIStatus status = napi_get_value_double(env, value, &double_value);

    NAPI_ASSERT(env, status != NAPIOK, "Failed to produce error condition");

    const NAPIExtendedErrorInfo *error_info = 0;
    NAPI_CALL(env, napi_get_last_error_info(env, &error_info));

    NAPI_ASSERT(env, error_info->errorCode == status,
                "Last error info code should match last status");
    NAPI_ASSERT(env, error_info->errorMessage,
                "Last error info message should not be null");

    return NULL;
}

static NAPIValue testNapiErrorCleanup(NAPIEnv env, NAPICallbackInfo /*info*/) {
    const NAPIExtendedErrorInfo *error_info = 0;
    NAPI_CALL(env, napi_get_last_error_info(env, &error_info));

    NAPIValue result;
    bool is_ok = error_info->errorCode == NAPIOK;
    NAPI_CALL(env, napi_get_boolean(env, is_ok, &result));

    return result;
}

static NAPIValue testNapiTypeof(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPIValueType argument_type;
    NAPI_CALL(env, napi_typeof(env, args[0], &argument_type));

    NAPIValue result = NULL;
    if (argument_type == NAPINumber) {
        NAPI_CALL(env, napi_create_string_utf8(
                env, "number", NAPI_AUTO_LENGTH, &result));
    } else if (argument_type == NAPIString) {
        NAPI_CALL(env, napi_create_string_utf8(
                env, "string", NAPI_AUTO_LENGTH, &result));
    } else if (argument_type == NAPIFunction) {
        NAPI_CALL(env, napi_create_string_utf8(
                env, "function", NAPI_AUTO_LENGTH, &result));
    } else if (argument_type == NAPIObject) {
        NAPI_CALL(env, napi_create_string_utf8(
                env, "object", NAPI_AUTO_LENGTH, &result));
    } else if (argument_type == NAPIBoolean) {
        NAPI_CALL(env, napi_create_string_utf8(
                env, "boolean", NAPI_AUTO_LENGTH, &result));
    } else if (argument_type == NAPIUndefined) {
        NAPI_CALL(env, napi_create_string_utf8(
                env, "undefined", NAPI_AUTO_LENGTH, &result));
    } else if (argument_type == NAPISymbol) {
        NAPI_CALL(env, napi_create_string_utf8(
                env, "symbol", NAPI_AUTO_LENGTH, &result));
    } else if (argument_type == NAPINull) {
        NAPI_CALL(env, napi_create_string_utf8(
                env, "null", NAPI_AUTO_LENGTH, &result));
    }
    return result;
}

static bool deref_item_called = false;
static void deref_item(NAPIEnv env, void *data, void *hint) {
    (void) hint;

    NAPI_ASSERT_RETURN_VOID(env, data == &deref_item_called,
                            "Finalize callback was called with the correct pointer");

    deref_item_called = true;
}

static NAPIValue deref_item_was_called(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue it_was_called;

    NAPI_CALL(env, napi_get_boolean(env, deref_item_called, &it_was_called));

    return it_was_called;
}

static NAPIValue wrap_first_arg(NAPIEnv env,
                                NAPICallbackInfo info,
                                NAPIFinalize finalizer,
                                void *data) {
    size_t argc = 1;
    NAPIValue to_wrap;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &to_wrap, NULL, NULL));
    NAPI_CALL(env, napi_wrap(env, to_wrap, data, finalizer, NULL, NULL));

    return to_wrap;
}

static NAPIValue wrap(NAPIEnv env, NAPICallbackInfo info) {
    deref_item_called = false;
    return wrap_first_arg(env, info, deref_item, &deref_item_called);
}

static NAPIValue unwrap(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue wrapped;
    void *data;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &wrapped, NULL, NULL));
    NAPI_CALL(env, napi_unwrap(env, wrapped, &data));

    return NULL;
}

static NAPIValue remove_wrap(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue wrapped;
    void *data;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &wrapped, NULL, NULL));
    NAPI_CALL(env, napi_remove_wrap(env, wrapped, &data));

    return NULL;
}

static bool finalize_called = false;
static void test_finalize(NAPIEnv /*env*/, void */*data*/, void */*hint*/) {
    finalize_called = true;
}

static NAPIValue test_finalize_wrap(NAPIEnv env, NAPICallbackInfo info) {
    return wrap_first_arg(env, info, test_finalize, NULL);
}

static NAPIValue finalize_was_called(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue it_was_called;

    NAPI_CALL(env, napi_get_boolean(env, finalize_called, &it_was_called));

    return it_was_called;
}

static NAPIValue testAdjustExternalMemory(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue result;
    int64_t adjustedValue;

    NAPI_CALL(env, napi_adjust_external_memory(env, 1, &adjustedValue));
    NAPI_CALL(env, napi_create_double(env, (double) adjustedValue, &result));

    return result;
}

static NAPIValue testNapiRun(NAPIEnv env, NAPICallbackInfo info) {
    NAPIValue script, result;
    size_t argc = 1;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &script, NULL, NULL));

    NAPI_CALL(env, napi_run_script(env, script, &result));

    return result;
}

static const char *env_cleanup_finalizer_messages[] = {
        "simple wrap",
        "wrap, removeWrap",
        "first wrap",
        "second wrap"
};

static void cleanup_env_finalizer(NAPIEnv env, void *data, void *hint) {
    (void) env;
    (void) hint;

    printf("finalize at env cleanup for %s\n",
           env_cleanup_finalizer_messages[(uintptr_t) data]);
}

static NAPIValue env_cleanup_wrap(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 2;
    NAPIValue argv[2];
    uint32_t value;
    uintptr_t ptr_value;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    NAPI_CALL(env, napi_get_value_uint32(env, argv[1], &value));

    ptr_value = value;
    return wrap_first_arg(env, info, cleanup_env_finalizer, (void *) ptr_value);
}

EXTERN_C_END

TEST(TestGeneral, Test) {
    NAPIEnv env = nullptr;
    ASSERT_EQ(NAPICreateEnv(&env), NAPIOK);

    NAPIValue global = nullptr;
    ASSERT_EQ(napi_get_global(env, &global), NAPIOK);

    NAPIValue exports;
    ASSERT_EQ(napi_create_object(env, &exports), NAPIOK);
    ASSERT_EQ(napi_set_named_property(env, global, "exports", exports), NAPIOK);

    NAPIPropertyDescriptor descriptors[] = {
            DECLARE_NAPI_PROPERTY("testStrictEquals", testStrictEquals),
            DECLARE_NAPI_PROPERTY("testGetPrototype", testGetPrototype),
            DECLARE_NAPI_PROPERTY("testNapiRun", testNapiRun),
            DECLARE_NAPI_PROPERTY("doInstanceOf", doInstanceOf),
            DECLARE_NAPI_PROPERTY("getUndefined", getUndefined),
            DECLARE_NAPI_PROPERTY("getNull", getNull),
            DECLARE_NAPI_PROPERTY("createNapiError", createNapiError),
            DECLARE_NAPI_PROPERTY("testNapiErrorCleanup", testNapiErrorCleanup),
            DECLARE_NAPI_PROPERTY("testNapiTypeof", testNapiTypeof),
            DECLARE_NAPI_PROPERTY("wrap", wrap),
            DECLARE_NAPI_PROPERTY("envCleanupWrap", env_cleanup_wrap),
            DECLARE_NAPI_PROPERTY("unwrap", unwrap),
            DECLARE_NAPI_PROPERTY("removeWrap", remove_wrap),
            DECLARE_NAPI_PROPERTY("testFinalizeWrap", test_finalize_wrap),
            DECLARE_NAPI_PROPERTY("finalizeWasCalled", finalize_was_called),
            DECLARE_NAPI_PROPERTY("derefItemWasCalled", deref_item_was_called),
            DECLARE_NAPI_PROPERTY("testAdjustExternalMemory", testAdjustExternalMemory)
    };

    ASSERT_EQ(napi_define_properties(
            env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors), NAPIOK);

    EXPECT_EQ(initAssert(env, global), NAPIOK);

    NAPIValue result = nullptr;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(env, "(function () {\n"
                                              "    const val1 = '1';\n"
                                              "    const val2 = 1;\n"
                                              "    const val3 = 1;\n"
                                              "\n"
                                              "    class BaseClass {\n"
                                              "    }\n"
                                              "\n"
                                              "    class ExtendedClass extends BaseClass {\n"
                                              "    }\n"
                                              "\n"
                                              "    const baseObject = new BaseClass();\n"
                                              "    const extendedObject = new ExtendedClass();\n"
                                              "\n"
                                              "    // Test napi_strict_equals\n"
                                              "    assert(globalThis.exports.testStrictEquals(val1, val1));\n"
                                              "    assert.strictEqual(globalThis.exports.testStrictEquals(val1, val2), false);\n"
                                              "    assert(globalThis.exports.testStrictEquals(val2, val3));\n"
                                              "\n"
                                              "    // Test napi_get_prototype\n"
                                              "    assert.strictEqual(globalThis.exports.testGetPrototype(baseObject),\n"
                                              "        Object.getPrototypeOf(baseObject));\n"
                                              "    assert.strictEqual(globalThis.exports.testGetPrototype(extendedObject),\n"
                                              "        Object.getPrototypeOf(extendedObject));\n"
                                              "    // Prototypes for base and extended should be different.\n"
                                              "    assert.notStrictEqual(globalThis.exports.testGetPrototype(baseObject),\n"
                                              "        globalThis.exports.testGetPrototype(extendedObject));\n"
                                              "\n"
                                              "    [\n"
                                              "        123,\n"
                                              "        'test string',\n"
                                              "        function () { },\n"
                                              "        new Object(),\n"
                                              "        true,\n"
                                              "        undefined,\n"
                                              "        Symbol()\n"
                                              "    ].forEach((val) => {\n"
                                              "        assert.strictEqual(globalThis.exports.testNapiTypeof(val), typeof val);\n"
                                              "    });\n"
                                              "\n"
                                              "    // Since typeof in js return object need to validate specific case\n"
                                              "    // for null\n"
                                              "    assert.strictEqual(globalThis.exports.testNapiTypeof(null), 'null');\n"
                                              "\n"
                                              "    // Assert that wrapping twice fails.\n"
                                              "    const x = {};\n"
                                              "    globalThis.exports.wrap(x);\n"
                                              "    assert.throws(() => globalThis.exports.wrap(x));\n"
                                              "    // Clean up here, otherwise derefItemWasCalled() will be polluted.\n"
                                              "    globalThis.exports.removeWrap(x);\n"
                                              "\n"
                                              "    // Ensure that wrapping, removing the wrap, and then wrapping again works.\n"
                                              "    const y = {};\n"
                                              "    globalThis.exports.wrap(y);\n"
                                              "    globalThis.exports.removeWrap(y);\n"
                                              "    // Wrapping twice succeeds if a remove_wrap() separates the instances\n"
                                              "    globalThis.exports.wrap(y);\n"
                                              "    // Clean up here, otherwise derefItemWasCalled() will be polluted.\n"
                                              "    globalThis.exports.removeWrap(y);\n"
                                              "\n"
                                              "    // Test napi_adjust_external_memory\n"
                                              "    const adjustedValue = globalThis.exports.testAdjustExternalMemory();\n"
                                              "    assert.strictEqual(typeof adjustedValue, 'number');\n"
                                              "    assert(adjustedValue > 0);\n"
                                              "})();",
                                         "https://n-api.com/test_general_test.js",
                                         &result), NAPIOK);
    NAPIValueType valueType;
    EXPECT_EQ(napi_typeof(env, result, &valueType), NAPIOK);
    EXPECT_EQ(valueType, NAPIUndefined);

    result = nullptr;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(env, "(function () {\n"
                                              "    assert.strictEqual(globalThis.exports.getUndefined(), undefined);\n"
                                              "    assert.strictEqual(globalThis.exports.getNull(), null);\n"
                                              "})();",
                                         "https://n-api.com/test_general_testGlobals.js",
                                         &result), NAPIOK);
    EXPECT_EQ(napi_typeof(env, result, &valueType), NAPIOK);
    EXPECT_EQ(valueType, NAPIUndefined);

    result = nullptr;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(env, "(function () {\n"
                                              "    // We can only perform this test if we have a working Symbol.hasInstance\n"
                                              "    if (typeof Symbol !== 'undefined' && 'hasInstance' in Symbol &&\n"
                                              "        typeof Symbol.hasInstance === 'symbol') {\n"
                                              "\n"
                                              "        function compareToNative(theObject, theConstructor) {\n"
                                              "            assert.strictEqual(\n"
                                              "                globalThis.exports.doInstanceOf(theObject, theConstructor),\n"
                                              "                (theObject instanceof theConstructor)\n"
                                              "            );\n"
                                              "        }\n"
                                              "\n"
                                              "        function MyClass() { }\n"
                                              "        Object.defineProperty(MyClass, Symbol.hasInstance, {\n"
                                              "            value: function (candidate) {\n"
                                              "                return 'mark' in candidate;\n"
                                              "            }\n"
                                              "        });\n"
                                              "\n"
                                              "        function MySubClass() { }\n"
                                              "        MySubClass.prototype = new MyClass();\n"
                                              "\n"
                                              "        let x = new MySubClass();\n"
                                              "        let y = new MySubClass();\n"
                                              "        x.mark = true;\n"
                                              "\n"
                                              "        compareToNative(x, MySubClass);\n"
                                              "        compareToNative(y, MySubClass);\n"
                                              "        compareToNative(x, MyClass);\n"
                                              "        compareToNative(y, MyClass);\n"
                                              "\n"
                                              "        x = new MyClass();\n"
                                              "        y = new MyClass();\n"
                                              "        x.mark = true;\n"
                                              "\n"
                                              "        compareToNative(x, MySubClass);\n"
                                              "        compareToNative(y, MySubClass);\n"
                                              "        compareToNative(x, MyClass);\n"
                                              "        compareToNative(y, MyClass);\n"
                                              "    }\n"
                                              "})();",
                                         "https://n-api.com/test_general_testInstanceOf.js",
                                         &result), NAPIOK);
    EXPECT_EQ(napi_typeof(env, result, &valueType), NAPIOK);
    EXPECT_EQ(valueType, NAPIUndefined);

    result = nullptr;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(env, "(function () {\n"
                                              "    const testCase = '(41.92 + 0.08);';\n"
                                              "    const expected = 42;\n"
                                              "    const actual = globalThis.exports.testNapiRun(testCase);\n"
                                              "\n"
                                              "    assert.strictEqual(actual, expected);\n"
                                              "    assert.throws(() => globalThis.exports.testNapiRun({ abc: 'def' }));\n"
                                              "})();",
                                         "https://n-api.com/test_general_testNapiRun.js",
                                         &result), NAPIOK);
    EXPECT_EQ(napi_typeof(env, result, &valueType), NAPIOK);
    EXPECT_EQ(valueType, NAPIUndefined);

    result = nullptr;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(env, "(function () {\n"
                                              "    globalThis.exports.createNapiError();\n"
                                              "    assert(globalThis.exports.testNapiErrorCleanup());\n"
                                              "})();",
                                         "https://n-api.com/test_general_testNapiStatus.js",
                                         &result), NAPIOK);
    EXPECT_EQ(napi_typeof(env, result, &valueType), NAPIOK);
    EXPECT_EQ(valueType, NAPIUndefined);

    ASSERT_EQ(NAPIFreeEnv(env), NAPIOK);
}