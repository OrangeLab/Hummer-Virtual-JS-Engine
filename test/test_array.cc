#include <gtest/gtest.h>
#include <napi/js_native_api.h>
#include <napi_assert.h>

EXTERN_C_START

static NAPIValue TestGetElement(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 2;
    NAPIValue args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc >= 2, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == NAPIObject,
                "Wrong type of arguments. Expects an array as first argument.");

    NAPIValueType valuetype1;
    NAPI_CALL(env, napi_typeof(env, args[1], &valuetype1));

    NAPI_ASSERT(env, valuetype1 == NAPINumber,
                "Wrong type of arguments. Expects an integer as second argument.");

    NAPIValue array = args[0];
    int32_t index;
    NAPI_CALL(env, napi_get_value_int32(env, args[1], &index));

    NAPI_ASSERT(env, index >= 0, "Invalid index. Expects a positive integer.");

    bool isarray;
    NAPI_CALL(env, napi_is_array(env, array, &isarray));

    if (!isarray) {
        return nullptr;
    }

    uint32_t length;
    NAPI_CALL(env, napi_get_array_length(env, array, &length));

    NAPI_ASSERT(env, ((uint32_t) index < length), "Index out of bounds!");

    NAPIValue ret;
    NAPI_CALL(env, napi_get_element(env, array, index, &ret));

    return ret;
}

static NAPIValue TestHasElement(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 2;
    NAPIValue args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc >= 2, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == NAPIObject,
                "Wrong type of arguments. Expects an array as first argument.");

    NAPIValueType valuetype1;
    NAPI_CALL(env, napi_typeof(env, args[1], &valuetype1));

    NAPI_ASSERT(env, valuetype1 == NAPINumber,
                "Wrong type of arguments. Expects an integer as second argument.");

    NAPIValue array = args[0];
    int32_t index;
    NAPI_CALL(env, napi_get_value_int32(env, args[1], &index));

    bool isarray;
    NAPI_CALL(env, napi_is_array(env, array, &isarray));

    if (!isarray) {
        return nullptr;
    }

    bool has_element;
    NAPI_CALL(env, napi_has_element(env, array, index, &has_element));

    NAPIValue ret;
    NAPI_CALL(env, napi_get_boolean(env, has_element, &ret));

    return ret;
}

static NAPIValue TestDeleteElement(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 2;
    NAPIValue args[2];

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    NAPI_ASSERT(env, argc == 2, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));
    NAPI_ASSERT(env, valuetype0 == NAPIObject,
                "Wrong type of arguments. Expects an array as first argument.");

    NAPIValueType valuetype1;
    NAPI_CALL(env, napi_typeof(env, args[1], &valuetype1));
    NAPI_ASSERT(env, valuetype1 == NAPINumber,
                "Wrong type of arguments. Expects an integer as second argument.");

    NAPIValue array = args[0];
    int32_t index;
    bool result;
    NAPIValue ret;

    NAPI_CALL(env, napi_get_value_int32(env, args[1], &index));
    NAPI_CALL(env, napi_is_array(env, array, &result));

    if (!result) {
        return nullptr;
    }

    NAPI_CALL(env, napi_delete_element(env, array, index, &result));
    NAPI_CALL(env, napi_get_boolean(env, result, &ret));

    return ret;
}

static NAPIValue New(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == NAPIObject,
                "Wrong type of arguments. Expects an array as first argument.");

    NAPIValue ret;
    NAPI_CALL(env, napi_create_array(env, &ret));

    uint32_t i, length;
    NAPI_CALL(env, napi_get_array_length(env, args[0], &length));

    for (i = 0; i < length; i++) {
        NAPIValue e;
        NAPI_CALL(env, napi_get_element(env, args[0], i, &e));
        NAPI_CALL(env, napi_set_element(env, ret, i, e));
    }

    return ret;
}

static NAPIValue NewWithLength(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == NAPINumber,
                "Wrong type of arguments. Expects an integer the first argument.");

    // V8 可以正确处理 2^32 - 1，但是 JavaScriptCore 无法正确处理，只能换成 uint_32
    uint32_t array_length;
    NAPI_CALL(env, napi_get_value_uint32(env, args[0], &array_length));

    NAPIValue ret;
    NAPI_CALL(env, napi_create_array_with_length(env, array_length, &ret));

    return ret;
}

EXTERN_C_END

TEST(TestArray, NewLengthGetHasDelete) {
    NAPIEnv env = nullptr;
    ASSERT_EQ(NAPICreateEnv(&env), NAPIOK);

    NAPIValue global = nullptr;
    ASSERT_EQ(napi_get_global(env, &global), NAPIOK);

    NAPIPropertyDescriptor descriptors[] = {
            DECLARE_NAPI_PROPERTY("TestGetElement", TestGetElement),
            DECLARE_NAPI_PROPERTY("TestHasElement", TestHasElement),
            DECLARE_NAPI_PROPERTY("TestDeleteElement", TestDeleteElement),
            DECLARE_NAPI_PROPERTY("New", New),
            DECLARE_NAPI_PROPERTY("NewWithLength", NewWithLength),
    };

    ASSERT_EQ(napi_define_properties(
            env, global, sizeof(descriptors) / sizeof(*descriptors), descriptors), NAPIOK);

    EXPECT_EQ(initAssert(env, global), NAPIOK);

    NAPIValue result = nullptr;
    // assert.deepStrictEqual(globalThis.New(array), array); 未实现
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(env, "(function () {\n"
                                              "    const array = [\n"
                                              "        1,\n"
                                              "        9,\n"
                                              "        48,\n"
                                              "        13493,\n"
                                              "        9459324,\n"
                                              "        { name: 'hello' },\n"
                                              "        [\n"
                                              "            'world',\n"
                                              "            'node',\n"
                                              "            'abi'\n"
                                              "        ]\n"
                                              "    ];\n"
                                              "\n"
                                              "    assert.throws(\n"
                                              "        () => {\n"
                                              "            globalThis.TestGetElement(array, array.length + 1);\n"
                                              "        }\n"
                                              "    );\n"
                                              "\n"
                                              "    assert.throws(\n"
                                              "        () => {\n"
                                              "            globalThis.TestGetElement(array, -2);\n"
                                              "        },\n"
                                              "    );\n"
                                              "\n"
                                              "    array.forEach(function (element, index) {\n"
                                              "        assert.strictEqual(globalThis.TestGetElement(array, index), element);\n"
                                              "    });\n"
                                              "\n"
                                              "    assert(globalThis.TestHasElement(array, 0));\n"
                                              "    assert.strictEqual(globalThis.TestHasElement(array, array.length + 1), false);\n"
                                              "\n"
                                              "    assert(globalThis.NewWithLength(0) instanceof Array);\n"
                                              "    assert(globalThis.NewWithLength(1) instanceof Array);\n"
                                              "    // Check max allowed length for an array 2^32 -1\n"
                                              "    assert(globalThis.NewWithLength(4294967295) instanceof Array);\n"
                                              "\n"
                                              "    {\n"
                                              "        // Verify that array elements can be deleted.\n"
                                              "        const arr = ['a', 'b', 'c', 'd'];\n"
                                              "\n"
                                              "        assert.strictEqual(arr.length, 4);\n"
                                              "        assert.strictEqual(2 in arr, true);\n"
                                              "        assert.strictEqual(globalThis.TestDeleteElement(arr, 2), true);\n"
                                              "        assert.strictEqual(arr.length, 4);\n"
                                              "        assert.strictEqual(2 in arr, false);\n"
                                              "    }\n"
                                              "})();",
                                         "https://n-api.com/test_array.js",
                                         &result), NAPIOK);
    NAPIValueType valueType;
    EXPECT_EQ(napi_typeof(env, result, &valueType), NAPIOK);
    EXPECT_EQ(valueType, NAPIUndefined);

    ASSERT_EQ(NAPIFreeEnv(env), NAPIOK);
}