#include <gtest/gtest.h>
#include <napi/js_native_api.h>
#include <napi_assert.h>

EXTERN_C_START

static NAPIValue checkError(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    bool r;
    NAPI_CALL(env, napi_is_error(env, args[0], &r));

    NAPIValue result;
    NAPI_CALL(env, napi_get_boolean(env, r, &result));

    return result;
}

static NAPIValue throwExistingError(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue message;
    NAPIValue error;
    NAPI_CALL(env, napi_create_string_utf8(
            env, "existing error", NAPI_AUTO_LENGTH, &message));
    NAPI_CALL(env, napi_create_error(env, NULL, message, &error));
    NAPI_CALL(env, napi_throw(env, error));
    return NULL;
}

static NAPIValue throwError(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPI_CALL(env, napi_throw_error(env, NULL, "error"));
    return NULL;
}

static NAPIValue throwRangeError(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPI_CALL(env, napi_throw_range_error(env, NULL, "range error"));
    return NULL;
}

static NAPIValue throwTypeError(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPI_CALL(env, napi_throw_type_error(env, NULL, "type error"));
    return NULL;
}

static NAPIValue throwErrorCode(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPI_CALL(env, napi_throw_error(env, "ERR_TEST_CODE", "Error [error]"));
    return NULL;
}

static NAPIValue throwRangeErrorCode(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPI_CALL(env, napi_throw_range_error(env,
                                          "ERR_TEST_CODE",
                                          "RangeError [range error]"));
    return NULL;
}

static NAPIValue throwTypeErrorCode(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPI_CALL(env, napi_throw_type_error(env,
                                         "ERR_TEST_CODE",
                                         "TypeError [type error]"));
    return NULL;
}


static NAPIValue createError(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue result;
    NAPIValue message;
    NAPI_CALL(env, napi_create_string_utf8(
            env, "error", NAPI_AUTO_LENGTH, &message));
    NAPI_CALL(env, napi_create_error(env, NULL, message, &result));
    return result;
}

static NAPIValue createRangeError(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue result;
    NAPIValue message;
    NAPI_CALL(env, napi_create_string_utf8(
            env, "range error", NAPI_AUTO_LENGTH, &message));
    NAPI_CALL(env, napi_create_range_error(env, NULL, message, &result));
    return result;
}

static NAPIValue createTypeError(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue result;
    NAPIValue message;
    NAPI_CALL(env, napi_create_string_utf8(
            env, "type error", NAPI_AUTO_LENGTH, &message));
    NAPI_CALL(env, napi_create_type_error(env, NULL, message, &result));
    return result;
}

static NAPIValue createErrorCode(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue result;
    NAPIValue message;
    NAPIValue code;
    NAPI_CALL(env, napi_create_string_utf8(
            env, "Error [error]", NAPI_AUTO_LENGTH, &message));
    NAPI_CALL(env, napi_create_string_utf8(
            env, "ERR_TEST_CODE", NAPI_AUTO_LENGTH, &code));
    NAPI_CALL(env, napi_create_error(env, code, message, &result));
    return result;
}

static NAPIValue createRangeErrorCode(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue result;
    NAPIValue message;
    NAPIValue code;
    NAPI_CALL(env, napi_create_string_utf8(env,
                                           "RangeError [range error]",
                                           NAPI_AUTO_LENGTH,
                                           &message));
    NAPI_CALL(env, napi_create_string_utf8(
            env, "ERR_TEST_CODE", NAPI_AUTO_LENGTH, &code));
    NAPI_CALL(env, napi_create_range_error(env, code, message, &result));
    return result;
}

static NAPIValue createTypeErrorCode(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue result;
    NAPIValue message;
    NAPIValue code;
    NAPI_CALL(env, napi_create_string_utf8(env,
                                           "TypeError [type error]",
                                           NAPI_AUTO_LENGTH,
                                           &message));
    NAPI_CALL(env, napi_create_string_utf8(
            env, "ERR_TEST_CODE", NAPI_AUTO_LENGTH, &code));
    NAPI_CALL(env, napi_create_type_error(env, code, message, &result));
    return result;
}

static NAPIValue throwArbitrary(NAPIEnv env, NAPICallbackInfo info) {
    NAPIValue arbitrary;
    size_t argc = 1;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &arbitrary, NULL, NULL));
    NAPI_CALL(env, napi_throw(env, arbitrary));
    return NULL;
}

EXTERN_C_END

TEST(TestError, Test) {
    NAPIEnv env = nullptr;
    ASSERT_EQ(NAPICreateEnv(&env), NAPIOK);

    NAPIValue global = nullptr;
    ASSERT_EQ(napi_get_global(env, &global), NAPIOK);

    NAPIValue exports;
    ASSERT_EQ(napi_create_object(env, &exports), NAPIOK);
    ASSERT_EQ(napi_set_named_property(env, global, "exports", exports), NAPIOK);

    NAPIPropertyDescriptor descriptors[] = {
            DECLARE_NAPI_PROPERTY("checkError", checkError),
            DECLARE_NAPI_PROPERTY("throwExistingError", throwExistingError),
            DECLARE_NAPI_PROPERTY("throwError", throwError),
            DECLARE_NAPI_PROPERTY("throwRangeError", throwRangeError),
            DECLARE_NAPI_PROPERTY("throwTypeError", throwTypeError),
            DECLARE_NAPI_PROPERTY("throwErrorCode", throwErrorCode),
            DECLARE_NAPI_PROPERTY("throwRangeErrorCode", throwRangeErrorCode),
            DECLARE_NAPI_PROPERTY("throwTypeErrorCode", throwTypeErrorCode),
            DECLARE_NAPI_PROPERTY("throwArbitrary", throwArbitrary),
            DECLARE_NAPI_PROPERTY("createError", createError),
            DECLARE_NAPI_PROPERTY("createRangeError", createRangeError),
            DECLARE_NAPI_PROPERTY("createTypeError", createTypeError),
            DECLARE_NAPI_PROPERTY("createErrorCode", createErrorCode),
            DECLARE_NAPI_PROPERTY("createRangeErrorCode", createRangeErrorCode),
            DECLARE_NAPI_PROPERTY("createTypeErrorCode", createTypeErrorCode),
    };

    ASSERT_EQ(napi_define_properties(
            env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors), NAPIOK);

    EXPECT_EQ(initAssert(env, global), NAPIOK);

    NAPIValue result = nullptr;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(env, "(function () {\n"
                                              "    const theError = new Error('Some error');\n"
                                              "    const theTypeError = new TypeError('Some type error');\n"
                                              "    const theSyntaxError = new SyntaxError('Some syntax error');\n"
                                              "    const theRangeError = new RangeError('Some type error');\n"
                                              "    const theReferenceError = new ReferenceError('Some reference error');\n"
                                              "    const theURIError = new URIError('Some URI error');\n"
                                              "    const theEvalError = new EvalError('Some eval error');\n"
                                              "\n"
                                              "    class MyError extends Error { }\n"
                                              "    const myError = new MyError('Some MyError');\n"
                                              "\n"
                                              "    // Test that native error object is correctly classed\n"
                                              "    assert.strictEqual(globalThis.exports.checkError(theError), true);\n"
                                              "\n"
                                              "    // Test that native type error object is correctly classed\n"
                                              "    assert.strictEqual(globalThis.exports.checkError(theTypeError), true);\n"
                                              "\n"
                                              "    // Test that native syntax error object is correctly classed\n"
                                              "    assert.strictEqual(globalThis.exports.checkError(theSyntaxError), true);\n"
                                              "\n"
                                              "    // Test that native range error object is correctly classed\n"
                                              "    assert.strictEqual(globalThis.exports.checkError(theRangeError), true);\n"
                                              "\n"
                                              "    // Test that native reference error object is correctly classed\n"
                                              "    assert.strictEqual(globalThis.exports.checkError(theReferenceError), true);\n"
                                              "\n"
                                              "    // Test that native URI error object is correctly classed\n"
                                              "    assert.strictEqual(globalThis.exports.checkError(theURIError), true);\n"
                                              "\n"
                                              "    // Test that native eval error object is correctly classed\n"
                                              "    assert.strictEqual(globalThis.exports.checkError(theEvalError), true);\n"
                                              "\n"
                                              "    // Test that class derived from native error is correctly classed\n"
                                              "    assert.strictEqual(globalThis.exports.checkError(myError), true);\n"
                                              "\n"
                                              "    // Test that non-error object is correctly classed\n"
                                              "    assert.strictEqual(globalThis.exports.checkError({}), false);\n"
                                              "\n"
                                              "    // Test that non-error primitive is correctly classed\n"
                                              "    assert.strictEqual(globalThis.exports.checkError('non-object'), false);\n"
                                              "\n"
                                              "    assert.throws(() => {\n"
                                              "        globalThis.exports.throwExistingError();\n"
                                              "    });\n"
                                              "\n"
                                              "    assert.throws(() => {\n"
                                              "        globalThis.exports.throwError();\n"
                                              "    });\n"
                                              "\n"
                                              "    assert.throws(() => {\n"
                                              "        globalThis.exports.throwRangeError();\n"
                                              "    });\n"
                                              "\n"
                                              "    assert.throws(() => {\n"
                                              "        globalThis.exports.throwTypeError();\n"
                                              "    });\n"
                                              "\n"
                                              "    [42, {}, [], Symbol('xyzzy'), true, 'ball', undefined, null, NaN]\n"
                                              "        .forEach((value) => assert.throws(\n"
                                              "            () => globalThis.exports.throwArbitrary(value)\n"
                                              "        ));\n"
                                              "\n"
                                              "    assert.throws(\n"
                                              "        () => globalThis.exports.throwErrorCode());\n"
                                              "\n"
                                              "    assert.throws(\n"
                                              "        () => globalThis.exports.throwRangeErrorCode());\n"
                                              "\n"
                                              "    assert.throws(\n"
                                              "        () => globalThis.exports.throwTypeErrorCode());\n"
                                              "\n"
                                              "    let error = globalThis.exports.createError();\n"
                                              "    assert(error instanceof Error);\n"
                                              "    assert.strictEqual(error.message, 'error');\n"
                                              "\n"
                                              "    error = globalThis.exports.createRangeError();\n"
                                              "    assert(error instanceof RangeError);\n"
                                              "    assert.strictEqual(error.message, 'range error');\n"
                                              "\n"
                                              "    error = globalThis.exports.createTypeError();\n"
                                              "    assert(error instanceof TypeError);\n"
                                              "    assert.strictEqual(error.message, 'type error');\n"
                                              "\n"
                                              "    error = globalThis.exports.createErrorCode();\n"
                                              "    assert(error instanceof Error);\n"
                                              "    assert.strictEqual(error.code, 'ERR_TEST_CODE');\n"
                                              "    assert.strictEqual(error.message, 'Error [error]');\n"
                                              "    assert.strictEqual(error.name, 'Error');\n"
                                              "\n"
                                              "    error = globalThis.exports.createRangeErrorCode();\n"
                                              "    assert(error instanceof RangeError);\n"
                                              "    assert.strictEqual(error.message, 'RangeError [range error]');\n"
                                              "    assert.strictEqual(error.code, 'ERR_TEST_CODE');\n"
                                              "    assert.strictEqual(error.name, 'RangeError');\n"
                                              "\n"
                                              "    error = globalThis.exports.createTypeErrorCode();\n"
                                              "    assert(error instanceof TypeError);\n"
                                              "    assert.strictEqual(error.message, 'TypeError [type error]');\n"
                                              "    assert.strictEqual(error.code, 'ERR_TEST_CODE');\n"
                                              "    assert.strictEqual(error.name, 'TypeError');\n"
                                              "})();",
                                         "https://n-api.com/test_error.js",
                                         &result), NAPIOK);
    NAPIValueType valueType;
    EXPECT_EQ(napi_typeof(env, result, &valueType), NAPIOK);
    EXPECT_EQ(valueType, NAPIUndefined);

    ASSERT_EQ(NAPIFreeEnv(env), NAPIOK);
}