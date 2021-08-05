#include <test.h>

EXTERN_C_START

static NAPIValue doInstanceOf(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 2;
    NAPIValue args[2];
    assert(napi_get_cb_info(env, info, &argc, args, nullptr, nullptr) == NAPICommonOK);

    bool instanceof ;
    assert(napi_instanceof(env, args[0], args[1], & instanceof) == NAPIExceptionOK);

    NAPIValue result;
    assert(napi_get_boolean(env, instanceof, &result) == NAPIErrorOK);

    return result;
}

static NAPIValue getUndefined(NAPIEnv env, NAPICallbackInfo callbackInfo)
{
    NAPIValue result;
    assert(napi_get_undefined(env, &result) == NAPICommonOK);
    void *data;
    assert(napi_get_cb_info(env, callbackInfo, nullptr, nullptr, nullptr, &data) == NAPICommonOK);
    assert(data == env);

    return result;
}

static NAPIValue getNull(NAPIEnv env, NAPICallbackInfo callbackInfo)
{
    NAPIValue result;
    assert(napi_get_null(env, &result) == NAPICommonOK);
    void *data;
    assert(napi_get_cb_info(env, callbackInfo, nullptr, nullptr, nullptr, &data) == NAPICommonOK);
    assert(!data);

    return result;
}

static NAPIValue getGlobal(NAPIEnv env, NAPICallbackInfo /*callbackInfo*/)
{
    NAPIValue result;
    assert(napi_get_global(env, &result) == NAPIErrorOK);

    return result;
}

static NAPIValue testNumber(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue args[1];
    assert(napi_get_cb_info(env, info, &argc, args, nullptr, nullptr) == NAPICommonOK);

    assert(argc >= 1);

    NAPIValueType valueType;
    assert(napi_typeof(env, args[0], &valueType) == NAPICommonOK);

    assert(valueType == NAPINumber);

    double input;
    assert(napi_get_value_double(env, args[0], &input) == NAPIErrorOK);

    NAPIValue output;
    assert(napi_create_double(env, input, &output) == NAPIErrorOK);

    return output;
}

EXTERN_C_END

TEST_F(Test, General)
{
    NAPIValue getUndefinedValue, getNullValue, getGlobalValue, testNumberValue, doInstanceOfValue;
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, getUndefined, globalEnv, &getUndefinedValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_function(globalEnv, "", getNull, nullptr, &getNullValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_function(globalEnv, "getGlobal", getGlobal, nullptr, &getGlobalValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_function(globalEnv, "测试数字", testNumber, nullptr, &testNumberValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, doInstanceOf, nullptr, &doInstanceOfValue), NAPIExceptionOK);
    NAPIValue stringValue;
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "getUndefined", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, getUndefinedValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "getNull", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, getNullValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "getGlobal", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, getGlobalValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "testNumber", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, testNumberValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "doInstanceOf", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, doInstanceOfValue), NAPIExceptionOK);

    NAPIValue trueValue, falseValue;
    ASSERT_EQ(napi_get_boolean(globalEnv, true, &trueValue), NAPIErrorOK);
    ASSERT_EQ(napi_get_boolean(globalEnv, false, &falseValue), NAPIErrorOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "true", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, trueValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "false", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, falseValue), NAPIExceptionOK);

    NAPIValue nullCStringValue, emptyStringValue, asciiStringValue, utf8StringValue;
    ASSERT_EQ(napi_create_string_utf8(globalEnv, nullptr, &nullCStringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "", &emptyStringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "Hello, world!", &asciiStringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "测试", &utf8StringValue), NAPIExceptionOK);

    ASSERT_EQ(napi_create_string_utf8(globalEnv, "nullCString", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, nullCStringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "emptyString", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, emptyStringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "asciiString", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, asciiStringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "utf8String", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, utf8StringValue), NAPIExceptionOK);

    ASSERT_EQ(
        NAPIRunScript(
            globalEnv,
            "(()=>{\"use strict\";function "
            "l(l){globalThis.assert(Object.is(l,globalThis.addon.testNumber(l)))}globalThis.assert(void "
            "0===globalThis.addon.getUndefined()),globalThis.assert(null===globalThis.addon.getNull()),globalThis."
            "assert(globalThis.addon.getGlobal()===globalThis),globalThis.assert(\"boolean\"==typeof "
            "globalThis.addon.true&&globalThis.addon.true),globalThis.assert(\"boolean\"==typeof "
            "globalThis.addon.false&&!globalThis.addon.false),l(0),l(-0),l(1),l(-1),l(100),l(2121),l(-1233),l(986583),"
            "l(-976675),l(987654322134568e50),l(-4350987086545761e48),l(Number.MIN_SAFE_INTEGER),l(Number.MAX_SAFE_"
            "INTEGER),l(Number.MAX_SAFE_INTEGER+10),l(Number.MIN_VALUE),l(Number.MAX_VALUE),l(Number.MAX_VALUE+10),l("
            "Number.POSITIVE_INFINITY),l(Number.NEGATIVE_INFINITY),l(Number.NaN),globalThis.assert(\"\"===globalThis."
            "addon.nullCString),globalThis.assert(\"\"===globalThis.addon.emptyString),globalThis.assert(\"Hello, "
            "world!\"===globalThis.addon.asciiString),globalThis.assert(\"测试\"===globalThis.addon.utf8String),"
            "globalThis.assert(\"\"===globalThis.addon.getUndefined.name),globalThis.assert(\"\"===globalThis.addon."
            "getNull.name),globalThis.assert(\"getGlobal\"===globalThis.addon.getGlobal.name),globalThis.assert("
            "\"测试数字\"===globalThis.addon.testNumber.name),globalThis.assert(globalThis.addon.doInstanceOf({},"
            "Object)),globalThis.assert(globalThis.addon.doInstanceOf([],Object)),globalThis.assert(!globalThis.addon."
            "doInstanceOf({},Array)),globalThis.assert(globalThis.addon.doInstanceOf([],Array))})();",
            "https://www.napi.com/general.js", nullptr),
        NAPIExceptionOK);
}