#include <test.h>

EXTERN_C_START

static NAPIValue runWithCNullThis(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue argv[1];
    assert(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr) == NAPIOK);
    assert(argc == 1);
    NAPIValueType valueType;
    assert(napi_typeof(env, argv[0], &valueType) == NAPIOK);
    assert(valueType == NAPIFunction);
    assert(napi_call_function(env, nullptr, argv[0], 0, nullptr, nullptr) == NAPIOK);

    return nullptr;
}

static NAPIValue run(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue argv[1];
    assert(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr) == NAPIOK);
    assert(argc == 1);
    NAPIValueType valueType;
    assert(napi_typeof(env, argv[0], &valueType) == NAPIOK);
    assert(valueType == NAPIFunction);
    NAPIValue global;
    assert(napi_get_global(env, &global) == NAPIOK);
    assert(napi_call_function(env, global, argv[0], 0, nullptr, nullptr) == NAPIOK);

    return nullptr;
}

static NAPIValue runWithArgument(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 3;
    NAPIValue argv[3];
    assert(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr) == NAPIOK);
    assert(argc == 3);
    NAPIValueType valueType;
    assert(napi_typeof(env, argv[0], &valueType) == NAPIOK);
    assert(valueType == NAPIFunction);
    assert(napi_typeof(env, argv[1], &valueType) == NAPIOK);
    assert(valueType == NAPIString);
    assert(napi_typeof(env, argv[2], &valueType) == NAPIOK);
    assert(valueType == NAPIString);
    NAPIValue args[] = {argv[1], argv[2]};
    assert(napi_call_function(env, nullptr, argv[0], 2, args, nullptr) == NAPIOK);

    return nullptr;
}

static NAPIValue runWithThis(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 2;
    NAPIValue argv[2];
    assert(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr) == NAPIOK);
    assert(argc == 2);
    NAPIValueType valueType;
    assert(napi_typeof(env, argv[0], &valueType) == NAPIOK);
    assert(valueType == NAPIFunction);
    assert(napi_typeof(env, argv[1], &valueType) == NAPIOK);
    assert(valueType == NAPIObject);
    assert(napi_call_function(env, argv[1], argv[0], 0, nullptr, nullptr) == NAPIOK);

    return nullptr;
}

static NAPIValue throwWithArgument(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue argv[1];
    void *data;
    assert(napi_get_cb_info(env, info, &argc, argv, nullptr, &data) == NAPIOK);
    assert(data == env);
    assert(argc == 1);
    assert(napi_throw(env, argv[0]) == NAPIOK);

    return nullptr;
}

static NAPIValue runWithCatch(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue argv[1];
    assert(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr) == NAPIOK);
    assert(argc == 1);
    NAPIValueType valueType;
    assert(napi_typeof(env, argv[0], &valueType) == NAPIOK);
    assert(valueType == NAPIFunction);
    assert(napi_call_function(env, nullptr, argv[0], 0, nullptr, nullptr) == NAPIPendingException);
    NAPIValue output;
    assert(napi_get_and_clear_last_exception(env, &output) == NAPIOK);

    return output;
}

static NAPIValue newWithCatch(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 2;
    NAPIValue argv[2];
    assert(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr) == NAPIOK);
    assert(argc == 2);
    NAPIValueType valueType;
    assert(napi_typeof(env, argv[0], &valueType) == NAPIOK);
    assert(valueType == NAPIFunction);
    NAPIValue thisValue;
    assert(napi_new_instance(env, argv[0], 1, &argv[1], &thisValue) == NAPIPendingException);
    NAPIValue output;
    assert(napi_get_and_clear_last_exception(env, &output) == NAPIOK);

    return output;
}

static NAPIValue returnWithArgument(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue argv[1];
    assert(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr) == NAPIOK);
    assert(argc == 1);

    return argv[0];
}

static NAPIValue returnWithThis(NAPIEnv env, NAPICallbackInfo info)
{
    NAPIValue output;
    assert(napi_get_cb_info(env, info, nullptr, nullptr, &output, nullptr) == NAPIOK);

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
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, runWithCNullThis, nullptr, &runWithUndefinedThisValue), NAPIOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, run, nullptr, &runValue), NAPIOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, runWithArgument, nullptr, &runWithArgumentValue), NAPIOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, runWithThis, nullptr, &runWithThisValue), NAPIOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, throwWithArgument, globalEnv, &throwValue), NAPIOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, runWithCatch, nullptr, &runWithCatchValue), NAPIOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, newWithCatch, nullptr, &newWithCatchValue), NAPIOK);
    NAPIValue ctorValue, returnWithArgumentValue, returnWithThisValue, returnWithCNullValue;
    ASSERT_EQ(NAPIDefineClass(globalEnv, nullptr, throwWithArgument, globalEnv, &ctorValue), NAPIOK);
    ASSERT_EQ(NAPIDefineClass(globalEnv, nullptr, returnWithArgument, globalEnv, &returnWithArgumentValue), NAPIOK);
    ASSERT_EQ(NAPIDefineClass(globalEnv, nullptr, returnWithThis, globalEnv, &returnWithThisValue), NAPIOK);
    ASSERT_EQ(NAPIDefineClass(globalEnv, nullptr, returnWithCNull, globalEnv, &returnWithCNullValue), NAPIOK);
    NAPIValue stringValue;
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "runWithCNullThis", &stringValue), NAPIOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, runWithUndefinedThisValue), NAPIOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "run", &stringValue), NAPIOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, runValue), NAPIOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "runWithArgument", &stringValue), NAPIOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, runWithArgumentValue), NAPIOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "runWithThis", &stringValue), NAPIOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, runWithThisValue), NAPIOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "throwWithArgument", &stringValue), NAPIOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, throwValue), NAPIOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "runWithCatch", &stringValue), NAPIOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, runWithCatchValue), NAPIOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "ctor", &stringValue), NAPIOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, ctorValue), NAPIOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "newWithCatch", &stringValue), NAPIOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, newWithCatchValue), NAPIOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "returnWithArgument", &stringValue), NAPIOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, returnWithArgumentValue), NAPIOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "returnWithThis", &stringValue), NAPIOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, returnWithThisValue), NAPIOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "returnWithCNull", &stringValue), NAPIOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, returnWithCNullValue), NAPIOK);
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
        NAPIPendingException);
    NAPIValue exceptionValue;
    ASSERT_EQ(napi_get_and_clear_last_exception(globalEnv, &exceptionValue), NAPIOK);
    NAPIValueType valueType;
    ASSERT_EQ(napi_typeof(globalEnv, exceptionValue, &valueType), NAPIOK);
    ASSERT_EQ(valueType, NAPINull);
}