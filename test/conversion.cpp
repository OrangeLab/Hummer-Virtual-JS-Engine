#include <test.h>

EXTERN_C_START

static NAPIValue toBool(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue args[1];
    assert(napi_get_cb_info(env, info, &argc, args, nullptr, nullptr) == NAPICommonOK);

    NAPIValue output;
    assert(napi_coerce_to_bool(env, args[0], &output) == NAPIExceptionOK);

    return output;
}

static NAPIValue toNumber(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue args[1];
    assert(napi_get_cb_info(env, info, &argc, args, nullptr, nullptr) == NAPICommonOK);

    NAPIValue output;
    assert(napi_coerce_to_number(env, args[0], &output) == NAPIExceptionOK);

    return output;
}

static NAPIValue toString(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue args[1];
    assert(napi_get_cb_info(env, info, &argc, args, nullptr, nullptr) == NAPICommonOK);

    NAPIValue output;
    assert(napi_coerce_to_string(env, args[0], &output) == NAPIExceptionOK);

    return output;
}

EXTERN_C_END

TEST_F(Test, Conversion)
{
    NAPIValue toBoolValue, toNumberValue, toStringValue;
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, toBool, nullptr, &toBoolValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, toNumber, nullptr, &toNumberValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, toString, nullptr, &toStringValue), NAPIExceptionOK);
    NAPIValue stringValue;
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "toBool", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, toBoolValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "toNumber", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, toNumberValue), NAPIExceptionOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "toString", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, toStringValue), NAPIExceptionOK);
    ASSERT_EQ(
        NAPIRunScript(
            globalEnv,
            "(()=>{\"use "
            "strict\";globalThis.assert(Object.is(globalThis.addon.toBool(!0),!0)),globalThis.assert(Object.is("
            "globalThis.addon.toBool(1),!0)),globalThis.assert(Object.is(globalThis.addon.toBool(-1),!0)),globalThis."
            "assert(Object.is(globalThis.addon.toBool(\"true\"),!0)),globalThis.assert(Object.is(globalThis.addon."
            "toBool(\"false\"),!0)),globalThis.assert(Object.is(globalThis.addon.toBool({}),!0)),globalThis.assert("
            "Object.is(globalThis.addon.toBool([]),!0)),globalThis.assert(Object.is(globalThis.addon.toBool(!1),!1)),"
            "globalThis.assert(Object.is(globalThis.addon.toBool(void "
            "0),!1)),globalThis.assert(Object.is(globalThis.addon.toBool(null),!1)),globalThis.assert(Object.is("
            "globalThis.addon.toBool(0),!1)),globalThis.assert(Object.is(globalThis.addon.toBool(Number.NaN),!1)),"
            "globalThis.assert(Object.is(globalThis.addon.toBool(\"\"),!1)),globalThis.assert(Object.is(globalThis."
            "addon.toNumber(0),0)),globalThis.assert(Object.is(globalThis.addon.toNumber(1),1)),globalThis.assert("
            "Object.is(globalThis.addon.toNumber(1.1),1.1)),globalThis.assert(Object.is(globalThis.addon.toNumber(-1),-"
            "1)),globalThis.assert(Object.is(globalThis.addon.toNumber(\"0\"),0)),globalThis.assert(Object.is("
            "globalThis.addon.toNumber(\"1\"),1)),globalThis.assert(Object.is(globalThis.addon.toNumber(\"1.1\"),1.1)),"
            "globalThis.assert(Object.is(globalThis.addon.toNumber([]),0)),globalThis.assert(Object.is(globalThis."
            "addon.toNumber(!1),0)),globalThis.assert(Object.is(globalThis.addon.toNumber(null),0)),globalThis.assert("
            "Object.is(globalThis.addon.toNumber(\"\"),0)),globalThis.assert(Number.isNaN(globalThis.addon.toNumber("
            "Number.NaN))),globalThis.assert(Number.isNaN(globalThis.addon.toNumber({}))),globalThis.assert(Number."
            "isNaN(globalThis.addon.toNumber(void "
            "0))),globalThis.assert(Object.is(globalThis.addon.toString(\"\"),\"\")),globalThis.assert(Object.is("
            "globalThis.addon.toString(\"globalThis.addon\"),\"globalThis.addon\")),globalThis.assert(Object.is("
            "globalThis.addon.toString(void "
            "0),\"undefined\")),globalThis.assert(Object.is(globalThis.addon.toString(null),\"null\")),globalThis."
            "assert(Object.is(globalThis.addon.toString(!1),\"false\")),globalThis.assert(Object.is(globalThis.addon."
            "toString(!0),\"true\")),globalThis.assert(Object.is(globalThis.addon.toString(0),\"0\")),globalThis."
            "assert(Object.is(globalThis.addon.toString(1.1),\"1.1\")),globalThis.assert(Object.is(globalThis.addon."
            "toString(Number.NaN),\"NaN\")),globalThis.assert(Object.is(globalThis.addon.toString({}),\"[object "
            "Object]\")),globalThis.assert(Object.is(globalThis.addon.toString({toString:function(){return\"globalThis."
            "addon\"}}),\"globalThis.addon\")),globalThis.assert(Object.is(globalThis.addon.toString([]),\"\")),"
            "globalThis.assert(Object.is(globalThis.addon.toString([1,2,3]),\"1,2,3\"))})();",
            "https://www.napi.com/conversion.js", nullptr),
        NAPIExceptionOK);
}