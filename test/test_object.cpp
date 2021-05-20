#include <js_native_api_test.h>
#include <common.h>

EXTERN_C_START

static int test_value = 3;

static NAPIValue Get(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 2;
    NAPIValue args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

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
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

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
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValueType value_type0;
    NAPI_CALL(env, napi_typeof(env, args[0], &value_type0));

    NAPI_ASSERT(env, value_type0 == NAPIObject,
                "Wrong type of arguments. Expects an object as first argument.");

    NAPIValue output;
    NAPI_CALL(env, napi_get_property_names(env, args[0], &output));

    return output;
}

//static NAPIValue GetSymbolNames(NAPIEnv globalEnv, NAPICallbackInfo info) {
//    size_t argc = 1;
//    NAPIValue args[1];
//    NAPI_CALL(globalEnv, napi_get_cb_info(globalEnv, info, &argc, args, nullptr, nullptr));
//
//    NAPI_ASSERT(globalEnv, argc >= 1, "Wrong number of arguments");
//
//    NAPIValueType value_type0;
//    NAPI_CALL(globalEnv, napi_typeof(globalEnv, args[0], &value_type0));
//
//    NAPI_ASSERT(globalEnv,
//                value_type0 == NAPIObject,
//                "Wrong type of arguments. Expects an object as first argument.");
//
//    NAPIValue output;
//    NAPI_CALL(globalEnv,
//              napi_get_all_property_names(
//                      globalEnv,
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
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

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
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

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
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

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
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

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
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc == 2, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == NAPIObject,
                "Wrong type of arguments. Expects an object as first argument.");

    // NAPIValueType valuetype1;
    // NAPI_CALL(globalEnv, napi_typeof(globalEnv, args[1], &valuetype1));
    //
    // NAPI_ASSERT(globalEnv, valuetype1 == NAPIString || valuetype1 == NAPISymbol,
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

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));
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
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

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
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &arg, nullptr, nullptr));

    NAPI_CALL(env, napi_wrap(env, arg, &test_value, nullptr, nullptr, nullptr));
    return getUndefined(env);
}

static NAPIValue Unwrap(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue arg;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &arg, nullptr, nullptr));

    void *data;
    NAPI_CALL(env, napi_unwrap(env, arg, &data));

    bool is_expected = (data != nullptr && *(int *) data == 3);
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

    status = napi_set_property(nullptr, object, key, value);

    add_returned_status(env,
                        "envIsNull",
                        object,
                        "Invalid argument",
                        NAPIInvalidArg,
                        status);

    napi_set_property(env, getUndefined(env), key, value);

    add_last_status(env, "objectIsNull", object);

    napi_set_property(env, object, getUndefined(env), value);

    add_last_status(env, "keyIsNull", object);

    napi_set_property(env, object, key, getUndefined(env));

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

    status = napi_has_property(nullptr, object, key, &result);

    add_returned_status(env,
                        "envIsNull",
                        object,
                        "Invalid argument",
                        NAPIInvalidArg,
                        status);

    napi_has_property(env, getUndefined(env), key, &result);

    add_last_status(env, "objectIsNull", object);

    napi_has_property(env, object, getUndefined(env), &result);

    add_last_status(env, "keyIsNull", object);

    napi_has_property(env, object, key, nullptr);

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

    status = napi_get_property(nullptr, object, key, &result);

    add_returned_status(env,
                        "envIsNull",
                        object,
                        "Invalid argument",
                        NAPIInvalidArg,
                        status);

    napi_get_property(env, getUndefined(env), key, &result);

    add_last_status(env, "objectIsNull", object);

    napi_get_property(env, object, getUndefined(env), &result);

    add_last_status(env, "keyIsNull", object);

    napi_get_property(env, object, key, nullptr);

    add_last_status(env, "resultIsNull", object);

    return object;
}

//static NAPIValue TestFreeze(NAPIEnv globalEnv,
//                             NAPICallbackInfo info) {
//    size_t argc = 1;
//    NAPIValue args[1];
//    NAPI_CALL(globalEnv, napi_get_cb_info(globalEnv, info, &argc, args, nullptr, nullptr));
//
//    NAPIValue object = args[0];
//    NAPI_CALL(globalEnv, NAPIObject_freeze(globalEnv, object));
//
//    return object;
//}

//static NAPIValue TestSeal(NAPIEnv globalEnv,
//                           NAPICallbackInfo info) {
//    size_t argc = 1;
//    NAPIValue args[1];
//    NAPI_CALL(globalEnv, napi_get_cb_info(globalEnv, info, &argc, args, nullptr, nullptr));
//
//    NAPIValue object = args[0];
//    NAPI_CALL(globalEnv, NAPIObject_seal(globalEnv, object));
//
//    return object;
//}

// We create two type tags. They are basically 128-bit UUIDs.
//static const napi_type_tag type_tags[2] = {
//        {0xdaf987b3cc62481a, 0xb745b0497f299531},
//        {0xbb7936c374084d9b, 0xa9548d0762eeedb9}
//};

//static NAPIValue
//TypeTaggedInstance(NAPIEnv globalEnv, NAPICallbackInfo info) {
//    size_t argc = 1;
//    uint32_t type_index;
//    NAPIValue instance, which_type;
//
//    NAPI_CALL(globalEnv, napi_get_cb_info(globalEnv, info, &argc, &which_type, nullptr, nullptr));
//    NAPI_CALL(globalEnv, napi_get_value_uint32(globalEnv, which_type, &type_index));
//    NAPI_CALL(globalEnv, napi_create_object(globalEnv, &instance));
//    NAPI_CALL(globalEnv, napi_type_tag_object(globalEnv, instance, &type_tags[type_index]));
//
//    return instance;
//}

//static NAPIValue
//CheckTypeTag(NAPIEnv globalEnv, NAPICallbackInfo info) {
//    size_t argc = 2;
//    bool result;
//    NAPIValue argv[2], js_result;
//    uint32_t type_index;
//
//    NAPI_CALL(globalEnv, napi_get_cb_info(globalEnv, info, &argc, argv, nullptr, nullptr));
//    NAPI_CALL(globalEnv, napi_get_value_uint32(globalEnv, argv[0], &type_index));
//    NAPI_CALL(globalEnv, napi_check_object_type_tag(globalEnv,
//                                              argv[1],
//                                              &type_tags[type_index],
//                                              &result));
//    NAPI_CALL(globalEnv, napi_get_boolean(globalEnv, result, &js_result));
//
//    return js_result;
//}

EXTERN_C_END

TEST_F(Test, TestObject) {
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
            globalEnv, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors), NAPIOK);

    NAPIValue result;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(globalEnv,
                                         "(()=>{var t={7473:(t,r,e)=>{e(8922),e(5967),e(5824),e(8555),e(2615),e(1732),e(5903),e(1825),e(8394),e(5915),e(1766),e(2737),e(9911),e(4315),e(3131),e(4714),e(659),e(9120),e(5327),e(1502);var o=e(4058);t.exports=o.Symbol},3916:t=>{t.exports=function(t){if(\"function\"!=typeof t)throw TypeError(String(t)+\" is not a function\");return t}},6059:(t,r,e)=>{var o=e(941);t.exports=function(t){if(!o(t))throw TypeError(String(t)+\" is not an object\");return t}},1692:(t,r,e)=>{var o=e(4529),n=e(3057),a=e(9413),i=function(t){return function(r,e,i){var s,l=o(r),u=n(l.length),c=a(i,u);if(t&&e!=e){for(;u>c;)if((s=l[c++])!=s)return!0}else for(;u>c;c++)if((t||c in l)&&l[c]===e)return t||c||0;return!t&&-1}};t.exports={includes:i(!0),indexOf:i(!1)}},3610:(t,r,e)=>{var o=e(6843),n=e(7026),a=e(9678),i=e(3057),s=e(4692),l=[].push,u=function(t){var r=1==t,e=2==t,u=3==t,c=4==t,f=6==t,p=7==t,b=5==t||f;return function(h,g,d,v){for(var y,T,m=a(h),w=n(m),x=o(g,d,3),O=i(w.length),S=0,E=v||s,j=r?E(h,O):e||p?E(h,0):void 0;O>S;S++)if((b||S in w)&&(T=x(y=w[S],S,m),t))if(r)j[S]=T;else if(T)switch(t){case 3:return!0;case 5:return y;case 6:return S;case 2:l.call(j,y)}else switch(t){case 4:return!1;case 7:l.call(j,y)}return f?-1:u||c?c:j}};t.exports={forEach:u(0),map:u(1),filter:u(2),some:u(3),every:u(4),find:u(5),findIndex:u(6),filterOut:u(7)}},568:(t,r,e)=>{var o=e(5981),n=e(9813),a=e(3385),i=n(\"species\");t.exports=function(t){return a>=51||!o((function(){var r=[];return(r.constructor={})[i]=function(){return{foo:1}},1!==r[t](Boolean).foo}))}},4692:(t,r,e)=>{var o=e(941),n=e(1052),a=e(9813)(\"species\");t.exports=function(t,r){var e;return n(t)&&(\"function\"!=typeof(e=t.constructor)||e!==Array&&!n(e.prototype)?o(e)&&null===(e=e[a])&&(e=void 0):e=void 0),new(void 0===e?Array:e)(0===r?0:r)}},2532:t=>{var r={}.toString;t.exports=function(t){return r.call(t).slice(8,-1)}},9697:(t,r,e)=>{var o=e(2885),n=e(2532),a=e(9813)(\"toStringTag\"),i=\"Arguments\"==n(function(){return arguments}());t.exports=o?n:function(t){var r,e,o;return void 0===t?\"Undefined\":null===t?\"Null\":\"string\"==typeof(e=function(t,r){try{return t[r]}catch(t){}}(r=Object(t),a))?e:i?n(r):\"Object\"==(o=n(r))&&\"function\"==typeof r.callee?\"Arguments\":o}},2029:(t,r,e)=>{var o=e(5746),n=e(5988),a=e(1887);t.exports=o?function(t,r,e){return n.f(t,r,a(1,e))}:function(t,r,e){return t[r]=e,t}},1887:t=>{t.exports=function(t,r){return{enumerable:!(1&t),configurable:!(2&t),writable:!(4&t),value:r}}},5449:(t,r,e)=>{\"use strict\";var o=e(6935),n=e(5988),a=e(1887);t.exports=function(t,r,e){var i=o(r);i in t?n.f(t,i,a(0,e)):t[i]=e}},6349:(t,r,e)=>{var o=e(4058),n=e(7457),a=e(1477),i=e(5988).f;t.exports=function(t){var r=o.Symbol||(o.Symbol={});n(r,t)||i(r,t,{value:a.f(t)})}},5746:(t,r,e)=>{var o=e(5981);t.exports=!o((function(){return 7!=Object.defineProperty({},1,{get:function(){return 7}})[1]}))},1333:(t,r,e)=>{var o=e(1899),n=e(941),a=o.document,i=n(a)&&n(a.createElement);t.exports=function(t){return i?a.createElement(t):{}}},6049:(t,r,e)=>{var o=e(2532),n=e(1899);t.exports=\"process\"==o(n.process)},2861:(t,r,e)=>{var o=e(626);t.exports=o(\"navigator\",\"userAgent\")||\"\"},3385:(t,r,e)=>{var o,n,a=e(1899),i=e(2861),s=a.process,l=s&&s.versions,u=l&&l.v8;u?n=(o=u.split(\".\"))[0]+o[1]:i&&(!(o=i.match(/Edge\\/(\\d+)/))||o[1]>=74)&&(o=i.match(/Chrome\\/(\\d+)/))&&(n=o[1]),t.exports=n&&+n},6759:t=>{t.exports=[\"constructor\",\"hasOwnProperty\",\"isPrototypeOf\",\"propertyIsEnumerable\",\"toLocaleString\",\"toString\",\"valueOf\"]},6887:(t,r,e)=>{\"use strict\";var o=e(1899),n=e(9677).f,a=e(7252),i=e(4058),s=e(6843),l=e(2029),u=e(7457),c=function(t){var r=function(r,e,o){if(this instanceof t){switch(arguments.length){case 0:return new t;case 1:return new t(r);case 2:return new t(r,e)}return new t(r,e,o)}return t.apply(this,arguments)};return r.prototype=t.prototype,r};t.exports=function(t,r){var e,f,p,b,h,g,d,v,y=t.target,T=t.global,m=t.stat,w=t.proto,x=T?o:m?o[y]:(o[y]||{}).prototype,O=T?i:i[y]||(i[y]={}),S=O.prototype;for(p in r)e=!a(T?p:y+(m?\".\":\"#\")+p,t.forced)&&x&&u(x,p),h=O[p],e&&(g=t.noTargetGet?(v=n(x,p))&&v.value:x[p]),b=e&&g?g:r[p],e&&typeof h==typeof b||(d=t.bind&&e?s(b,o):t.wrap&&e?c(b):w&&\"function\"==typeof b?s(Function.call,b):b,(t.sham||b&&b.sham||h&&h.sham)&&l(d,\"sham\",!0),O[p]=d,w&&(u(i,f=y+\"Prototype\")||l(i,f,{}),i[f][p]=b,t.real&&S&&!S[p]&&l(S,p,b)))}},5981:t=>{t.exports=function(t){try{return!!t()}catch(t){return!0}}},6843:(t,r,e)=>{var o=e(3916);t.exports=function(t,r,e){if(o(t),void 0===r)return t;switch(e){case 0:return function(){return t.call(r)};case 1:return function(e){return t.call(r,e)};case 2:return function(e,o){return t.call(r,e,o)};case 3:return function(e,o,n){return t.call(r,e,o,n)}}return function(){return t.apply(r,arguments)}}},626:(t,r,e)=>{var o=e(4058),n=e(1899),a=function(t){return\"function\"==typeof t?t:void 0};t.exports=function(t,r){return arguments.length<2?a(o[t])||a(n[t]):o[t]&&o[t][r]||n[t]&&n[t][r]}},1899:(t,r,e)=>{var o=function(t){return t&&t.Math==Math&&t};t.exports=o(\"object\"==typeof globalThis&&globalThis)||o(\"object\"==typeof window&&window)||o(\"object\"==typeof self&&self)||o(\"object\"==typeof e.g&&e.g)||function(){return this}()||Function(\"return this\")()},7457:(t,r,e)=>{var o=e(9678),n={}.hasOwnProperty;t.exports=function(t,r){return n.call(o(t),r)}},7748:t=>{t.exports={}},5463:(t,r,e)=>{var o=e(626);t.exports=o(\"document\",\"documentElement\")},2840:(t,r,e)=>{var o=e(5746),n=e(5981),a=e(1333);t.exports=!o&&!n((function(){return 7!=Object.defineProperty(a(\"div\"),\"a\",{get:function(){return 7}}).a}))},7026:(t,r,e)=>{var o=e(5981),n=e(2532),a=\"\".split;t.exports=o((function(){return!Object(\"z\").propertyIsEnumerable(0)}))?function(t){return\"String\"==n(t)?a.call(t,\"\"):Object(t)}:Object},1302:(t,r,e)=>{var o=e(3030),n=Function.toString;\"function\"!=typeof o.inspectSource&&(o.inspectSource=function(t){return n.call(t)}),t.exports=o.inspectSource},5402:(t,r,e)=>{var o,n,a,i=e(8019),s=e(1899),l=e(941),u=e(2029),c=e(7457),f=e(3030),p=e(4262),b=e(7748),h=\"Object already initialized\",g=s.WeakMap;if(i){var d=f.state||(f.state=new g),v=d.get,y=d.has,T=d.set;o=function(t,r){if(y.call(d,t))throw new TypeError(h);return r.facade=t,T.call(d,t,r),r},n=function(t){return v.call(d,t)||{}},a=function(t){return y.call(d,t)}}else{var m=p(\"state\");b[m]=!0,o=function(t,r){if(c(t,m))throw new TypeError(h);return r.facade=t,u(t,m,r),r},n=function(t){return c(t,m)?t[m]:{}},a=function(t){return c(t,m)}}t.exports={set:o,get:n,has:a,enforce:function(t){return a(t)?n(t):o(t,{})},getterFor:function(t){return function(r){var e;if(!l(r)||(e=n(r)).type!==t)throw TypeError(\"Incompatible receiver, \"+t+\" required\");return e}}}},1052:(t,r,e)=>{var o=e(2532);t.exports=Array.isArray||function(t){return\"Array\"==o(t)}},7252:(t,r,e)=>{var o=e(5981),n=/#|\\.prototype\\./,a=function(t,r){var e=s[i(t)];return e==u||e!=l&&(\"function\"==typeof r?o(r):!!r)},i=a.normalize=function(t){return String(t).replace(n,\".\").toLowerCase()},s=a.data={},l=a.NATIVE=\"N\",u=a.POLYFILL=\"P\";t.exports=a},941:t=>{t.exports=function(t){return\"object\"==typeof t?null!==t:\"function\"==typeof t}},2529:t=>{t.exports=!0},2497:(t,r,e)=>{var o=e(6049),n=e(3385),a=e(5981);t.exports=!!Object.getOwnPropertySymbols&&!a((function(){return!Symbol.sham&&(o?38===n:n>37&&n<41)}))},8019:(t,r,e)=>{var o=e(1899),n=e(1302),a=o.WeakMap;t.exports=\"function\"==typeof a&&/native code/.test(n(a))},9290:(t,r,e)=>{var o,n=e(6059),a=e(9938),i=e(6759),s=e(7748),l=e(5463),u=e(1333),c=e(4262)(\"IE_PROTO\"),f=function(){},p=function(t){return\"<script>\"+t+\"<\\/script>\"},b=function(){try{o=document.domain&&new ActiveXObject(\"htmlfile\")}catch(t){}var t,r;b=o?function(t){t.write(p(\"\")),t.close();var r=t.parentWindow.Object;return t=null,r}(o):((r=u(\"iframe\")).style.display=\"none\",l.appendChild(r),r.src=String(\"javascript:\"),(t=r.contentWindow.document).open(),t.write(p(\"document.F=Object\")),t.close(),t.F);for(var e=i.length;e--;)delete b.prototype[i[e]];return b()};s[c]=!0,t.exports=Object.create||function(t,r){var e;return null!==t?(f.prototype=n(t),e=new f,f.prototype=null,e[c]=t):e=b(),void 0===r?e:a(e,r)}},9938:(t,r,e)=>{var o=e(5746),n=e(5988),a=e(6059),i=e(4771);t.exports=o?Object.defineProperties:function(t,r){a(t);for(var e,o=i(r),s=o.length,l=0;s>l;)n.f(t,e=o[l++],r[e]);return t}},5988:(t,r,e)=>{var o=e(5746),n=e(2840),a=e(6059),i=e(6935),s=Object.defineProperty;r.f=o?s:function(t,r,e){if(a(t),r=i(r,!0),a(e),n)try{return s(t,r,e)}catch(t){}if(\"get\"in e||\"set\"in e)throw TypeError(\"Accessors not supported\");return\"value\"in e&&(t[r]=e.value),t}},9677:(t,r,e)=>{var o=e(5746),n=e(6760),a=e(1887),i=e(4529),s=e(6935),l=e(7457),u=e(2840),c=Object.getOwnPropertyDescriptor;r.f=o?c:function(t,r){if(t=i(t),r=s(r,!0),u)try{return c(t,r)}catch(t){}if(l(t,r))return a(!n.f.call(t,r),t[r])}},684:(t,r,e)=>{var o=e(4529),n=e(946).f,a={}.toString,i=\"object\"==typeof window&&window&&Object.getOwnPropertyNames?Object.getOwnPropertyNames(window):[];t.exports.f=function(t){return i&&\"[object Window]\"==a.call(t)?function(t){try{return n(t)}catch(t){return i.slice()}}(t):n(o(t))}},946:(t,r,e)=>{var o=e(5629),n=e(6759).concat(\"length\",\"prototype\");r.f=Object.getOwnPropertyNames||function(t){return o(t,n)}},7857:(t,r)=>{r.f=Object.getOwnPropertySymbols},5629:(t,r,e)=>{var o=e(7457),n=e(4529),a=e(1692).indexOf,i=e(7748);t.exports=function(t,r){var e,s=n(t),l=0,u=[];for(e in s)!o(i,e)&&o(s,e)&&u.push(e);for(;r.length>l;)o(s,e=r[l++])&&(~a(u,e)||u.push(e));return u}},4771:(t,r,e)=>{var o=e(5629),n=e(6759);t.exports=Object.keys||function(t){return o(t,n)}},6760:(t,r)=>{\"use strict\";var e={}.propertyIsEnumerable,o=Object.getOwnPropertyDescriptor,n=o&&!e.call({1:2},1);r.f=n?function(t){var r=o(this,t);return!!r&&r.enumerable}:e},5623:(t,r,e)=>{\"use strict\";var o=e(2885),n=e(9697);t.exports=o?{}.toString:function(){return\"[object \"+n(this)+\"]\"}},4058:t=>{t.exports={}},9754:(t,r,e)=>{var o=e(2029);t.exports=function(t,r,e,n){n&&n.enumerable?t[r]=e:o(t,r,e)}},8219:t=>{t.exports=function(t){if(null==t)throw TypeError(\"Can't call method on \"+t);return t}},4911:(t,r,e)=>{var o=e(1899),n=e(2029);t.exports=function(t,r){try{n(o,t,r)}catch(e){o[t]=r}return r}},904:(t,r,e)=>{var o=e(2885),n=e(5988).f,a=e(2029),i=e(7457),s=e(5623),l=e(9813)(\"toStringTag\");t.exports=function(t,r,e,u){if(t){var c=e?t:t.prototype;i(c,l)||n(c,l,{configurable:!0,value:r}),u&&!o&&a(c,\"toString\",s)}}},4262:(t,r,e)=>{var o=e(8726),n=e(9418),a=o(\"keys\");t.exports=function(t){return a[t]||(a[t]=n(t))}},3030:(t,r,e)=>{var o=e(1899),n=e(4911),a=\"__core-js_shared__\",i=o[a]||n(a,{});t.exports=i},8726:(t,r,e)=>{var o=e(2529),n=e(3030);(t.exports=function(t,r){return n[t]||(n[t]=void 0!==r?r:{})})(\"versions\",[]).push({version:\"3.11.1\",mode:o?\"pure\":\"global\",copyright:\"© 2021 Denis Pushkarev (zloirock.ru)\"})},9413:(t,r,e)=>{var o=e(8459),n=Math.max,a=Math.min;t.exports=function(t,r){var e=o(t);return e<0?n(e+r,0):a(e,r)}},4529:(t,r,e)=>{var o=e(7026),n=e(8219);t.exports=function(t){return o(n(t))}},8459:t=>{var r=Math.ceil,e=Math.floor;t.exports=function(t){return isNaN(t=+t)?0:(t>0?e:r)(t)}},3057:(t,r,e)=>{var o=e(8459),n=Math.min;t.exports=function(t){return t>0?n(o(t),9007199254740991):0}},9678:(t,r,e)=>{var o=e(8219);t.exports=function(t){return Object(o(t))}},6935:(t,r,e)=>{var o=e(941);t.exports=function(t,r){if(!o(t))return t;var e,n;if(r&&\"function\"==typeof(e=t.toString)&&!o(n=e.call(t)))return n;if(\"function\"==typeof(e=t.valueOf)&&!o(n=e.call(t)))return n;if(!r&&\"function\"==typeof(e=t.toString)&&!o(n=e.call(t)))return n;throw TypeError(\"Can't convert object to primitive value\")}},2885:(t,r,e)=>{var o={};o[e(9813)(\"toStringTag\")]=\"z\",t.exports=\"[object z]\"===String(o)},9418:t=>{var r=0,e=Math.random();t.exports=function(t){return\"Symbol(\"+String(void 0===t?\"\":t)+\")_\"+(++r+e).toString(36)}},2302:(t,r,e)=>{var o=e(2497);t.exports=o&&!Symbol.sham&&\"symbol\"==typeof Symbol.iterator},1477:(t,r,e)=>{var o=e(9813);r.f=o},9813:(t,r,e)=>{var o=e(1899),n=e(8726),a=e(7457),i=e(9418),s=e(2497),l=e(2302),u=n(\"wks\"),c=o.Symbol,f=l?c:c&&c.withoutSetter||i;t.exports=function(t){return a(u,t)&&(s||\"string\"==typeof u[t])||(s&&a(c,t)?u[t]=c[t]:u[t]=f(\"Symbol.\"+t)),u[t]}},8922:(t,r,e)=>{\"use strict\";var o=e(6887),n=e(5981),a=e(1052),i=e(941),s=e(9678),l=e(3057),u=e(5449),c=e(4692),f=e(568),p=e(9813),b=e(3385),h=p(\"isConcatSpreadable\"),g=9007199254740991,d=\"Maximum allowed index exceeded\",v=b>=51||!n((function(){var t=[];return t[h]=!1,t.concat()[0]!==t})),y=f(\"concat\"),T=function(t){if(!i(t))return!1;var r=t[h];return void 0!==r?!!r:a(t)};o({target:\"Array\",proto:!0,forced:!v||!y},{concat:function(t){var r,e,o,n,a,i=s(this),f=c(i,0),p=0;for(r=-1,o=arguments.length;r<o;r++)if(T(a=-1===r?i:arguments[r])){if(p+(n=l(a.length))>g)throw TypeError(d);for(e=0;e<n;e++,p++)e in a&&u(f,p,a[e])}else{if(p>=g)throw TypeError(d);u(f,p++,a)}return f.length=p,f}})},9120:(t,r,e)=>{var o=e(1899);e(904)(o.JSON,\"JSON\",!0)},5327:()=>{},5967:()=>{},1502:()=>{},8555:(t,r,e)=>{e(6349)(\"asyncIterator\")},2615:()=>{},1732:(t,r,e)=>{e(6349)(\"hasInstance\")},5903:(t,r,e)=>{e(6349)(\"isConcatSpreadable\")},1825:(t,r,e)=>{e(6349)(\"iterator\")},5824:(t,r,e)=>{\"use strict\";var o=e(6887),n=e(1899),a=e(626),i=e(2529),s=e(5746),l=e(2497),u=e(2302),c=e(5981),f=e(7457),p=e(1052),b=e(941),h=e(6059),g=e(9678),d=e(4529),v=e(6935),y=e(1887),T=e(9290),m=e(4771),w=e(946),x=e(684),O=e(7857),S=e(9677),E=e(5988),j=e(6760),q=e(2029),P=e(9754),_=e(8726),N=e(4262),A=e(7748),z=e(9418),H=e(9813),M=e(1477),I=e(6349),F=e(904),k=e(5402),C=e(3610).forEach,D=N(\"hidden\"),G=\"Symbol\",W=H(\"toPrimitive\"),L=k.set,J=k.getterFor(G),U=Object.prototype,B=n.Symbol,Q=a(\"JSON\",\"stringify\"),R=S.f,V=E.f,X=x.f,Y=j.f,K=_(\"symbols\"),Z=_(\"op-symbols\"),$=_(\"string-to-symbol-registry\"),tt=_(\"symbol-to-string-registry\"),rt=_(\"wks\"),et=n.QObject,ot=!et||!et.prototype||!et.prototype.findChild,nt=s&&c((function(){return 7!=T(V({},\"a\",{get:function(){return V(this,\"a\",{value:7}).a}})).a}))?function(t,r,e){var o=R(U,r);o&&delete U[r],V(t,r,e),o&&t!==U&&V(U,r,o)}:V,at=function(t,r){var e=K[t]=T(B.prototype);return L(e,{type:G,tag:t,description:r}),s||(e.description=r),e},it=u?function(t){return\"symbol\"==typeof t}:function(t){return Object(t)instanceof B},st=function(t,r,e){t===U&&st(Z,r,e),h(t);var o=v(r,!0);return h(e),f(K,o)?(e.enumerable?(f(t,D)&&t[D][o]&&(t[D][o]=!1),e=T(e,{enumerable:y(0,!1)})):(f(t,D)||V(t,D,y(1,{})),t[D][o]=!0),nt(t,o,e)):V(t,o,e)},lt=function(t,r){h(t);var e=d(r),o=m(e).concat(pt(e));return C(o,(function(r){s&&!ut.call(e,r)||st(t,r,e[r])})),t},ut=function(t){var r=v(t,!0),e=Y.call(this,r);return!(this===U&&f(K,r)&&!f(Z,r))&&(!(e||!f(this,r)||!f(K,r)||f(this,D)&&this[D][r])||e)},ct=function(t,r){var e=d(t),o=v(r,!0);if(e!==U||!f(K,o)||f(Z,o)){var n=R(e,o);return!n||!f(K,o)||f(e,D)&&e[D][o]||(n.enumerable=!0),n}},ft=function(t){var r=X(d(t)),e=[];return C(r,(function(t){f(K,t)||f(A,t)||e.push(t)})),e},pt=function(t){var r=t===U,e=X(r?Z:d(t)),o=[];return C(e,(function(t){!f(K,t)||r&&!f(U,t)||o.push(K[t])})),o};l||(P((B=function(){if(this instanceof B)throw TypeError(\"Symbol is not a constructor\");var t=arguments.length&&void 0!==arguments[0]?String(arguments[0]):void 0,r=z(t),e=function(t){this===U&&e.call(Z,t),f(this,D)&&f(this[D],r)&&(this[D][r]=!1),nt(this,r,y(1,t))};return s&&ot&&nt(U,r,{configurable:!0,set:e}),at(r,t)}).prototype,\"toString\",(function(){return J(this).tag})),P(B,\"withoutSetter\",(function(t){return at(z(t),t)})),j.f=ut,E.f=st,S.f=ct,w.f=x.f=ft,O.f=pt,M.f=function(t){return at(H(t),t)},s&&(V(B.prototype,\"description\",{configurable:!0,get:function(){return J(this).description}}),i||P(U,\"propertyIsEnumerable\",ut,{unsafe:!0}))),o({global:!0,wrap:!0,forced:!l,sham:!l},{Symbol:B}),C(m(rt),(function(t){I(t)})),o({target:G,stat:!0,forced:!l},{for:function(t){var r=String(t);if(f($,r))return $[r];var e=B(r);return $[r]=e,tt[e]=r,e},keyFor:function(t){if(!it(t))throw TypeError(t+\" is not a symbol\");if(f(tt,t))return tt[t]},useSetter:function(){ot=!0},useSimple:function(){ot=!1}}),o({target:\"Object\",stat:!0,forced:!l,sham:!s},{create:function(t,r){return void 0===r?T(t):lt(T(t),r)},defineProperty:st,defineProperties:lt,getOwnPropertyDescriptor:ct}),o({target:\"Object\",stat:!0,forced:!l},{getOwnPropertyNames:ft,getOwnPropertySymbols:pt}),o({target:\"Object\",stat:!0,forced:c((function(){O.f(1)}))},{getOwnPropertySymbols:function(t){return O.f(g(t))}}),Q&&o({target:\"JSON\",stat:!0,forced:!l||c((function(){var t=B();return\"[null]\"!=Q([t])||\"{}\"!=Q({a:t})||\"{}\"!=Q(Object(t))}))},{stringify:function(t,r,e){for(var o,n=[t],a=1;arguments.length>a;)n.push(arguments[a++]);if(o=r,(b(r)||void 0!==t)&&!it(t))return p(r)||(r=function(t,r){if(\"function\"==typeof o&&(r=o.call(this,t,r)),!it(r))return r}),n[1]=r,Q.apply(null,n)}}),B.prototype[W]||q(B.prototype,W,B.prototype.valueOf),F(B,G),A[D]=!0},5915:(t,r,e)=>{e(6349)(\"matchAll\")},8394:(t,r,e)=>{e(6349)(\"match\")},1766:(t,r,e)=>{e(6349)(\"replace\")},2737:(t,r,e)=>{e(6349)(\"search\")},9911:(t,r,e)=>{e(6349)(\"species\")},4315:(t,r,e)=>{e(6349)(\"split\")},3131:(t,r,e)=>{e(6349)(\"toPrimitive\")},4714:(t,r,e)=>{e(6349)(\"toStringTag\")},659:(t,r,e)=>{e(6349)(\"unscopables\")},2547:(t,r,e)=>{var o=e(7473);t.exports=o}},r={};function e(o){var n=r[o];if(void 0!==n)return n.exports;var a=r[o]={exports:{}};return t[o](a,a.exports,e),a.exports}e.n=t=>{var r=t&&t.__esModule?()=>t.default:()=>t;return e.d(r,{a:r}),r},e.d=(t,r)=>{for(var o in r)e.o(r,o)&&!e.o(t,o)&&Object.defineProperty(t,o,{enumerable:!0,get:r[o]})},e.g=function(){if(\"object\"==typeof globalThis)return globalThis;try{return this||new Function(\"return this\")()}catch(t){if(\"object\"==typeof window)return window}}(),e.o=(t,r)=>Object.prototype.hasOwnProperty.call(t,r),(()=>{\"use strict\";var t=e(2547),r=e.n(t),o={hello:\"world\",array:[1,94,\"str\",12.321,{test:\"obj in arr\"}],newObject:{test:\"obj in obj\"}};globalThis.assert.strictEqual(globalThis.addon.Get(o,\"hello\"),\"world\"),globalThis.assert.strictEqual(globalThis.addon.GetNamed(o,\"hello\"),\"world\"),globalThis.assert(globalThis.addon.Has(o,\"hello\")),globalThis.assert(globalThis.addon.HasNamed(o,\"hello\")),globalThis.assert(globalThis.addon.Has(o,\"array\")),globalThis.assert(globalThis.addon.Has(o,\"newObject\"));var n=globalThis.addon.New();globalThis.assert(globalThis.addon.Has(n,\"test_number\")),globalThis.assert.strictEqual(n.test_number,987654321),globalThis.assert.strictEqual(n.test_string,\"test string\");var a=function(){this.foo=42,this.bar=43};a.prototype.bar=44,a.prototype.baz=45;var i=new a;globalThis.assert.strictEqual(globalThis.addon.Get(i,\"foo\"),42),globalThis.assert.strictEqual(globalThis.addon.Get(i,\"bar\"),43),globalThis.assert.strictEqual(globalThis.addon.Get(i,\"baz\"),45),globalThis.assert.strictEqual(globalThis.addon.Get(i,\"toString\"),Object.prototype.toString),[!0,!1,null,void 0,{},[],0,1,function(){}].forEach((function(t){globalThis.assert.throws((function(){globalThis.addon.HasOwn({},t)}))}));var s=function(){this.foo=42,this.bar=43,this[l]=44},l=r()(),u=r()();s.prototype.bar=45,s.prototype.baz=46,s.prototype[u]=47;var c=new s;globalThis.assert.strictEqual(globalThis.addon.HasOwn(c,\"foo\"),!0),globalThis.assert.strictEqual(globalThis.addon.HasOwn(c,\"bar\"),!0),globalThis.assert.strictEqual(globalThis.addon.HasOwn(c,l),!0),globalThis.assert.strictEqual(globalThis.addon.HasOwn(c,\"baz\"),!1),globalThis.assert.strictEqual(globalThis.addon.HasOwn(c,\"toString\"),!1),globalThis.assert.strictEqual(globalThis.addon.HasOwn(c,u),!1);var f={};globalThis.assert(globalThis.addon.Set(f,\"string\",\"value\")),globalThis.assert(globalThis.addon.SetNamed(f,\"named_string\",\"value\")),globalThis.assert(globalThis.addon.Has(f,\"string\")),globalThis.assert(globalThis.addon.HasNamed(f,\"named_string\")),globalThis.assert.strictEqual(globalThis.addon.Get(f,\"string\"),\"value\");var p={};globalThis.addon.Wrap(p),globalThis.assert(globalThis.addon.Unwrap(p));var b={};Object.setPrototypeOf(b,{protoA:!0}),globalThis.addon.Wrap(b),globalThis.assert(globalThis.addon.Unwrap(b)),globalThis.assert(b.protoA);var h=r()(),g={foo:\"bar\",[h]:\"baz\"};globalThis.assert.strictEqual(\"foo\"in g,!0),globalThis.assert.strictEqual(h in g,!0),globalThis.assert.strictEqual(\"does_not_exist\"in g,!1),globalThis.assert.strictEqual(globalThis.addon.Delete(g,\"foo\"),!0),globalThis.assert.strictEqual(\"foo\"in g,!1),globalThis.assert.strictEqual(h in g,!0),globalThis.assert.strictEqual(\"does_not_exist\"in g,!1),globalThis.assert.strictEqual(\"foo\"in g,!1),globalThis.assert.strictEqual(\"does_not_exist\"in g,!1);var d={};Object.defineProperty(d,\"foo\",{configurable:!1}),globalThis.assert.strictEqual(globalThis.addon.Delete(d,\"foo\"),!1),globalThis.assert.strictEqual(\"foo\"in d,!0);var v=function(){this.foo=\"bar\"};v.prototype.foo=\"baz\";var y=new v;globalThis.assert.strictEqual(y.foo,\"bar\"),globalThis.assert.strictEqual(globalThis.addon.Delete(y,\"foo\"),!0),globalThis.assert.strictEqual(y.foo,\"baz\"),globalThis.assert.strictEqual(globalThis.addon.Delete(y,\"foo\"),!0),globalThis.assert.strictEqual(y.foo,\"baz\")})()})();",
                                         "https://www.didi.com/test_object.js",
                                         &result), NAPIOK);
}