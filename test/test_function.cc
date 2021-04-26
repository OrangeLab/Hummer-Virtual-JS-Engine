#include <gtest/gtest.h>
#include <napi/js_native_api.h>
#include <napi_assert.h>

EXTERN_C_START

static NAPIValue TestCreateFunctionParameters(NAPIEnv env,
                                              NAPICallbackInfo /*info*/) {
    NAPIStatus status;
    NAPIValue result, return_value;

    NAPI_CALL(env, napi_create_object(env, &return_value));

    status = napi_create_function(NULL,
                                  "TrackedFunction",
                                  NAPI_AUTO_LENGTH,
                                  TestCreateFunctionParameters,
                                  NULL,
                                  &result);

    add_returned_status(env,
                        "envIsNull",
                        return_value,
                        "Invalid argument",
                        NAPIInvalidArg,
                        status);

    napi_create_function(env,
                         NULL,
                         NAPI_AUTO_LENGTH,
                         TestCreateFunctionParameters,
                         NULL,
                         &result);

    add_last_status(env, "nameIsNull", return_value);

    napi_create_function(env,
                         "TrackedFunction",
                         NAPI_AUTO_LENGTH,
                         NULL,
                         NULL,
                         &result);

    add_last_status(env, "cbIsNull", return_value);

    napi_create_function(env,
                         "TrackedFunction",
                         NAPI_AUTO_LENGTH,
                         TestCreateFunctionParameters,
                         NULL,
                         NULL);

    add_last_status(env, "resultIsNull", return_value);

    return return_value;
}

static NAPIValue TestCallFunction(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 10;
    NAPIValue args[10];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc > 0, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == NAPIFunction,
                "Wrong type of arguments. Expects a function as first argument.");

    NAPIValue *argv = args + 1;
    argc = argc - 1;

    NAPIValue global;
    NAPI_CALL(env, napi_get_global(env, &global));

    NAPIValue result;
    NAPI_CALL(env, napi_call_function(env, global, args[0], argc, argv, &result));

    return result;
}

static NAPIValue TestFunctionName(NAPIEnv /*env*/, NAPICallbackInfo /*info*/) {
    return NULL;
}

static void finalize_function(NAPIEnv env, void *data, void */*hint*/) {
    NAPIRef ref = static_cast<NAPIRef>(data);

    // Retrieve the JavaScript undefined value.
    NAPIValue undefined;
    NAPI_CALL_RETURN_VOID(env, napi_get_undefined(env, &undefined));

    // Retrieve the JavaScript function we must call.
    NAPIValue js_function;
    NAPI_CALL_RETURN_VOID(env, napi_get_reference_value(env, ref, &js_function));

    // Call the JavaScript function to indicate that the generated JavaScript
    // function is about to be gc-ed.
    NAPI_CALL_RETURN_VOID(env, napi_call_function(env,
                                                  undefined,
                                                  js_function,
                                                  0,
                                                  NULL,
                                                  NULL));

    // Destroy the persistent reference to the function we just called so as to
    // properly clean up.
    NAPI_CALL_RETURN_VOID(env, napi_delete_reference(env, ref));
}

static NAPIValue MakeTrackedFunction(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue js_finalize_cb;
    NAPIValueType arg_type;

    // Retrieve and validate from the arguments the function we will use to
    // indicate to JavaScript that the function we are about to create is about to
    // be gc-ed.
    NAPI_CALL(env, napi_get_cb_info(env,
                                    info,
                                    &argc,
                                    &js_finalize_cb,
                                    NULL,
                                    NULL));
    NAPI_ASSERT(env, argc == 1, "Wrong number of arguments");
    NAPI_CALL(env, napi_typeof(env, js_finalize_cb, &arg_type));
    NAPI_ASSERT(env, arg_type == NAPIFunction, "Argument must be a function");

    // Dynamically create a function.
    NAPIValue result;
    NAPI_CALL(env, napi_create_function(env,
                                        "TrackedFunction",
                                        NAPI_AUTO_LENGTH,
                                        TestFunctionName,
                                        NULL,
                                        &result));

    // Create a strong reference to the function we will call when the tracked
    // function is about to be gc-ed.
    NAPIRef js_finalize_cb_ref;
    NAPI_CALL(env, napi_create_reference(env,
                                         js_finalize_cb,
                                         1,
                                         &js_finalize_cb_ref));

    // Attach a finalizer to the dynamically created function and pass it the
    // strong reference we created in the previous step.
    NAPI_CALL(env, napi_wrap(env,
                             result,
                             js_finalize_cb_ref,
                             finalize_function,
                             NULL,
                             NULL));

    return result;
}

EXTERN_C_END

TEST(TestFunction, Test) {
    NAPIEnv env = nullptr;
    ASSERT_EQ(NAPICreateEnv(&env), NAPIOK);

    NAPIValue global = nullptr;
    ASSERT_EQ(napi_get_global(env, &global), NAPIOK);

    NAPIValue exports;
    ASSERT_EQ(napi_create_object(env, &exports), NAPIOK);
    ASSERT_EQ(napi_set_named_property(env, global, "exports", exports), NAPIOK);

    NAPIValue fn1;
    ASSERT_EQ(napi_create_function(
            env, NULL, NAPI_AUTO_LENGTH, TestCallFunction, NULL, &fn1), NAPIOK);

    NAPIValue fn2;
    ASSERT_EQ(napi_create_function(
            env, "Name", NAPI_AUTO_LENGTH, TestFunctionName, NULL, &fn2), NAPIOK);

    NAPIValue fn3;
    ASSERT_EQ(napi_create_function(
            env, "Name_extra", -1, TestFunctionName, NULL, &fn3), NAPIOK);

    NAPIValue fn4;
    ASSERT_EQ(napi_create_function(env,
                                   "MakeTrackedFunction",
                                   NAPI_AUTO_LENGTH,
                                   MakeTrackedFunction,
                                   NULL,
                                   &fn4), NAPIOK);

    NAPIValue fn5;
    ASSERT_EQ(napi_create_function(env,
                                   "TestCreateFunctionParameters",
                                   NAPI_AUTO_LENGTH,
                                   TestCreateFunctionParameters,
                                   NULL,
                                   &fn5), NAPIOK);

    ASSERT_EQ(napi_set_named_property(env, exports, "TestCall", fn1), NAPIOK);
    ASSERT_EQ(napi_set_named_property(env, exports, "TestName", fn2), NAPIOK);
    ASSERT_EQ(napi_set_named_property(env, exports, "TestNameShort", fn3), NAPIOK);
    ASSERT_EQ(napi_set_named_property(env,
                                      exports,
                                      "MakeTrackedFunction",
                                      fn4), NAPIOK);

    ASSERT_EQ(napi_set_named_property(env,
                                      exports,
                                      "TestCreateFunctionParameters",
                                      fn5), NAPIOK);

    EXPECT_EQ(initAssert(env, global), NAPIOK);

    NAPIValue result = nullptr;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(env, "(function () {\n"
                                              "    function func1() {\n"
                                              "        return 1;\n"
                                              "    }\n"
                                              "    assert.strictEqual(globalThis.exports.TestCall(func1), 1);\n"
                                              "\n"
                                              "    function func2() {\n"
                                              "        console.log('hello world!');\n"
                                              "        return null;\n"
                                              "    }\n"
                                              "    assert.strictEqual(globalThis.exports.TestCall(func2), null);\n"
                                              "\n"
                                              "    function func3(input) {\n"
                                              "        return input + 1;\n"
                                              "    }\n"
                                              "    assert.strictEqual(globalThis.exports.TestCall(func3, 1), 2);\n"
                                              "\n"
                                              "    function func4(input) {\n"
                                              "        return func3(input);\n"
                                              "    }\n"
                                              "    assert.strictEqual(globalThis.exports.TestCall(func4, 1), 2);\n"
                                              "\n"
                                              "    assert.strictEqual(globalThis.exports.TestName.name, 'Name');\n"
                                              "    assert.strictEqual(globalThis.exports.TestNameShort.name, 'Name_extra');\n"
                                              "})();",
                                         "https://n-api.com/test_function.js",
                                         &result), NAPIOK);
    NAPIValueType valueType;
    EXPECT_EQ(napi_typeof(env, result, &valueType), NAPIOK);
    EXPECT_EQ(valueType, NAPIUndefined);

    ASSERT_EQ(NAPIFreeEnv(env), NAPIOK);
}