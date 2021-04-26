#include <gtest/gtest.h>
#include <napi/js_native_api.h>
#include <napi_assert.h>

EXTERN_C_START

static int test_value = 3;

static NAPIValue Get(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 2;
    NAPIValue args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc >= 2, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == NAPIObject,
                "Wrong type of arguments. Expects an object as first argument.");

    NAPIValueType valuetype1;
    NAPI_CALL(env, napi_typeof(env, args[1], &valuetype1));

    NAPI_ASSERT(env, valuetype1 == NAPIString || valuetype1 == NAPISymbol,
                "Wrong type of arguments. Expects a string or symbol as second.");

    NAPIValue object = args[0];
    NAPIValue output;
    NAPI_CALL(env, napi_get_property(env, object, args[1], &output));

    return output;
}

static NAPIValue GetNamed(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 2;
    NAPIValue args[2];
    char key[256] = "";
    size_t key_length;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc >= 2, "Wrong number of arguments");

    NAPIValueType value_type0;
    NAPI_CALL(env, napi_typeof(env, args[0], &value_type0));

    NAPI_ASSERT(env, value_type0 == NAPIObject,
                "Wrong type of arguments. Expects an object as first argument.");

    NAPIValueType value_type1;
    NAPI_CALL(env, napi_typeof(env, args[1], &value_type1));

    NAPI_ASSERT(env, value_type1 == NAPIString,
                "Wrong type of arguments. Expects a string as second.");

    NAPIValue object = args[0];
    NAPI_CALL(env,
              napi_get_value_string_utf8(env, args[1], key, 255, &key_length));
    key[255] = 0;
    NAPI_ASSERT(env, key_length <= 255,
                "Cannot accommodate keys longer than 255 bytes");
    NAPIValue output;
    NAPI_CALL(env, napi_get_named_property(env, object, key, &output));

    return output;
}

static NAPIValue GetPropertyNames(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValueType value_type0;
    NAPI_CALL(env, napi_typeof(env, args[0], &value_type0));

    NAPI_ASSERT(env, value_type0 == NAPIObject,
                "Wrong type of arguments. Expects an object as first argument.");

    NAPIValue output;
    NAPI_CALL(env, napi_get_property_names(env, args[0], &output));

    return output;
}

//static NAPIValue GetSymbolNames(NAPIEnv env, NAPICallbackInfo info) {
//    size_t argc = 1;
//    NAPIValue args[1];
//    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
//
//    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");
//
//    NAPIValueType value_type0;
//    NAPI_CALL(env, napi_typeof(env, args[0], &value_type0));
//
//    NAPI_ASSERT(env,
//                value_type0 == NAPIObject,
//                "Wrong type of arguments. Expects an object as first argument.");
//
//    NAPIValue output;
//    NAPI_CALL(env,
//              napi_get_all_property_names(
//                      env,
//                      args[0],
//                      napi_key_include_prototypes,
//                      napi_key_skip_strings,
//                      napi_key_numbers_to_strings,
//                      &output));
//
//    return output;
//}

static NAPIValue Set(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 3;
    NAPIValue args[3];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc >= 3, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == NAPIObject,
                "Wrong type of arguments. Expects an object as first argument.");

    NAPIValueType valuetype1;
    NAPI_CALL(env, napi_typeof(env, args[1], &valuetype1));

    NAPI_ASSERT(env, valuetype1 == NAPIString || valuetype1 == NAPISymbol,
                "Wrong type of arguments. Expects a string or symbol as second.");

    NAPI_CALL(env, napi_set_property(env, args[0], args[1], args[2]));

    NAPIValue valuetrue;
    NAPI_CALL(env, napi_get_boolean(env, true, &valuetrue));

    return valuetrue;
}

static NAPIValue SetNamed(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 3;
    NAPIValue args[3];
    char key[256] = "";
    size_t key_length;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc >= 3, "Wrong number of arguments");

    NAPIValueType value_type0;
    NAPI_CALL(env, napi_typeof(env, args[0], &value_type0));

    NAPI_ASSERT(env, value_type0 == NAPIObject,
                "Wrong type of arguments. Expects an object as first argument.");

    NAPIValueType value_type1;
    NAPI_CALL(env, napi_typeof(env, args[1], &value_type1));

    NAPI_ASSERT(env, value_type1 == NAPIString,
                "Wrong type of arguments. Expects a string as second.");

    NAPI_CALL(env,
              napi_get_value_string_utf8(env, args[1], key, 255, &key_length));
    key[255] = 0;
    NAPI_ASSERT(env, key_length <= 255,
                "Cannot accommodate keys longer than 255 bytes");

    NAPI_CALL(env, napi_set_named_property(env, args[0], key, args[2]));

    NAPIValue value_true;
    NAPI_CALL(env, napi_get_boolean(env, true, &value_true));

    return value_true;
}

static NAPIValue Has(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 2;
    NAPIValue args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc >= 2, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == NAPIObject,
                "Wrong type of arguments. Expects an object as first argument.");

    NAPIValueType valuetype1;
    NAPI_CALL(env, napi_typeof(env, args[1], &valuetype1));

    NAPI_ASSERT(env, valuetype1 == NAPIString || valuetype1 == NAPISymbol,
                "Wrong type of arguments. Expects a string or symbol as second.");

    bool has_property;
    NAPI_CALL(env, napi_has_property(env, args[0], args[1], &has_property));

    NAPIValue ret;
    NAPI_CALL(env, napi_get_boolean(env, has_property, &ret));

    return ret;
}

static NAPIValue HasNamed(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 2;
    NAPIValue args[2];
    char key[256] = "";
    size_t key_length;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc >= 2, "Wrong number of arguments");

    NAPIValueType value_type0;
    NAPI_CALL(env, napi_typeof(env, args[0], &value_type0));

    NAPI_ASSERT(env, value_type0 == NAPIObject,
                "Wrong type of arguments. Expects an object as first argument.");

    NAPIValueType value_type1;
    NAPI_CALL(env, napi_typeof(env, args[1], &value_type1));

    NAPI_ASSERT(env, value_type1 == NAPIString || value_type1 == NAPISymbol,
                "Wrong type of arguments. Expects a string as second.");

    NAPI_CALL(env,
              napi_get_value_string_utf8(env, args[1], key, 255, &key_length));
    key[255] = 0;
    NAPI_ASSERT(env, key_length <= 255,
                "Cannot accommodate keys longer than 255 bytes");

    bool has_property;
    NAPI_CALL(env, napi_has_named_property(env, args[0], key, &has_property));

    NAPIValue ret;
    NAPI_CALL(env, napi_get_boolean(env, has_property, &ret));

    return ret;
}

static NAPIValue HasOwn(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 2;
    NAPIValue args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc == 2, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == NAPIObject,
                "Wrong type of arguments. Expects an object as first argument.");

    // NAPIValueType valuetype1;
    // NAPI_CALL(env, napi_typeof(env, args[1], &valuetype1));
    //
    // NAPI_ASSERT(env, valuetype1 == NAPIString || valuetype1 == NAPISymbol,
    //   "Wrong type of arguments. Expects a string or symbol as second.");

    bool has_property;
    NAPI_CALL(env, napi_has_own_property(env, args[0], args[1], &has_property));

    NAPIValue ret;
    NAPI_CALL(env, napi_get_boolean(env, has_property, &ret));

    return ret;
}

static NAPIValue Delete(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 2;
    NAPIValue args[2];

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    NAPI_ASSERT(env, argc == 2, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));
    NAPI_ASSERT(env, valuetype0 == NAPIObject,
                "Wrong type of arguments. Expects an object as first argument.");

    NAPIValueType valuetype1;
    NAPI_CALL(env, napi_typeof(env, args[1], &valuetype1));
    NAPI_ASSERT(env, valuetype1 == NAPIString || valuetype1 == NAPISymbol,
                "Wrong type of arguments. Expects a string or symbol as second.");

    bool result;
    NAPIValue ret;
    NAPI_CALL(env, napi_delete_property(env, args[0], args[1], &result));
    NAPI_CALL(env, napi_get_boolean(env, result, &ret));

    return ret;
}

static NAPIValue New(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue ret;
    NAPI_CALL(env, napi_create_object(env, &ret));

    NAPIValue num;
    NAPI_CALL(env, napi_create_int32(env, 987654321, &num));

    NAPI_CALL(env, napi_set_named_property(env, ret, "test_number", num));

    NAPIValue str;
    const char *str_val = "test string";
//    size_t str_len = strlen(str_val);
    NAPI_CALL(env, napi_create_string_utf8(env, str_val, -1, &str));

    NAPI_CALL(env, napi_set_named_property(env, ret, "test_string", str));

    return ret;
}

static NAPIValue Inflate(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == NAPIObject,
                "Wrong type of arguments. Expects an object as first argument.");

    NAPIValue obj = args[0];
    NAPIValue propertynames;
    NAPI_CALL(env, napi_get_property_names(env, obj, &propertynames));

    uint32_t i, length;
    NAPI_CALL(env, napi_get_array_length(env, propertynames, &length));

    for (i = 0; i < length; i++) {
        NAPIValue property_str;
        NAPI_CALL(env, napi_get_element(env, propertynames, i, &property_str));

        NAPIValue value;
        NAPI_CALL(env, napi_get_property(env, obj, property_str, &value));

        double double_val;
        NAPI_CALL(env, napi_get_value_double(env, value, &double_val));
        NAPI_CALL(env, napi_create_double(env, double_val + 1, &value));
        NAPI_CALL(env, napi_set_property(env, obj, property_str, value));
    }

    return obj;
}

static NAPIValue Wrap(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue arg;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &arg, NULL, NULL));

    NAPI_CALL(env, napi_wrap(env, arg, &test_value, NULL, NULL, NULL));
    return NULL;
}

static NAPIValue Unwrap(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue arg;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &arg, NULL, NULL));

    void *data;
    NAPI_CALL(env, napi_unwrap(env, arg, &data));

    bool is_expected = (data != NULL && *(int *) data == 3);
    NAPIValue result;
    NAPI_CALL(env, napi_get_boolean(env, is_expected, &result));
    return result;
}

static NAPIValue TestSetProperty(NAPIEnv env,
                                 NAPICallbackInfo /*info*/) {
    NAPIStatus status;
    NAPIValue object, key, value;

    NAPI_CALL(env, napi_create_object(env, &object));

    NAPI_CALL(env, napi_create_string_utf8(env, "", NAPI_AUTO_LENGTH, &key));

    NAPI_CALL(env, napi_create_object(env, &value));

    status = napi_set_property(NULL, object, key, value);

    add_returned_status(env,
                        "envIsNull",
                        object,
                        "Invalid argument",
                        NAPIInvalidArg,
                        status);

    napi_set_property(env, NULL, key, value);

    add_last_status(env, "objectIsNull", object);

    napi_set_property(env, object, NULL, value);

    add_last_status(env, "keyIsNull", object);

    napi_set_property(env, object, key, NULL);

    add_last_status(env, "valueIsNull", object);

    return object;
}

static NAPIValue TestHasProperty(NAPIEnv env,
                                 NAPICallbackInfo /*info*/) {
    NAPIStatus status;
    NAPIValue object, key;
    bool result;

    NAPI_CALL(env, napi_create_object(env, &object));

    NAPI_CALL(env, napi_create_string_utf8(env, "", NAPI_AUTO_LENGTH, &key));

    status = napi_has_property(NULL, object, key, &result);

    add_returned_status(env,
                        "envIsNull",
                        object,
                        "Invalid argument",
                        NAPIInvalidArg,
                        status);

    napi_has_property(env, NULL, key, &result);

    add_last_status(env, "objectIsNull", object);

    napi_has_property(env, object, NULL, &result);

    add_last_status(env, "keyIsNull", object);

    napi_has_property(env, object, key, NULL);

    add_last_status(env, "resultIsNull", object);

    return object;
}

static NAPIValue TestGetProperty(NAPIEnv env,
                                 NAPICallbackInfo /*info*/) {
    NAPIStatus status;
    NAPIValue object, key, result;

    NAPI_CALL(env, napi_create_object(env, &object));

    NAPI_CALL(env, napi_create_string_utf8(env, "", NAPI_AUTO_LENGTH, &key));

    NAPI_CALL(env, napi_create_object(env, &result));

    status = napi_get_property(NULL, object, key, &result);

    add_returned_status(env,
                        "envIsNull",
                        object,
                        "Invalid argument",
                        NAPIInvalidArg,
                        status);

    napi_get_property(env, NULL, key, &result);

    add_last_status(env, "objectIsNull", object);

    napi_get_property(env, object, NULL, &result);

    add_last_status(env, "keyIsNull", object);

    napi_get_property(env, object, key, NULL);

    add_last_status(env, "resultIsNull", object);

    return object;
}

//static NAPIValue TestFreeze(NAPIEnv env,
//                             NAPICallbackInfo info) {
//    size_t argc = 1;
//    NAPIValue args[1];
//    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
//
//    NAPIValue object = args[0];
//    NAPI_CALL(env, NAPIObject_freeze(env, object));
//
//    return object;
//}

//static NAPIValue TestSeal(NAPIEnv env,
//                           NAPICallbackInfo info) {
//    size_t argc = 1;
//    NAPIValue args[1];
//    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
//
//    NAPIValue object = args[0];
//    NAPI_CALL(env, NAPIObject_seal(env, object));
//
//    return object;
//}

// We create two type tags. They are basically 128-bit UUIDs.
//static const napi_type_tag type_tags[2] = {
//        {0xdaf987b3cc62481a, 0xb745b0497f299531},
//        {0xbb7936c374084d9b, 0xa9548d0762eeedb9}
//};

//static NAPIValue
//TypeTaggedInstance(NAPIEnv env, NAPICallbackInfo info) {
//    size_t argc = 1;
//    uint32_t type_index;
//    NAPIValue instance, which_type;
//
//    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &which_type, NULL, NULL));
//    NAPI_CALL(env, napi_get_value_uint32(env, which_type, &type_index));
//    NAPI_CALL(env, napi_create_object(env, &instance));
//    NAPI_CALL(env, napi_type_tag_object(env, instance, &type_tags[type_index]));
//
//    return instance;
//}

//static NAPIValue
//CheckTypeTag(NAPIEnv env, NAPICallbackInfo info) {
//    size_t argc = 2;
//    bool result;
//    NAPIValue argv[2], js_result;
//    uint32_t type_index;
//
//    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));
//    NAPI_CALL(env, napi_get_value_uint32(env, argv[0], &type_index));
//    NAPI_CALL(env, napi_check_object_type_tag(env,
//                                              argv[1],
//                                              &type_tags[type_index],
//                                              &result));
//    NAPI_CALL(env, napi_get_boolean(env, result, &js_result));
//
//    return js_result;
//}

EXTERN_C_END

TEST(TestNumber, Test) {
    NAPIEnv env = nullptr;
    ASSERT_EQ(NAPICreateEnv(&env), NAPIOK);

    NAPIValue global = nullptr;
    ASSERT_EQ(napi_get_global(env, &global), NAPIOK);

    NAPIValue exports;
    ASSERT_EQ(napi_create_object(env, &exports), NAPIOK);
    ASSERT_EQ(napi_set_named_property(env, global, "exports", exports), NAPIOK);

    NAPIPropertyDescriptor descriptors[] = {
            DECLARE_NAPI_PROPERTY("Get", Get),
            DECLARE_NAPI_PROPERTY("GetNamed", GetNamed),
            DECLARE_NAPI_PROPERTY("GetPropertyNames", GetPropertyNames),
//            DECLARE_NAPI_PROPERTY("GetSymbolNames", GetSymbolNames),
            DECLARE_NAPI_PROPERTY("Set", Set),
            DECLARE_NAPI_PROPERTY("SetNamed", SetNamed),
            DECLARE_NAPI_PROPERTY("Has", Has),
            DECLARE_NAPI_PROPERTY("HasNamed", HasNamed),
            DECLARE_NAPI_PROPERTY("HasOwn", HasOwn),
            DECLARE_NAPI_PROPERTY("Delete", Delete),
            DECLARE_NAPI_PROPERTY("New", New),
            DECLARE_NAPI_PROPERTY("Inflate", Inflate),
            DECLARE_NAPI_PROPERTY("Wrap", Wrap),
            DECLARE_NAPI_PROPERTY("Unwrap", Unwrap),
            DECLARE_NAPI_PROPERTY("TestSetProperty", TestSetProperty),
            DECLARE_NAPI_PROPERTY("TestHasProperty", TestHasProperty),
//            DECLARE_NAPI_PROPERTY("TypeTaggedInstance", TypeTaggedInstance),
//            DECLARE_NAPI_PROPERTY("CheckTypeTag", CheckTypeTag),
            DECLARE_NAPI_PROPERTY("TestGetProperty", TestGetProperty),
//            DECLARE_NAPI_PROPERTY("TestFreeze", TestFreeze),
//            DECLARE_NAPI_PROPERTY("TestSeal", TestSeal),
    };

    ASSERT_EQ(napi_define_properties(
            env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors), NAPIOK);

    EXPECT_EQ(initAssert(env, global), NAPIOK);

    NAPIValue result = nullptr;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(env, "(function () {\n"
                                              "    const object = {\n"
                                              "        hello: 'world',\n"
                                              "        array: [\n"
                                              "            1, 94, 'str', 12.321, { test: 'obj in arr' }\n"
                                              "        ],\n"
                                              "        newObject: {\n"
                                              "            test: 'obj in obj'\n"
                                              "        }\n"
                                              "    };\n"
                                              "\n"
                                              "    assert.strictEqual(globalThis.exports.Get(object, 'hello'), 'world');\n"
                                              "    assert.strictEqual(globalThis.exports.GetNamed(object, 'hello'), 'world');\n"
                                              "\n"
                                              "    assert(globalThis.exports.Has(object, 'hello'));\n"
                                              "    assert(globalThis.exports.HasNamed(object, 'hello'));\n"
                                              "    assert(globalThis.exports.Has(object, 'array'));\n"
                                              "    assert(globalThis.exports.Has(object, 'newObject'));\n"
                                              "\n"
                                              "    const newObject = globalThis.exports.New();\n"
                                              "    assert(globalThis.exports.Has(newObject, 'test_number'));\n"
                                              "    assert.strictEqual(newObject.test_number, 987654321);\n"
                                              "    assert.strictEqual(newObject.test_string, 'test string');\n"
                                              "\n"
                                              "    {\n"
                                              "        // Verify that napi_get_property() walks the prototype chain.\n"
                                              "        function MyObject() {\n"
                                              "            this.foo = 42;\n"
                                              "            this.bar = 43;\n"
                                              "        }\n"
                                              "\n"
                                              "        MyObject.prototype.bar = 44;\n"
                                              "        MyObject.prototype.baz = 45;\n"
                                              "\n"
                                              "        const obj = new MyObject();\n"
                                              "\n"
                                              "        assert.strictEqual(globalThis.exports.Get(obj, 'foo'), 42);\n"
                                              "        assert.strictEqual(globalThis.exports.Get(obj, 'bar'), 43);\n"
                                              "        assert.strictEqual(globalThis.exports.Get(obj, 'baz'), 45);\n"
                                              "        assert.strictEqual(globalThis.exports.Get(obj, 'toString'),\n"
                                              "            Object.prototype.toString);\n"
                                              "    }\n"
                                              "\n"
                                              "    {\n"
                                              "        // Verify that napi_has_own_property() fails if property is not a name.\n"
                                              "        [true, false, null, undefined, {}, [], 0, 1, () => { }].forEach((value) => {\n"
                                              "            assert.throws(() => {\n"
                                              "                globalThis.exports.HasOwn({}, value);\n"
                                              "            });\n"
                                              "        });\n"
                                              "    }\n"
                                              "\n"
                                              "    {\n"
                                              "        // Verify that napi_has_own_property() does not walk the prototype chain.\n"
                                              "        const symbol1 = Symbol();\n"
                                              "        const symbol2 = Symbol();\n"
                                              "\n"
                                              "        function MyObject() {\n"
                                              "            this.foo = 42;\n"
                                              "            this.bar = 43;\n"
                                              "            this[symbol1] = 44;\n"
                                              "        }\n"
                                              "\n"
                                              "        MyObject.prototype.bar = 45;\n"
                                              "        MyObject.prototype.baz = 46;\n"
                                              "        MyObject.prototype[symbol2] = 47;\n"
                                              "\n"
                                              "        const obj = new MyObject();\n"
                                              "\n"
                                              "        assert.strictEqual(globalThis.exports.HasOwn(obj, 'foo'), true);\n"
                                              "        assert.strictEqual(globalThis.exports.HasOwn(obj, 'bar'), true);\n"
                                              "        assert.strictEqual(globalThis.exports.HasOwn(obj, symbol1), true);\n"
                                              "        assert.strictEqual(globalThis.exports.HasOwn(obj, 'baz'), false);\n"
                                              "        assert.strictEqual(globalThis.exports.HasOwn(obj, 'toString'), false);\n"
                                              "        assert.strictEqual(globalThis.exports.HasOwn(obj, symbol2), false);\n"
                                              "    }\n"
                                              "\n"
                                              "    {\n"
                                              "        const object2 = {};\n"
                                              "\n"
                                              "        assert(globalThis.exports.Set(object2, 'string', 'value'));\n"
                                              "        assert(globalThis.exports.SetNamed(object2, 'named_string', 'value'));\n"
                                              "        assert(globalThis.exports.Has(object2, 'string'));\n"
                                              "        assert(globalThis.exports.HasNamed(object2, 'named_string'));\n"
                                              "        assert.strictEqual(globalThis.exports.Get(object2, 'string'), 'value');\n"
                                              "    }\n"
                                              "\n"
                                              "    {\n"
                                              "        // Wrap a pointer in a JS object, then verify the pointer can be unwrapped.\n"
                                              "        const wrapper = {};\n"
                                              "        globalThis.exports.Wrap(wrapper);\n"
                                              "\n"
                                              "        assert(globalThis.exports.Unwrap(wrapper));\n"
                                              "    }\n"
                                              "\n"
                                              "    {\n"
                                              "        // Verify that wrapping doesn't break an object's prototype chain.\n"
                                              "        const wrapper = {};\n"
                                              "        const protoA = { protoA: true };\n"
                                              "        Object.setPrototypeOf(wrapper, protoA);\n"
                                              "        globalThis.exports.Wrap(wrapper);\n"
                                              "\n"
                                              "        assert(globalThis.exports.Unwrap(wrapper));\n"
                                              "        assert(wrapper.protoA);\n"
                                              "    }\n"
                                              "\n"
                                              "    {\n"
                                              "        // Verify that normal and nonexistent properties can be deleted.\n"
                                              "        const sym = Symbol();\n"
                                              "        const obj = { foo: 'bar', [sym]: 'baz' };\n"
                                              "\n"
                                              "        assert.strictEqual('foo' in obj, true);\n"
                                              "        assert.strictEqual(sym in obj, true);\n"
                                              "        assert.strictEqual('does_not_exist' in obj, false);\n"
                                              "        assert.strictEqual(globalThis.exports.Delete(obj, 'foo'), true);\n"
                                              "        assert.strictEqual('foo' in obj, false);\n"
                                              "        assert.strictEqual(sym in obj, true);\n"
                                              "        assert.strictEqual('does_not_exist' in obj, false);\n"
                                              "        assert.strictEqual('foo' in obj, false);\n"
                                              "        assert.strictEqual(sym in obj, false);\n"
                                              "        assert.strictEqual('does_not_exist' in obj, false);\n"
                                              "    }\n"
                                              "\n"
                                              "    {\n"
                                              "        // Verify that non-configurable properties are not deleted.\n"
                                              "        const obj = {};\n"
                                              "\n"
                                              "        Object.defineProperty(obj, 'foo', { configurable: false });\n"
                                              "        assert.strictEqual(globalThis.exports.Delete(obj, 'foo'), false);\n"
                                              "        assert.strictEqual('foo' in obj, true);\n"
                                              "    }\n"
                                              "\n"
                                              "    {\n"
                                              "        // Verify that prototype properties are not deleted.\n"
                                              "        function Foo() {\n"
                                              "            this.foo = 'bar';\n"
                                              "        }\n"
                                              "\n"
                                              "        Foo.prototype.foo = 'baz';\n"
                                              "\n"
                                              "        const obj = new Foo();\n"
                                              "\n"
                                              "        assert.strictEqual(obj.foo, 'bar');\n"
                                              "        assert.strictEqual(globalThis.exports.Delete(obj, 'foo'), true);\n"
                                              "        assert.strictEqual(obj.foo, 'baz');\n"
                                              "        assert.strictEqual(globalThis.exports.Delete(obj, 'foo'), true);\n"
                                              "        assert.strictEqual(obj.foo, 'baz');\n"
                                              "    }\n"
                                              "})();",
                                         "https://n-api.com/test_object.js",
                                         &result), NAPIOK);
    NAPIValueType valueType;
    EXPECT_EQ(napi_typeof(env, result, &valueType), NAPIOK);
    EXPECT_EQ(valueType, NAPIUndefined);

    ASSERT_EQ(NAPIFreeEnv(env), NAPIOK);
}