#include <common.h>
#include <js_native_api_test.h>

EXTERN_C_START

static NAPIValue TestGetElement(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 2;
    NAPIValue args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 2, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == NAPIObject, "Wrong type of arguments. Expects an array as first argument.");

    NAPIValueType valuetype1;
    NAPI_CALL(env, napi_typeof(env, args[1], &valuetype1));

    NAPI_ASSERT(env, valuetype1 == NAPINumber, "Wrong type of arguments. Expects an integer as second argument.");

    NAPIValue array = args[0];
    int32_t index;
    NAPI_CALL(env, napi_get_value_int32(env, args[1], &index));

    NAPI_ASSERT(env, index >= 0, "Invalid index. Expects a positive integer.");

    bool isarray;
    NAPI_CALL(env, napi_is_array(env, array, &isarray));

    if (!isarray)
    {
        return getUndefined(env);
    }

    uint32_t length;
    NAPI_CALL(env, napi_get_array_length(env, array, &length));

    NAPI_ASSERT(env, ((uint32_t)index < length), "Index out of bounds!");

    NAPIValue ret;
    NAPI_CALL(env, napi_get_element(env, array, index, &ret));

    return ret;
}

static NAPIValue TestHasElement(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 2;
    NAPIValue args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 2, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == NAPIObject, "Wrong type of arguments. Expects an array as first argument.");

    NAPIValueType valuetype1;
    NAPI_CALL(env, napi_typeof(env, args[1], &valuetype1));

    NAPI_ASSERT(env, valuetype1 == NAPINumber, "Wrong type of arguments. Expects an integer as second argument.");

    NAPIValue array = args[0];
    int32_t index;
    NAPI_CALL(env, napi_get_value_int32(env, args[1], &index));

    bool isarray;
    NAPI_CALL(env, napi_is_array(env, array, &isarray));

    if (!isarray)
    {
        return getUndefined(env);
    }

    bool has_element;
    NAPI_CALL(env, napi_has_element(env, array, index, &has_element));

    NAPIValue ret;
    NAPI_CALL(env, napi_get_boolean(env, has_element, &ret));

    return ret;
}

static NAPIValue TestDeleteElement(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 2;
    NAPIValue args[2];

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));
    NAPI_ASSERT(env, argc == 2, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));
    NAPI_ASSERT(env, valuetype0 == NAPIObject, "Wrong type of arguments. Expects an array as first argument.");

    NAPIValueType valuetype1;
    NAPI_CALL(env, napi_typeof(env, args[1], &valuetype1));
    NAPI_ASSERT(env, valuetype1 == NAPINumber, "Wrong type of arguments. Expects an integer as second argument.");

    NAPIValue array = args[0];
    int32_t index;
    bool result;
    NAPIValue ret;

    NAPI_CALL(env, napi_get_value_int32(env, args[1], &index));
    NAPI_CALL(env, napi_is_array(env, array, &result));

    if (!result)
    {
        return getUndefined(env);
    }

    NAPI_CALL(env, napi_delete_element(env, array, index, &result));
    NAPI_CALL(env, napi_get_boolean(env, result, &ret));

    return ret;
}

static NAPIValue New(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == NAPIObject, "Wrong type of arguments. Expects an array as first argument.");

    NAPIValue ret;
    NAPI_CALL(env, napi_create_array(env, &ret));

    uint32_t i, length;
    NAPI_CALL(env, napi_get_array_length(env, args[0], &length));

    for (i = 0; i < length; i++)
    {
        NAPIValue e;
        NAPI_CALL(env, napi_get_element(env, args[0], i, &e));
        NAPI_CALL(env, napi_set_element(env, ret, i, e));
    }

    return ret;
}

static NAPIValue NewWithLength(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == NAPINumber, "Wrong type of arguments. Expects an integer the first argument.");

    // V8 可以正确处理 2^32 - 1，但是 JavaScriptCore 无法正确处理，只能换成
    // uint_32
    uint32_t array_length;
    NAPI_CALL(env, napi_get_value_uint32(env, args[0], &array_length));

    NAPIValue ret;
    NAPI_CALL(env, napi_create_array_with_length(env, array_length, &ret));

    return ret;
}

EXTERN_C_END

TEST_F(Test, TestArray)
{
    NAPIPropertyDescriptor descriptors[] = {
        DECLARE_NAPI_PROPERTY("TestGetElement", TestGetElement),
        DECLARE_NAPI_PROPERTY("TestHasElement", TestHasElement),
        DECLARE_NAPI_PROPERTY("TestDeleteElement", TestDeleteElement),
        DECLARE_NAPI_PROPERTY("New", New),
        DECLARE_NAPI_PROPERTY("NewWithLength", NewWithLength),
    };
    ASSERT_EQ(napi_define_properties(globalEnv, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors),
              NAPIOK);

    NAPIValue result;
    ASSERT_EQ(
        NAPIRunScriptWithSourceUrl(
            globalEnv,
            "(()=>{\"use strict\";var "
            "s=[1,9,48,13493,9459324,{name:\"hello\"},[\"world\",\"node\",\"abi\"]];globalThis.assert.throws((function("
            "){globalThis.addon.TestGetElement(s,s.length+1)})),globalThis.assert.throws((function(){globalThis.addon."
            "TestGetElement(s,-2)})),s.forEach((function(a,l){globalThis.assert.strictEqual(globalThis.addon."
            "TestGetElement(s,l),a)})),globalThis.assert(globalThis.addon.TestHasElement(s,0)),globalThis.assert."
            "strictEqual(globalThis.addon.TestHasElement(s,s.length+1),!1),globalThis.assert(globalThis.addon."
            "NewWithLength(0)instanceof Array),globalThis.assert(globalThis.addon.NewWithLength(1)instanceof "
            "Array),globalThis.assert(globalThis.addon.NewWithLength(4294967295)instanceof Array);var "
            "a=[\"a\",\"b\",\"c\",\"d\"];globalThis.assert.strictEqual(a.length,4),globalThis.assert.strictEqual(2 in "
            "a,!0),globalThis.assert.strictEqual(globalThis.addon.TestDeleteElement(a,2),!0),globalThis.assert."
            "strictEqual(a.length,4),globalThis.assert.strictEqual(2 in a,!1)})();",
            "https://www.didi.com/test_array.js", &result),
        NAPIOK);
}
