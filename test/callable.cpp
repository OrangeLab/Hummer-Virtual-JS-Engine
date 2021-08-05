#include <test.h>

EXTERN_C_START

static NAPIValue runWithCNullThis(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue argv[1];
    assert(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr) == NAPICommonOK);
    assert(argc == 1);
    NAPIValueType valueType;
    assert(napi_typeof(env, argv[0], &valueType) == NAPICommonOK);
    assert(valueType == NAPIFunction);
    assert(napi_call_function(env, nullptr, argv[0], 0, nullptr, nullptr) == NAPIExceptionOK);

    return nullptr;
}

static NAPIValue run(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue argv[1];
    assert(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr) == NAPICommonOK);
    assert(argc == 1);
    NAPIValueType valueType;
    assert(napi_typeof(env, argv[0], &valueType) == NAPICommonOK);
    assert(valueType == NAPIFunction);
    NAPIValue global;
    assert(napi_get_global(env, &global) == NAPIErrorOK);
    assert(napi_call_function(env, global, argv[0], 0, nullptr, nullptr) == NAPIExceptionOK);

    return nullptr;
}

static NAPIValue runWithArgument(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 3;
    NAPIValue argv[3];
    assert(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr) == NAPICommonOK);
    assert(argc == 3);
    NAPIValueType valueType;
    assert(napi_typeof(env, argv[0], &valueType) == NAPICommonOK);
    assert(valueType == NAPIFunction);
    assert(napi_typeof(env, argv[1], &valueType) == NAPICommonOK);
    assert(valueType == NAPIString);
    assert(napi_typeof(env, argv[2], &valueType) == NAPICommonOK);
    assert(valueType == NAPIString);
    NAPIValue args[] = {argv[1], argv[2]};
    assert(napi_call_function(env, nullptr, argv[0], 2, args, nullptr) == NAPIExceptionOK);

    return nullptr;
}

static NAPIValue runWithThis(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 2;
    NAPIValue argv[2];
    assert(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr) == NAPICommonOK);
    assert(argc == 2);
    NAPIValueType valueType;
    assert(napi_typeof(env, argv[0], &valueType) == NAPICommonOK);
    assert(valueType == NAPIFunction);
    assert(napi_typeof(env, argv[1], &valueType) == NAPICommonOK);
    assert(valueType == NAPIObject);
    assert(napi_call_function(env, argv[1], argv[0], 0, nullptr, nullptr) == NAPIExceptionOK);

    return nullptr;
}

static NAPIValue throwWithArgument(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue argv[1];
    void *data;
    assert(napi_get_cb_info(env, info, &argc, argv, nullptr, &data) == NAPICommonOK);
    assert(data == env);
    assert(argc == 1);
    assert(napi_throw(env, argv[0]) == NAPIExceptionOK);

    return nullptr;
}

static NAPIValue runWithCatch(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue argv[1];
    assert(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr) == NAPICommonOK);
    assert(argc == 1);
    NAPIValueType valueType;
    assert(napi_typeof(env, argv[0], &valueType) == NAPICommonOK);
    assert(valueType == NAPIFunction);
    assert(napi_call_function(env, nullptr, argv[0], 0, nullptr, nullptr) == NAPIExceptionPendingException);
    NAPIValue output;
    assert(napi_get_and_clear_last_exception(env, &output) == NAPIErrorOK);

    return output;
}

static NAPIValue newWithCatch(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 2;
    NAPIValue argv[2];
    assert(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr) == NAPICommonOK);
    assert(argc == 2);
    NAPIValueType valueType;
    assert(napi_typeof(env, argv[0], &valueType) == NAPICommonOK);
    assert(valueType == NAPIFunction);
    NAPIValue thisValue;
    assert(napi_new_instance(env, argv[0], 1, &argv[1], &thisValue) == NAPIExceptionPendingException);
    NAPIValue output;
    assert(napi_get_and_clear_last_exception(env, &output) == NAPIErrorOK);

    return output;
}

static NAPIValue returnWithArgument(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue argv[1];
    assert(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr) == NAPICommonOK);
    assert(argc == 1);

    return argv[0];
}

static NAPIValue returnWithThis(NAPIEnv env, NAPICallbackInfo info)
{
    NAPIValue output;
    assert(napi_get_cb_info(env, info, nullptr, nullptr, &output, nullptr) == NAPICommonOK);

    return output;
}

static NAPIValue returnWithCNull(NAPIEnv /*env*/, NAPICallbackInfo /*info*/)
{
    return nullptr;
}

EXTERN_C_END

TEST_F(Test, Callable)
{
    NAPIValue runWithUndefinedThisValue, runValue, runWithArgumentValue, runWithThisValue, throwValue,
        runWithCatchValue, newWithCatchValue;
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, runWithCNullThis, nullptr, &runWithUndefinedThisValue),
              NAPIExceptionOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, run, nullptr, &runValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, runWithArgument, nullptr, &runWithArgumentValue),
              NAPIExceptionOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, runWithThis, nullptr, &runWithThisValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, throwWithArgument, globalEnv, &throwValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, runWithCatch, nullptr, &runWithCatchValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, newWithCatch, nullptr, &newWithCatchValue), NAPIExceptionOK);
    NAPIValue ctorValue, returnWithArgumentValue, returnWithThisValue, returnWithCNullValue;
    ASSERT_EQ(NAPIDefineClass(globalEnv, nullptr, throwWithArgument, globalEnv, &ctorValue), NAPIExceptionOK);
    ASSERT_EQ(NAPIDefineClass(globalEnv, nullptr, returnWithArgument, globalEnv, &returnWithArgumentValue),
              NAPIExceptionOK);
    ASSERT_EQ(NAPIDefineClass(globalEnv, nullptr, returnWithThis, globalEnv, &returnWithThisValue), NAPIExceptionOK);
    ASSERT_EQ(NAPIDefineClass(globalEnv, nullptr, returnWithCNull, globalEnv, &returnWithCNullValue), NAPIExceptionOK);
    NAPIValue stringValue;
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "runWithCNullThis", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, runWithUndefinedThisValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "run", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, runValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "runWithArgument", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, runWithArgumentValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "runWithThis", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, runWithThisValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "throwWithArgument", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, throwValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "runWithCatch", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, runWithCatchValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "ctor", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, ctorValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "newWithCatch", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, newWithCatchValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "returnWithArgument", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, returnWithArgumentValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "returnWithThis", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, returnWithThisValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "returnWithCNull", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, returnWithCNullValue), NAPIExceptionOK);
    ASSERT_EQ(
        NAPIRunScript(
            globalEnv,
            "(()=>{\"use strict\";var "
            "l=!1;globalThis.addon.runWithCNullThis((function(){l=!0,globalThis.assert(this===globalThis)})),"
            "globalThis.assert(l),l=!1,globalThis.addon.run((function(){l=!0,globalThis.assert(this===globalThis)})),"
            "globalThis.assert(l);var "
            "a={};globalThis.addon.runWithThis((function(){globalThis.assert(this===a)}),a),globalThis.addon."
            "runWithArgument((function(){globalThis.assert(2===arguments.length),globalThis.assert(\"hello\"===("
            "arguments.length<=0?void 0:arguments[0])),globalThis.assert(\"world\"===(arguments.length<=1?void "
            "0:arguments[1]))}),\"hello\",\"world\");try{globalThis.addon.throwWithArgument(null)}catch(l){globalThis."
            "assert(null===l)}try{globalThis.addon.throwWithArgument(\"Error\")}catch(l){globalThis.assert(\"Error\"==="
            "l)}var t={};try{globalThis.addon.throwWithArgument(t)}catch(l){globalThis.assert(l===t)}try{new "
            "globalThis.addon.ctor(null)}catch(l){globalThis.assert(null===l)}try{new "
            "globalThis.addon.ctor(\"Error\")}catch(l){globalThis.assert(\"Error\"===l)}try{new "
            "globalThis.addon.ctor(t)}catch(l){globalThis.assert(l===t)}globalThis.assert(null===globalThis.addon."
            "runWithCatch((function(){throw "
            "null}))),globalThis.assert(\"null\"===globalThis.addon.runWithCatch((function(){throw\"null\"}))),"
            "globalThis.assert(globalThis.addon.runWithCatch((function(){throw t}))===t);var n=function l(a){throw "
            "function(l,a){if(!(l instanceof a))throw new TypeError(\"Cannot call a class as a "
            "function\")}(this,l),a};throw "
            "globalThis.assert(null===globalThis.addon.newWithCatch(n,null)),globalThis.assert(\"null\"===globalThis."
            "addon.newWithCatch(n,\"null\")),globalThis.assert(globalThis.addon.newWithCatch(n,t)===t),globalThis."
            "assert(new globalThis.addon.returnWithArgument(void 0)instanceof "
            "globalThis.addon.returnWithArgument),globalThis.assert(new "
            "globalThis.addon.returnWithArgument(null)instanceof "
            "globalThis.addon.returnWithArgument),globalThis.assert(new "
            "globalThis.addon.returnWithArgument(100)instanceof "
            "globalThis.addon.returnWithArgument),globalThis.assert(new "
            "globalThis.addon.returnWithArgument(!0)instanceof "
            "globalThis.addon.returnWithArgument),globalThis.assert(new "
            "globalThis.addon.returnWithArgument(!1)instanceof "
            "globalThis.addon.returnWithArgument),globalThis.assert(new "
            "globalThis.addon.returnWithArgument(\"string\")instanceof "
            "globalThis.addon.returnWithArgument),globalThis.assert(!(new "
            "globalThis.addon.returnWithArgument({})instanceof "
            "globalThis.addon.returnWithArgument)),globalThis.assert(new globalThis.addon.returnWithThis instanceof "
            "globalThis.addon.returnWithThis),globalThis.assert(new globalThis.addon.returnWithCNull instanceof "
            "globalThis.addon.returnWithCNull),null})();",
            "https://www.napi.com/callable.js", nullptr),
        NAPIExceptionPendingException);
    NAPIValue exceptionValue;
    ASSERT_EQ(napi_get_and_clear_last_exception(globalEnv, &exceptionValue), NAPIErrorOK);
    NAPIValueType valueType;
    ASSERT_EQ(napi_typeof(globalEnv, exceptionValue, &valueType), NAPICommonOK);
    ASSERT_EQ(valueType, NAPINull);
}