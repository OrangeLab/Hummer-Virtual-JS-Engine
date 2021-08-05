#include <test.h>

EXTERN_C_START

static NAPIValue typeofUndefined(NAPIEnv env, NAPICallbackInfo callbackInfo)
{
    size_t argc = 1;
    NAPIValue argv[1];
    assert(napi_get_cb_info(env, callbackInfo, &argc, argv, nullptr, nullptr) == NAPICommonOK);
    NAPIValueType valueType;
    assert(napi_typeof(env, argv[0], &valueType) == NAPICommonOK);
    assert(valueType == NAPIUndefined);

    return nullptr;
}

static NAPIValue typeofNull(NAPIEnv env, NAPICallbackInfo callbackInfo)
{
    size_t argc = 1;
    NAPIValue argv[1];
    assert(napi_get_cb_info(env, callbackInfo, &argc, argv, nullptr, nullptr) == NAPICommonOK);
    NAPIValueType valueType;
    assert(napi_typeof(env, argv[0], &valueType) == NAPICommonOK);
    assert(valueType == NAPINull);

    return nullptr;
}

static NAPIValue typeofBoolean(NAPIEnv env, NAPICallbackInfo callbackInfo)
{
    size_t argc = 1;
    NAPIValue argv[1];
    assert(napi_get_cb_info(env, callbackInfo, &argc, argv, nullptr, nullptr) == NAPICommonOK);
    NAPIValueType valueType;
    assert(napi_typeof(env, argv[0], &valueType) == NAPICommonOK);
    assert(valueType == NAPIBoolean);

    return nullptr;
}

static NAPIValue typeofNumber(NAPIEnv env, NAPICallbackInfo callbackInfo)
{
    size_t argc = 1;
    NAPIValue argv[1];
    assert(napi_get_cb_info(env, callbackInfo, &argc, argv, nullptr, nullptr) == NAPICommonOK);
    NAPIValueType valueType;
    assert(napi_typeof(env, argv[0], &valueType) == NAPICommonOK);
    assert(valueType == NAPINumber);

    return nullptr;
}

static NAPIValue typeofString(NAPIEnv env, NAPICallbackInfo callbackInfo)
{
    size_t argc = 1;
    NAPIValue argv[1];
    assert(napi_get_cb_info(env, callbackInfo, &argc, argv, nullptr, nullptr) == NAPICommonOK);
    NAPIValueType valueType;
    assert(napi_typeof(env, argv[0], &valueType) == NAPICommonOK);
    assert(valueType == NAPIString);

    return nullptr;
}

static NAPIValue typeofObject(NAPIEnv env, NAPICallbackInfo callbackInfo)
{
    size_t argc = 1;
    NAPIValue argv[1];
    assert(napi_get_cb_info(env, callbackInfo, &argc, argv, nullptr, nullptr) == NAPICommonOK);
    NAPIValueType valueType;
    assert(napi_typeof(env, argv[0], &valueType) == NAPICommonOK);
    assert(valueType == NAPIObject);

    return nullptr;
}

EXTERN_C_END

TEST_F(Test, Typeof)
{
    NAPIValue undefined, null, boolean, number, string, objectValue;
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, typeofUndefined, nullptr, &undefined), NAPIExceptionOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, typeofNull, nullptr, &null), NAPIExceptionOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, typeofBoolean, nullptr, &boolean), NAPIExceptionOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, typeofNumber, nullptr, &number), NAPIExceptionOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, typeofString, nullptr, &string), NAPIExceptionOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, typeofObject, nullptr, &objectValue), NAPIExceptionOK);

    NAPIValue stringValue;
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "typeofUndefined", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, undefined), NAPIExceptionOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "typeofNull", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, null), NAPIExceptionOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "typeofBoolean", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, boolean), NAPIExceptionOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "typeofNumber", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, number), NAPIExceptionOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "typeofString", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, string), NAPIExceptionOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "typeofObject", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, objectValue), NAPIExceptionOK);

    ASSERT_EQ(
        NAPIRunScript(
            globalEnv,
            "(()=>{\"use strict\";globalThis.addon.typeofUndefined(void "
            "0),globalThis.addon.typeofNull(null),globalThis.addon.typeofBoolean(!0),globalThis.addon.typeofBoolean(!1)"
            ",globalThis.addon.typeofNumber(0),globalThis.addon.typeofNumber(-0),globalThis.addon.typeofNumber(1),"
            "globalThis.addon.typeofNumber(-1),globalThis.addon.typeofNumber(100),globalThis.addon.typeofNumber(2121),"
            "globalThis.addon.typeofNumber(-1233),globalThis.addon.typeofNumber(986583),globalThis.addon.typeofNumber(-"
            "976675),globalThis.addon.typeofNumber(987654322134568e50),globalThis.addon.typeofNumber(-"
            "4350987086545761e48),globalThis.addon.typeofNumber(Number.MIN_SAFE_INTEGER),globalThis.addon.typeofNumber("
            "Number.MAX_SAFE_INTEGER),globalThis.addon.typeofNumber(Number.MAX_SAFE_INTEGER+10),globalThis.addon."
            "typeofNumber(Number.MIN_VALUE),globalThis.addon.typeofNumber(Number.MAX_VALUE),globalThis.addon."
            "typeofNumber(Number.MAX_VALUE+10),globalThis.addon.typeofNumber(Number.POSITIVE_INFINITY),globalThis."
            "addon.typeofNumber(Number.NEGATIVE_INFINITY),globalThis.addon.typeofNumber(Number.NaN),globalThis.addon."
            "typeofString(\"\"),globalThis.addon.typeofString(\"Hello, "
            "world!\"),globalThis.addon.typeofString(\"测试\"),globalThis.addon.typeofObject({}),globalThis.addon."
            "typeofObject([]),globalThis.addon.typeofObject(new Number(100)),globalThis.addon.typeofObject(new "
            "String(\"String\")),globalThis.addon.typeofObject(/s/)})();",
            "https://www.napi.com/typeof.js", nullptr),
        NAPIExceptionOK);
}