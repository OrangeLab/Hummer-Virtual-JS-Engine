#include <common.h>
#include <js_native_api_test.h>

EXTERN_C_START

static NAPIValue TestCreateFunctionParameters(NAPIEnv env, NAPICallbackInfo /*info*/)
{
    NAPIStatus status;
    NAPIValue result, return_value;

    NAPI_CALL(env, napi_create_object(env, &return_value));

    status = napi_create_function(nullptr, "TrackedFunction", NAPI_AUTO_LENGTH, TestCreateFunctionParameters, nullptr,
                                  &result);

    add_returned_status(env, "envIsNull", return_value, "Invalid argument", NAPIInvalidArg, status);

    napi_create_function(env, nullptr, NAPI_AUTO_LENGTH, TestCreateFunctionParameters, nullptr, &result);

    add_last_status(env, "nameIsNull", return_value);

    napi_create_function(env, "TrackedFunction", NAPI_AUTO_LENGTH, nullptr, nullptr, &result);

    add_last_status(env, "cbIsNull", return_value);

    napi_create_function(env, "TrackedFunction", NAPI_AUTO_LENGTH, TestCreateFunctionParameters, nullptr, nullptr);

    add_last_status(env, "resultIsNull", return_value);

    return return_value;
}

static NAPIValue TestCallFunction(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 10;
    NAPIValue args[10];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc > 0, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == NAPIFunction, "Wrong type of arguments. Expects a function as first argument.");

    NAPIValue *argv = args + 1;
    argc = argc - 1;

    NAPIValue global;
    NAPI_CALL(env, napi_get_global(env, &global));

    NAPIValue result;
    NAPI_CALL(env, napi_call_function(env, global, args[0], argc, argv, &result));

    return result;
}

static NAPIValue TestFunctionName(NAPIEnv /*env*/, NAPICallbackInfo /*info*/)
{
    return nullptr;
}

static void finalize_function(NAPIEnv env, void *data, void * /*hint*/)
{
    auto ref = static_cast<NAPIRef>(data);

    // Retrieve the JavaScript undefined value.
    NAPIValue undefined;
    NAPI_CALL_RETURN_VOID(env, napi_get_undefined(env, &undefined));

    // Retrieve the JavaScript function we must call.
    NAPIValue js_function;
    NAPI_CALL_RETURN_VOID(env, napi_get_reference_value(env, ref, &js_function));

    // Call the JavaScript function to indicate that the generated JavaScript
    // function is about to be gc-ed.
    NAPI_CALL_RETURN_VOID(env, napi_call_function(env, undefined, js_function, 0, nullptr, nullptr));

    // Destroy the persistent reference to the function we just called so as to
    // properly clean up.
    NAPI_CALL_RETURN_VOID(env, napi_delete_reference(env, ref));
}

static NAPIValue MakeTrackedFunction(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue js_finalize_cb;
    NAPIValueType arg_type;

    // Retrieve and validate from the arguments the function we will use to
    // indicate to JavaScript that the function we are about to create is about
    // to be gc-ed.
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &js_finalize_cb, nullptr, nullptr));
    NAPI_ASSERT(env, argc == 1, "Wrong number of arguments");
    NAPI_CALL(env, napi_typeof(env, js_finalize_cb, &arg_type));
    NAPI_ASSERT(env, arg_type == NAPIFunction, "Argument must be a function");

    // Dynamically create a function.
    NAPIValue result;
    NAPI_CALL(env, napi_create_function(env, "TrackedFunction", NAPI_AUTO_LENGTH, TestFunctionName, nullptr, &result));

    // Create a strong reference to the function we will call when the tracked
    // function is about to be gc-ed.
    NAPIRef js_finalize_cb_ref;
    NAPI_CALL(env, napi_create_reference(env, js_finalize_cb, 1, &js_finalize_cb_ref));

    // Attach a finalizer to the dynamically created function and pass it the
    // strong reference we created in the previous step.
    NAPI_CALL(env, napi_wrap(env, result, js_finalize_cb_ref, finalize_function, nullptr, nullptr));

    return result;
}

EXTERN_C_END

TEST_F(Test, TestFunction)
{
    NAPIValue fn1;
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, NAPI_AUTO_LENGTH, TestCallFunction, nullptr, &fn1), NAPIOK);

    NAPIValue fn2;
    ASSERT_EQ(napi_create_function(globalEnv, "Name", NAPI_AUTO_LENGTH, TestFunctionName, nullptr, &fn2), NAPIOK);

    NAPIValue fn3;
    ASSERT_EQ(napi_create_function(globalEnv, "Name_extra", -1, TestFunctionName, nullptr, &fn3), NAPIOK);

    NAPIValue fn4;
    ASSERT_EQ(
        napi_create_function(globalEnv, "MakeTrackedFunction", NAPI_AUTO_LENGTH, MakeTrackedFunction, nullptr, &fn4),
        NAPIOK);

    NAPIValue fn5;
    ASSERT_EQ(napi_create_function(globalEnv, "TestCreateFunctionParameters", NAPI_AUTO_LENGTH,
                                   TestCreateFunctionParameters, nullptr, &fn5),
              NAPIOK);

    ASSERT_EQ(napi_set_named_property(globalEnv, exports, "TestCall", fn1), NAPIOK);
    ASSERT_EQ(napi_set_named_property(globalEnv, exports, "TestName", fn2), NAPIOK);
    ASSERT_EQ(napi_set_named_property(globalEnv, exports, "TestNameShort", fn3), NAPIOK);
    ASSERT_EQ(napi_set_named_property(globalEnv, exports, "MakeTrackedFunction", fn4), NAPIOK);

    ASSERT_EQ(napi_set_named_property(globalEnv, exports, "TestCreateFunctionParameters", fn5), NAPIOK);

    NAPIValue result;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(
                  globalEnv,
                  "(()=>{\"use strict\";function l(l){return "
                  "l+1}globalThis.assert.strictEqual(globalThis.addon.TestCall((function(){return "
                  "1})),1),globalThis.assert.strictEqual(globalThis.addon.TestCall((function(){return "
                  "null})),null),globalThis.assert.strictEqual(globalThis.addon.TestCall(l,1),2),globalThis.assert."
                  "strictEqual(globalThis.addon.TestCall((function(a){return "
                  "l(a)}),1),2),globalThis.assert.strictEqual(globalThis.addon.TestName.name,\"Name\"),globalThis."
                  "assert.strictEqual(globalThis.addon.TestNameShort.name,\"Name_extra\")})();",
                  "https://www.napi.com/test_function.js", &result),
              NAPIOK);
}