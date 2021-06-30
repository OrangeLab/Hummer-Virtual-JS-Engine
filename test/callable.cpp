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
    assert(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr) == NAPIOK);
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
    assert(napi_typeof(env, output, &valueType) == NAPIOK);

    return output;
}

// static NAPIValue newWithCatch(NAPIEnv env, NAPICallbackInfo info)
//{
//     size_t argc = 2;
//     NAPIValue argv[2];
//     assert(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr) == NAPIOK);
//     assert(argc == 2);
//     NAPIValueType valueType;
//     assert(napi_typeof(env, argv[0], &valueType) == NAPIOK);
//     assert(valueType == NAPIFunction);
//     NAPIValue thisValue;
//     assert(napi_new_instance(env, argv[0], 1, &argv[1], &thisValue) == NAPIOK);
//     NAPIValue output;
//     assert(napi_get_and_clear_last_exception(env, &output) == NAPIOK);
//     assert(napi_typeof(env, output, &valueType) == NAPIOK);
//
//     return output;
// }

EXTERN_C_END

TEST_F(Test, Callable)
{
    NAPIValue runWithUndefinedThisValue, runValue, runWithArgumentValue, runWithThisValue, throwValue,
        runWithCatchValue;
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, -1, runWithCNullThis, nullptr, &runWithUndefinedThisValue),
              NAPIOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, -1, run, nullptr, &runValue), NAPIOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, -1, runWithArgument, nullptr, &runWithArgumentValue), NAPIOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, -1, runWithThis, nullptr, &runWithThisValue), NAPIOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, -1, throwWithArgument, nullptr, &throwValue), NAPIOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, -1, runWithCatch, nullptr, &runWithCatchValue), NAPIOK);
    NAPIValue stringValue;
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "runWithCNullThis", -1, &stringValue), NAPIOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, runWithUndefinedThisValue), NAPIOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "run", -1, &stringValue), NAPIOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, runValue), NAPIOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "runWithArgument", -1, &stringValue), NAPIOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, runWithArgumentValue), NAPIOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "runWithThis", -1, &stringValue), NAPIOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, runWithThisValue), NAPIOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "throwWithArgument", -1, &stringValue), NAPIOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, throwValue), NAPIOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "runWithCatch", -1, &stringValue), NAPIOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, runWithCatchValue), NAPIOK);
    ASSERT_EQ(
        NAPIRunScript(
            globalEnv,
            "(()=>{\"use strict\";var "
            "l=!1;globalThis.addon.runWithCNullThis((function(){l=!0,globalThis.assert(this===globalThis)})),"
            "globalThis.assert(l),l=!1,globalThis.addon.run((function(){l=!0,globalThis.assert(this===globalThis)})),"
            "globalThis.assert(l);var "
            "s={};globalThis.addon.runWithThis((function(){globalThis.assert(this===s)}),s),globalThis.addon."
            "runWithArgument((function(){globalThis.assert(2===arguments.length),globalThis.assert(\"hello\"===("
            "arguments.length<=0?void 0:arguments[0])),globalThis.assert(\"world\"===(arguments.length<=1?void "
            "0:arguments[1]))}),\"hello\",\"world\");try{globalThis.addon.throwWithArgument(null)}catch(l){globalThis."
            "assert(null===l)}try{globalThis.addon.throwWithArgument(\"Error\")}catch(l){globalThis.assert(\"Error\"==="
            "l)}var a={};try{globalThis.addon.throwWithArgument(a)}catch(l){globalThis.assert(l===a)}throw "
            "globalThis.assert(null===globalThis.addon.runWithCatch((function(){throw "
            "null}))),globalThis.assert(\"null\"===globalThis.addon.runWithCatch((function(){throw\"null\"}))),"
            "globalThis.assert(globalThis.addon.runWithCatch((function(){throw a}))===a),null})();",
            "https://www.napi.com/callable.js", nullptr),
        NAPIPendingException);
    NAPIValue exceptionValue;
    ASSERT_EQ(napi_get_and_clear_last_exception(globalEnv, &exceptionValue), NAPIOK);
    NAPIValueType valueType;
    ASSERT_EQ(napi_typeof(globalEnv, exceptionValue, &valueType), NAPIOK);
    ASSERT_EQ(valueType, NAPINull);
}