#include <test.h>

EXTERN_C_START

static NAPIValue get(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 2;
    NAPIValue args[2];
    assert(napi_get_cb_info(env, info, &argc, args, nullptr, nullptr) == NAPIOK);

    assert(argc >= 2);

    NAPIValueType valueType;
    assert(napi_typeof(env, args[0], &valueType) == NAPIOK);

    assert(valueType == NAPIObject);

    assert(napi_typeof(env, args[1], &valueType) == NAPIOK);

    assert(valueType == NAPIString || valueType == NAPINumber);

    NAPIValue object = args[0];
    NAPIValue output;
    assert(napi_get_property(env, object, args[1], &output) == NAPIOK);

    return output;
}

static NAPIValue set(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 3;
    NAPIValue args[3];
    assert(napi_get_cb_info(env, info, &argc, args, nullptr, nullptr) == NAPIOK);

    assert(argc >= 3);

    NAPIValueType valueType;
    assert(napi_typeof(env, args[0], &valueType) == NAPIOK);

    assert(valueType == NAPIObject);

    assert(napi_typeof(env, args[1], &valueType) == NAPIOK);

    assert(valueType == NAPIString || valueType == NAPINumber);

    assert(napi_set_property(env, args[0], args[1], args[2]) == NAPIOK);

    return nullptr;
}

static NAPIValue has(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 2;
    NAPIValue args[2];
    assert(napi_get_cb_info(env, info, &argc, args, nullptr, nullptr) == NAPIOK);

    assert(argc >= 2);

    NAPIValueType valueType;
    assert(napi_typeof(env, args[0], &valueType) == NAPIOK);

    assert(valueType == NAPIObject);

    assert(napi_typeof(env, args[1], &valueType) == NAPIOK);

    assert(valueType == NAPIString || valueType == NAPINumber);

    bool hasProperty;
    assert(napi_has_property(env, args[0], args[1], &hasProperty) == NAPIOK);

    NAPIValue ret;
    assert(napi_get_boolean(env, hasProperty, &ret) == NAPIOK);

    return ret;
}

static NAPIValue deleteFunction(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 2;
    NAPIValue args[2];

    assert(napi_get_cb_info(env, info, &argc, args, nullptr, nullptr) == NAPIOK);
    assert(argc >= 2);

    NAPIValueType valueType;
    assert(napi_typeof(env, args[0], &valueType) == NAPIOK);

    assert(valueType == NAPIObject);

    assert(napi_typeof(env, args[1], &valueType) == NAPIOK);

    assert(valueType == NAPIString || valueType == NAPINumber);

    bool result;
    NAPIValue ret;
    assert(napi_delete_property(env, args[0], args[1], &result) == NAPIOK);
    assert(napi_get_boolean(env, result, &ret) == NAPIOK);

    return ret;
}

static NAPIValue isArray(NAPIEnv env, NAPICallbackInfo callbackInfo)
{
    size_t argc = 1;
    NAPIValue argv[1];
    assert(napi_get_cb_info(env, callbackInfo, &argc, argv, nullptr, nullptr) == NAPIOK);
    bool result;
    assert(napi_is_array(env, argv[0], &result) == NAPIOK);
    NAPIValue output;
    assert(napi_get_boolean(env, result, &output) == NAPIOK);

    return output;
}

EXTERN_C_END

TEST_F(Test, Object)
{
    NAPIValue getValue, setValue, hasValue, deleteValue, isArrayValue;
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, -1, get, globalEnv, &getValue), NAPIOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, -1, set, globalEnv, &setValue), NAPIOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, -1, has, globalEnv, &hasValue), NAPIOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, -1, deleteFunction, globalEnv, &deleteValue), NAPIOK);
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, -1, isArray, globalEnv, &isArrayValue), NAPIOK);
    NAPIValue stringValue;
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "get", -1, &stringValue), NAPIOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, getValue), NAPIOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "set", -1, &stringValue), NAPIOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, setValue), NAPIOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "has", -1, &stringValue), NAPIOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, hasValue), NAPIOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "delete", -1, &stringValue), NAPIOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, deleteValue), NAPIOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "isArray", -1, &stringValue), NAPIOK);
    ASSERT_EQ(napi_set_property(globalEnv, addonValue, stringValue, isArrayValue), NAPIOK);
    ASSERT_EQ(
        NAPIRunScript(globalEnv,
                      "(()=>{\"use strict\";globalThis.assert(globalThis.addon.isArray([]));var "
                      "l={hello:\"world\",0:\"hello\"};globalThis.assert(\"world\"===globalThis.addon.get(l,\"hello\"))"
                      ",globalThis.assert(\"hello\"===globalThis.addon.get(l,0)),globalThis.assert(globalThis.addon."
                      "has(l,\"hello\")),globalThis.assert(globalThis.addon.has(l,0)),globalThis.assert(globalThis."
                      "addon.delete(l,\"hello\")),globalThis.assert(globalThis.addon.delete(l,0)),globalThis.assert(!"
                      "globalThis.addon.has(l,\"hello\")),globalThis.assert(!globalThis.addon.has(l,0));var "
                      "s=function(){this.foo=42,this.bar=43,this[0]=1,this[1]=2};s.prototype.bar=44,s.prototype[1]=3,s."
                      "prototype.baz=45,s.prototype[2]=4;var a=new "
                      "s;globalThis.assert(42===globalThis.addon.get(a,\"foo\")),globalThis.assert(43===globalThis."
                      "addon.get(a,\"bar\")),globalThis.assert(45===globalThis.addon.get(a,\"baz\")),globalThis.assert("
                      "globalThis.addon.get(a,\"toString\")===Object.prototype.toString),globalThis.assert(globalThis."
                      "addon.has(a,\"toString\")),globalThis.assert(1===globalThis.addon.get(a,0)),globalThis.assert(2="
                      "==globalThis.addon.get(a,1)),globalThis.assert(4===globalThis.addon.get(a,2)),globalThis.assert("
                      "globalThis.addon.delete(a,2)),globalThis.assert(globalThis.addon.delete(a,\"baz\")),globalThis."
                      "assert(globalThis.addon.has(a,2)),globalThis.assert(globalThis.addon.has(a,\"baz\"));var "
                      "o=function(l){globalThis.assert(!!l),globalThis.assert(null==l?void "
                      "0:l.configurable),globalThis.assert(null==l?void 0:l.enumerable),globalThis.assert(null==l?void "
                      "0:l.writable),globalThis.assert(null==l?void "
                      "0:l.value),globalThis.assert(!(null!=l&&l.get)),globalThis.assert(!(null!=l&&l.set))},e={};"
                      "globalThis.addon.set(e,\"world\",!0),globalThis.addon.set(e,0,!0),o(Object."
                      "getOwnPropertyDescriptor(e,\"world\")),o(Object.getOwnPropertyDescriptor(e,0))})();",
                      "https://www.napi.com/object.js", nullptr),
        NAPIOK);
}