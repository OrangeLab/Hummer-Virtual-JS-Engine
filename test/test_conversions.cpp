#include <js_native_api_test.h>
#include <common.h>

EXTERN_C_START

#define GEN_NULL_CHECK_BINDING(binding_name, output_type, api)            \
  static NAPIValue binding_name(NAPIEnv env, NAPICallbackInfo /*info*/) { \
    NAPIValue return_value;                                              \
    output_type result;                                                   \
    NAPI_CALL(env, napi_create_object(env, &return_value));               \
    add_returned_status(env,                                              \
                        "envIsNull",                                      \
                        return_value,                                     \
                        "Invalid argument",                               \
                        NAPIInvalidArg,                                 \
                        api(nullptr, return_value, &result));                \
    api(env, getUndefined(env), &result);                                              \
    add_last_status(env, "valueIsNull", return_value);                    \
    api(env, return_value, nullptr);                                         \
    add_last_status(env, "resultIsNull", return_value);                   \
    api(env, return_value, &result);                                      \
    add_last_status(env, "inputTypeCheck", return_value);                 \
    return return_value;                                                  \
  }

GEN_NULL_CHECK_BINDING(GetValueBool, bool, napi_get_value_bool)
GEN_NULL_CHECK_BINDING(GetValueInt32, int32_t, napi_get_value_int32)
GEN_NULL_CHECK_BINDING(GetValueUint32, uint32_t, napi_get_value_uint32)
GEN_NULL_CHECK_BINDING(GetValueInt64, int64_t, napi_get_value_int64)
GEN_NULL_CHECK_BINDING(GetValueDouble, double, napi_get_value_double)
GEN_NULL_CHECK_BINDING(CoerceToBool, NAPIValue, napi_coerce_to_bool)
GEN_NULL_CHECK_BINDING(CoerceToObject, NAPIValue, napi_coerce_to_object)
GEN_NULL_CHECK_BINDING(CoerceToString, NAPIValue, napi_coerce_to_string)

#define GEN_NULL_CHECK_STRING_BINDING(binding_name, arg_type, api)         \
  static NAPIValue binding_name(NAPIEnv env, NAPICallbackInfo /*info*/) {  \
    NAPIValue return_value;                                               \
    NAPI_CALL(env, napi_create_object(env, &return_value));                \
    arg_type buf1[4];                                                      \
    size_t length1 = 3;                                                    \
    add_returned_status(env,                                               \
                        "envIsNull",                                       \
                        return_value,                                      \
                        "Invalid argument",                                \
                        NAPIInvalidArg,                                  \
                        api(nullptr, return_value, buf1, length1, &length1)); \
    arg_type buf2[4];                                                      \
    size_t length2 = 3;                                                    \
    api(env, getUndefined(env), buf2, length2, &length2);                               \
    add_last_status(env, "valueIsNull", return_value);                     \
    api(env, return_value, nullptr, 3, nullptr);                                 \
    add_last_status(env, "wrongTypeIn", return_value);                     \
    NAPIValue string;                                                     \
    NAPI_CALL(env,                                                         \
              napi_create_string_utf8(env,                                 \
                                      "Something",                         \
                                      NAPI_AUTO_LENGTH,                    \
                                      &string));                           \
    api(env, string, nullptr, 3, nullptr);                                       \
    add_last_status(env, "bufAndOutLengthIsNull", return_value);           \
    return return_value;                                                   \
  }

GEN_NULL_CHECK_STRING_BINDING(GetValueStringUtf8,
                              char,
                              napi_get_value_string_utf8)
//GEN_NULL_CHECK_STRING_BINDING(GetValueStringLatin1,
//                              char,
//                              napi_get_value_string_latin1)
GEN_NULL_CHECK_STRING_BINDING(GetValueStringUtf16,
                              char16_t,
                              napi_get_value_string_utf16)

static void init_test_null(NAPIEnv
env,
NAPIValue exports
) {
NAPIValue test_null;

const NAPIPropertyDescriptor test_null_props[] = {
        DECLARE_NAPI_PROPERTY("getValueBool", GetValueBool),
        DECLARE_NAPI_PROPERTY("getValueInt32", GetValueInt32),
        DECLARE_NAPI_PROPERTY("getValueUint32", GetValueUint32),
        DECLARE_NAPI_PROPERTY("getValueInt64", GetValueInt64),
        DECLARE_NAPI_PROPERTY("getValueDouble", GetValueDouble),
        DECLARE_NAPI_PROPERTY("coerceToBool", CoerceToBool),
        DECLARE_NAPI_PROPERTY("coerceToObject", CoerceToObject),
        DECLARE_NAPI_PROPERTY("coerceToString", CoerceToString),
        DECLARE_NAPI_PROPERTY("getValueStringUtf8", GetValueStringUtf8),
//            DECLARE_NAPI_PROPERTY("getValueStringLatin1", GetValueStringLatin1),
        DECLARE_NAPI_PROPERTY("getValueStringUtf16", GetValueStringUtf16),
};

NAPI_CALL_RETURN_VOID(env, napi_create_object(env, &test_null)
);
NAPI_CALL_RETURN_VOID(env, napi_define_properties(
        env, test_null, sizeof(test_null_props) / sizeof(*test_null_props),
        test_null_props)
);

const NAPIPropertyDescriptor test_null_set = {
        "testNull", getUndefined(env), nullptr, nullptr, nullptr, test_null, NAPIEnumerable, nullptr
};

NAPI_CALL_RETURN_VOID(env,
        napi_define_properties(env, exports, 1, &test_null_set)
);
}

static NAPIValue AsBool(NAPIEnv
env,
NAPICallbackInfo info
) {
size_t argc = 1;
NAPIValue args[1];
NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr)
);

bool value;
NAPI_CALL(env, napi_get_value_bool(env, args[0], &value)
);

NAPIValue output;
NAPI_CALL(env, napi_get_boolean(env, value, &output)
);

return
output;
}

static NAPIValue AsInt32(NAPIEnv
env,
NAPICallbackInfo info
) {
size_t argc = 1;
NAPIValue args[1];
NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr)
);

int32_t value;
NAPI_CALL(env, napi_get_value_int32(env, args[0], &value)
);

NAPIValue output;
NAPI_CALL(env, napi_create_int32(env, value, &output)
);

return
output;
}

static NAPIValue AsUInt32(NAPIEnv
env,
NAPICallbackInfo info
) {
size_t argc = 1;
NAPIValue args[1];
NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr)
);

uint32_t value;
NAPI_CALL(env, napi_get_value_uint32(env, args[0], &value)
);

NAPIValue output;
NAPI_CALL(env, napi_create_uint32(env, value, &output)
);

return
output;
}

static NAPIValue AsInt64(NAPIEnv
env,
NAPICallbackInfo info
) {
size_t argc = 1;
NAPIValue args[1];
NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr)
);

int64_t value;
NAPI_CALL(env, napi_get_value_int64(env, args[0], &value)
);

NAPIValue output;
NAPI_CALL(env, napi_create_int64(env, (double) value, &output)
);

return
output;
}

static NAPIValue AsDouble(NAPIEnv
env,
NAPICallbackInfo info
) {
size_t argc = 1;
NAPIValue args[1];
NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr)
);

double value;
NAPI_CALL(env, napi_get_value_double(env, args[0], &value)
);

NAPIValue output;
NAPI_CALL(env, napi_create_double(env, value, &output)
);

return
output;
}

static NAPIValue AsString(NAPIEnv
env,
NAPICallbackInfo info
) {
size_t argc = 1;
NAPIValue args[1];
NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr)
);

char value[100];
NAPI_CALL(env,
        napi_get_value_string_utf8(env, args[0], value, sizeof(value), nullptr)
);

NAPIValue output;
NAPI_CALL(env, napi_create_string_utf8(
        env, value, NAPI_AUTO_LENGTH, &output)
);

return
output;
}

static NAPIValue ToBool(NAPIEnv
env,
NAPICallbackInfo info
) {
size_t argc = 1;
NAPIValue args[1];
NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr)
);

NAPIValue output;
NAPI_CALL(env, napi_coerce_to_bool(env, args[0], &output)
);

return
output;
}

static NAPIValue ToNumber(NAPIEnv
env,
NAPICallbackInfo info
) {
size_t argc = 1;
NAPIValue args[1];
NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr)
);

NAPIValue output;
NAPI_CALL(env, napi_coerce_to_number(env, args[0], &output)
);

return
output;
}

static NAPIValue ToObject(NAPIEnv
env,
NAPICallbackInfo info
) {
size_t argc = 1;
NAPIValue args[1];
NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr)
);

NAPIValue output;
NAPI_CALL(env, napi_coerce_to_object(env, args[0], &output)
);

return
output;
}

static NAPIValue ToString(NAPIEnv
env,
NAPICallbackInfo info
) {
size_t argc = 1;
NAPIValue args[1];
NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr)
);

NAPIValue output;
NAPI_CALL(env, napi_coerce_to_string(env, args[0], &output)
);

return
output;
}

EXTERN_C_END

TEST_F(Test, TestConversions) {
    NAPIPropertyDescriptor descriptors[] = {
            DECLARE_NAPI_PROPERTY("asBool", AsBool),
            DECLARE_NAPI_PROPERTY("asInt32", AsInt32),
            DECLARE_NAPI_PROPERTY("asUInt32", AsUInt32),
            DECLARE_NAPI_PROPERTY("asInt64", AsInt64),
            DECLARE_NAPI_PROPERTY("asDouble", AsDouble),
            DECLARE_NAPI_PROPERTY("asString", AsString),
            DECLARE_NAPI_PROPERTY("toBool", ToBool),
            DECLARE_NAPI_PROPERTY("toNumber", ToNumber),
            DECLARE_NAPI_PROPERTY("toObject", ToObject),
            DECLARE_NAPI_PROPERTY("toString", ToString),
    };
    ASSERT_EQ(
            napi_define_properties(globalEnv, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors),
            NAPIOK);
    init_test_null(globalEnv, exports);

    NAPIValue result;
    ASSERT_EQ(
            NAPIRunScriptWithSourceUrl(globalEnv,
                                       "(()=>{var t={7473:(t,r,o)=>{o(8922),o(5967),o(5824),o(8555),o(2615),o(1732),o(5903),o(1825),o(8394),o(5915),o(1766),o(2737),o(9911),o(4315),o(3131),o(4714),o(659),o(9120),o(5327),o(1502);var n=o(4058);t.exports=n.Symbol},3916:t=>{t.exports=function(t){if(\"function\"!=typeof t)throw TypeError(String(t)+\" is not a function\");return t}},6059:(t,r,o)=>{var n=o(941);t.exports=function(t){if(!n(t))throw TypeError(String(t)+\" is not an object\");return t}},1692:(t,r,o)=>{var n=o(4529),e=o(3057),a=o(9413),s=function(t){return function(r,o,s){var i,l=n(r),u=e(l.length),c=a(s,u);if(t&&o!=o){for(;u>c;)if((i=l[c++])!=i)return!0}else for(;u>c;c++)if((t||c in l)&&l[c]===o)return t||c||0;return!t&&-1}};t.exports={includes:s(!0),indexOf:s(!1)}},3610:(t,r,o)=>{var n=o(6843),e=o(7026),a=o(9678),s=o(3057),i=o(4692),l=[].push,u=function(t){var r=1==t,o=2==t,u=3==t,c=4==t,h=6==t,b=7==t,f=5==t||h;return function(g,d,p,T){for(var v,y,m=a(g),w=e(m),E=n(d,p,3),S=s(w.length),x=0,N=T||i,O=r?N(g,S):o||b?N(g,0):void 0;S>x;x++)if((f||x in w)&&(y=E(v=w[x],x,m),t))if(r)O[x]=y;else if(y)switch(t){case 3:return!0;case 5:return v;case 6:return x;case 2:l.call(O,v)}else switch(t){case 4:return!1;case 7:l.call(O,v)}return h?-1:u||c?c:O}};t.exports={forEach:u(0),map:u(1),filter:u(2),some:u(3),every:u(4),find:u(5),findIndex:u(6),filterOut:u(7)}},568:(t,r,o)=>{var n=o(5981),e=o(9813),a=o(3385),s=e(\"species\");t.exports=function(t){return a>=51||!n((function(){var r=[];return(r.constructor={})[s]=function(){return{foo:1}},1!==r[t](Boolean).foo}))}},4692:(t,r,o)=>{var n=o(941),e=o(1052),a=o(9813)(\"species\");t.exports=function(t,r){var o;return e(t)&&(\"function\"!=typeof(o=t.constructor)||o!==Array&&!e(o.prototype)?n(o)&&null===(o=o[a])&&(o=void 0):o=void 0),new(void 0===o?Array:o)(0===r?0:r)}},2532:t=>{var r={}.toString;t.exports=function(t){return r.call(t).slice(8,-1)}},9697:(t,r,o)=>{var n=o(2885),e=o(2532),a=o(9813)(\"toStringTag\"),s=\"Arguments\"==e(function(){return arguments}());t.exports=n?e:function(t){var r,o,n;return void 0===t?\"Undefined\":null===t?\"Null\":\"string\"==typeof(o=function(t,r){try{return t[r]}catch(t){}}(r=Object(t),a))?o:s?e(r):\"Object\"==(n=e(r))&&\"function\"==typeof r.callee?\"Arguments\":n}},2029:(t,r,o)=>{var n=o(5746),e=o(5988),a=o(1887);t.exports=n?function(t,r,o){return e.f(t,r,a(1,o))}:function(t,r,o){return t[r]=o,t}},1887:t=>{t.exports=function(t,r){return{enumerable:!(1&t),configurable:!(2&t),writable:!(4&t),value:r}}},5449:(t,r,o)=>{\"use strict\";var n=o(6935),e=o(5988),a=o(1887);t.exports=function(t,r,o){var s=n(r);s in t?e.f(t,s,a(0,o)):t[s]=o}},6349:(t,r,o)=>{var n=o(4058),e=o(7457),a=o(1477),s=o(5988).f;t.exports=function(t){var r=n.Symbol||(n.Symbol={});e(r,t)||s(r,t,{value:a.f(t)})}},5746:(t,r,o)=>{var n=o(5981);t.exports=!n((function(){return 7!=Object.defineProperty({},1,{get:function(){return 7}})[1]}))},1333:(t,r,o)=>{var n=o(1899),e=o(941),a=n.document,s=e(a)&&e(a.createElement);t.exports=function(t){return s?a.createElement(t):{}}},6049:(t,r,o)=>{var n=o(2532),e=o(1899);t.exports=\"process\"==n(e.process)},2861:(t,r,o)=>{var n=o(626);t.exports=n(\"navigator\",\"userAgent\")||\"\"},3385:(t,r,o)=>{var n,e,a=o(1899),s=o(2861),i=a.process,l=i&&i.versions,u=l&&l.v8;u?e=(n=u.split(\".\"))[0]+n[1]:s&&(!(n=s.match(/Edge\\/(\\d+)/))||n[1]>=74)&&(n=s.match(/Chrome\\/(\\d+)/))&&(e=n[1]),t.exports=e&&+e},6759:t=>{t.exports=[\"constructor\",\"hasOwnProperty\",\"isPrototypeOf\",\"propertyIsEnumerable\",\"toLocaleString\",\"toString\",\"valueOf\"]},6887:(t,r,o)=>{\"use strict\";var n=o(1899),e=o(9677).f,a=o(7252),s=o(4058),i=o(6843),l=o(2029),u=o(7457),c=function(t){var r=function(r,o,n){if(this instanceof t){switch(arguments.length){case 0:return new t;case 1:return new t(r);case 2:return new t(r,o)}return new t(r,o,n)}return t.apply(this,arguments)};return r.prototype=t.prototype,r};t.exports=function(t,r){var o,h,b,f,g,d,p,T,v=t.target,y=t.global,m=t.stat,w=t.proto,E=y?n:m?n[v]:(n[v]||{}).prototype,S=y?s:s[v]||(s[v]={}),x=S.prototype;for(b in r)o=!a(y?b:v+(m?\".\":\"#\")+b,t.forced)&&E&&u(E,b),g=S[b],o&&(d=t.noTargetGet?(T=e(E,b))&&T.value:E[b]),f=o&&d?d:r[b],o&&typeof g==typeof f||(p=t.bind&&o?i(f,n):t.wrap&&o?c(f):w&&\"function\"==typeof f?i(Function.call,f):f,(t.sham||f&&f.sham||g&&g.sham)&&l(p,\"sham\",!0),S[b]=p,w&&(u(s,h=v+\"Prototype\")||l(s,h,{}),s[h][b]=f,t.real&&x&&!x[b]&&l(x,b,f)))}},5981:t=>{t.exports=function(t){try{return!!t()}catch(t){return!0}}},6843:(t,r,o)=>{var n=o(3916);t.exports=function(t,r,o){if(n(t),void 0===r)return t;switch(o){case 0:return function(){return t.call(r)};case 1:return function(o){return t.call(r,o)};case 2:return function(o,n){return t.call(r,o,n)};case 3:return function(o,n,e){return t.call(r,o,n,e)}}return function(){return t.apply(r,arguments)}}},626:(t,r,o)=>{var n=o(4058),e=o(1899),a=function(t){return\"function\"==typeof t?t:void 0};t.exports=function(t,r){return arguments.length<2?a(n[t])||a(e[t]):n[t]&&n[t][r]||e[t]&&e[t][r]}},1899:(t,r,o)=>{var n=function(t){return t&&t.Math==Math&&t};t.exports=n(\"object\"==typeof globalThis&&globalThis)||n(\"object\"==typeof window&&window)||n(\"object\"==typeof self&&self)||n(\"object\"==typeof o.g&&o.g)||function(){return this}()||Function(\"return this\")()},7457:(t,r,o)=>{var n=o(9678),e={}.hasOwnProperty;t.exports=function(t,r){return e.call(n(t),r)}},7748:t=>{t.exports={}},5463:(t,r,o)=>{var n=o(626);t.exports=n(\"document\",\"documentElement\")},2840:(t,r,o)=>{var n=o(5746),e=o(5981),a=o(1333);t.exports=!n&&!e((function(){return 7!=Object.defineProperty(a(\"div\"),\"a\",{get:function(){return 7}}).a}))},7026:(t,r,o)=>{var n=o(5981),e=o(2532),a=\"\".split;t.exports=n((function(){return!Object(\"z\").propertyIsEnumerable(0)}))?function(t){return\"String\"==e(t)?a.call(t,\"\"):Object(t)}:Object},1302:(t,r,o)=>{var n=o(3030),e=Function.toString;\"function\"!=typeof n.inspectSource&&(n.inspectSource=function(t){return e.call(t)}),t.exports=n.inspectSource},5402:(t,r,o)=>{var n,e,a,s=o(8019),i=o(1899),l=o(941),u=o(2029),c=o(7457),h=o(3030),b=o(4262),f=o(7748),g=\"Object already initialized\",d=i.WeakMap;if(s){var p=h.state||(h.state=new d),T=p.get,v=p.has,y=p.set;n=function(t,r){if(v.call(p,t))throw new TypeError(g);return r.facade=t,y.call(p,t,r),r},e=function(t){return T.call(p,t)||{}},a=function(t){return v.call(p,t)}}else{var m=b(\"state\");f[m]=!0,n=function(t,r){if(c(t,m))throw new TypeError(g);return r.facade=t,u(t,m,r),r},e=function(t){return c(t,m)?t[m]:{}},a=function(t){return c(t,m)}}t.exports={set:n,get:e,has:a,enforce:function(t){return a(t)?e(t):n(t,{})},getterFor:function(t){return function(r){var o;if(!l(r)||(o=e(r)).type!==t)throw TypeError(\"Incompatible receiver, \"+t+\" required\");return o}}}},1052:(t,r,o)=>{var n=o(2532);t.exports=Array.isArray||function(t){return\"Array\"==n(t)}},7252:(t,r,o)=>{var n=o(5981),e=/#|\\.prototype\\./,a=function(t,r){var o=i[s(t)];return o==u||o!=l&&(\"function\"==typeof r?n(r):!!r)},s=a.normalize=function(t){return String(t).replace(e,\".\").toLowerCase()},i=a.data={},l=a.NATIVE=\"N\",u=a.POLYFILL=\"P\";t.exports=a},941:t=>{t.exports=function(t){return\"object\"==typeof t?null!==t:\"function\"==typeof t}},2529:t=>{t.exports=!0},2497:(t,r,o)=>{var n=o(6049),e=o(3385),a=o(5981);t.exports=!!Object.getOwnPropertySymbols&&!a((function(){return!Symbol.sham&&(n?38===e:e>37&&e<41)}))},8019:(t,r,o)=>{var n=o(1899),e=o(1302),a=n.WeakMap;t.exports=\"function\"==typeof a&&/native code/.test(e(a))},9290:(t,r,o)=>{var n,e=o(6059),a=o(9938),s=o(6759),i=o(7748),l=o(5463),u=o(1333),c=o(4262)(\"IE_PROTO\"),h=function(){},b=function(t){return\"<script>\"+t+\"<\\/script>\"},f=function(){try{n=document.domain&&new ActiveXObject(\"htmlfile\")}catch(t){}var t,r;f=n?function(t){t.write(b(\"\")),t.close();var r=t.parentWindow.Object;return t=null,r}(n):((r=u(\"iframe\")).style.display=\"none\",l.appendChild(r),r.src=String(\"javascript:\"),(t=r.contentWindow.document).open(),t.write(b(\"document.F=Object\")),t.close(),t.F);for(var o=s.length;o--;)delete f.prototype[s[o]];return f()};i[c]=!0,t.exports=Object.create||function(t,r){var o;return null!==t?(h.prototype=e(t),o=new h,h.prototype=null,o[c]=t):o=f(),void 0===r?o:a(o,r)}},9938:(t,r,o)=>{var n=o(5746),e=o(5988),a=o(6059),s=o(4771);t.exports=n?Object.defineProperties:function(t,r){a(t);for(var o,n=s(r),i=n.length,l=0;i>l;)e.f(t,o=n[l++],r[o]);return t}},5988:(t,r,o)=>{var n=o(5746),e=o(2840),a=o(6059),s=o(6935),i=Object.defineProperty;r.f=n?i:function(t,r,o){if(a(t),r=s(r,!0),a(o),e)try{return i(t,r,o)}catch(t){}if(\"get\"in o||\"set\"in o)throw TypeError(\"Accessors not supported\");return\"value\"in o&&(t[r]=o.value),t}},9677:(t,r,o)=>{var n=o(5746),e=o(6760),a=o(1887),s=o(4529),i=o(6935),l=o(7457),u=o(2840),c=Object.getOwnPropertyDescriptor;r.f=n?c:function(t,r){if(t=s(t),r=i(r,!0),u)try{return c(t,r)}catch(t){}if(l(t,r))return a(!e.f.call(t,r),t[r])}},684:(t,r,o)=>{var n=o(4529),e=o(946).f,a={}.toString,s=\"object\"==typeof window&&window&&Object.getOwnPropertyNames?Object.getOwnPropertyNames(window):[];t.exports.f=function(t){return s&&\"[object Window]\"==a.call(t)?function(t){try{return e(t)}catch(t){return s.slice()}}(t):e(n(t))}},946:(t,r,o)=>{var n=o(5629),e=o(6759).concat(\"length\",\"prototype\");r.f=Object.getOwnPropertyNames||function(t){return n(t,e)}},7857:(t,r)=>{r.f=Object.getOwnPropertySymbols},5629:(t,r,o)=>{var n=o(7457),e=o(4529),a=o(1692).indexOf,s=o(7748);t.exports=function(t,r){var o,i=e(t),l=0,u=[];for(o in i)!n(s,o)&&n(i,o)&&u.push(o);for(;r.length>l;)n(i,o=r[l++])&&(~a(u,o)||u.push(o));return u}},4771:(t,r,o)=>{var n=o(5629),e=o(6759);t.exports=Object.keys||function(t){return n(t,e)}},6760:(t,r)=>{\"use strict\";var o={}.propertyIsEnumerable,n=Object.getOwnPropertyDescriptor,e=n&&!o.call({1:2},1);r.f=e?function(t){var r=n(this,t);return!!r&&r.enumerable}:o},5623:(t,r,o)=>{\"use strict\";var n=o(2885),e=o(9697);t.exports=n?{}.toString:function(){return\"[object \"+e(this)+\"]\"}},4058:t=>{t.exports={}},9754:(t,r,o)=>{var n=o(2029);t.exports=function(t,r,o,e){e&&e.enumerable?t[r]=o:n(t,r,o)}},8219:t=>{t.exports=function(t){if(null==t)throw TypeError(\"Can't call method on \"+t);return t}},4911:(t,r,o)=>{var n=o(1899),e=o(2029);t.exports=function(t,r){try{e(n,t,r)}catch(o){n[t]=r}return r}},904:(t,r,o)=>{var n=o(2885),e=o(5988).f,a=o(2029),s=o(7457),i=o(5623),l=o(9813)(\"toStringTag\");t.exports=function(t,r,o,u){if(t){var c=o?t:t.prototype;s(c,l)||e(c,l,{configurable:!0,value:r}),u&&!n&&a(c,\"toString\",i)}}},4262:(t,r,o)=>{var n=o(8726),e=o(9418),a=n(\"keys\");t.exports=function(t){return a[t]||(a[t]=e(t))}},3030:(t,r,o)=>{var n=o(1899),e=o(4911),a=\"__core-js_shared__\",s=n[a]||e(a,{});t.exports=s},8726:(t,r,o)=>{var n=o(2529),e=o(3030);(t.exports=function(t,r){return e[t]||(e[t]=void 0!==r?r:{})})(\"versions\",[]).push({version:\"3.11.1\",mode:n?\"pure\":\"global\",copyright:\"© 2021 Denis Pushkarev (zloirock.ru)\"})},9413:(t,r,o)=>{var n=o(8459),e=Math.max,a=Math.min;t.exports=function(t,r){var o=n(t);return o<0?e(o+r,0):a(o,r)}},4529:(t,r,o)=>{var n=o(7026),e=o(8219);t.exports=function(t){return n(e(t))}},8459:t=>{var r=Math.ceil,o=Math.floor;t.exports=function(t){return isNaN(t=+t)?0:(t>0?o:r)(t)}},3057:(t,r,o)=>{var n=o(8459),e=Math.min;t.exports=function(t){return t>0?e(n(t),9007199254740991):0}},9678:(t,r,o)=>{var n=o(8219);t.exports=function(t){return Object(n(t))}},6935:(t,r,o)=>{var n=o(941);t.exports=function(t,r){if(!n(t))return t;var o,e;if(r&&\"function\"==typeof(o=t.toString)&&!n(e=o.call(t)))return e;if(\"function\"==typeof(o=t.valueOf)&&!n(e=o.call(t)))return e;if(!r&&\"function\"==typeof(o=t.toString)&&!n(e=o.call(t)))return e;throw TypeError(\"Can't convert object to primitive value\")}},2885:(t,r,o)=>{var n={};n[o(9813)(\"toStringTag\")]=\"z\",t.exports=\"[object z]\"===String(n)},9418:t=>{var r=0,o=Math.random();t.exports=function(t){return\"Symbol(\"+String(void 0===t?\"\":t)+\")_\"+(++r+o).toString(36)}},2302:(t,r,o)=>{var n=o(2497);t.exports=n&&!Symbol.sham&&\"symbol\"==typeof Symbol.iterator},1477:(t,r,o)=>{var n=o(9813);r.f=n},9813:(t,r,o)=>{var n=o(1899),e=o(8726),a=o(7457),s=o(9418),i=o(2497),l=o(2302),u=e(\"wks\"),c=n.Symbol,h=l?c:c&&c.withoutSetter||s;t.exports=function(t){return a(u,t)&&(i||\"string\"==typeof u[t])||(i&&a(c,t)?u[t]=c[t]:u[t]=h(\"Symbol.\"+t)),u[t]}},8922:(t,r,o)=>{\"use strict\";var n=o(6887),e=o(5981),a=o(1052),s=o(941),i=o(9678),l=o(3057),u=o(5449),c=o(4692),h=o(568),b=o(9813),f=o(3385),g=b(\"isConcatSpreadable\"),d=9007199254740991,p=\"Maximum allowed index exceeded\",T=f>=51||!e((function(){var t=[];return t[g]=!1,t.concat()[0]!==t})),v=h(\"concat\"),y=function(t){if(!s(t))return!1;var r=t[g];return void 0!==r?!!r:a(t)};n({target:\"Array\",proto:!0,forced:!T||!v},{concat:function(t){var r,o,n,e,a,s=i(this),h=c(s,0),b=0;for(r=-1,n=arguments.length;r<n;r++)if(y(a=-1===r?s:arguments[r])){if(b+(e=l(a.length))>d)throw TypeError(p);for(o=0;o<e;o++,b++)o in a&&u(h,b,a[o])}else{if(b>=d)throw TypeError(p);u(h,b++,a)}return h.length=b,h}})},9120:(t,r,o)=>{var n=o(1899);o(904)(n.JSON,\"JSON\",!0)},5327:()=>{},5967:()=>{},1502:()=>{},8555:(t,r,o)=>{o(6349)(\"asyncIterator\")},2615:()=>{},1732:(t,r,o)=>{o(6349)(\"hasInstance\")},5903:(t,r,o)=>{o(6349)(\"isConcatSpreadable\")},1825:(t,r,o)=>{o(6349)(\"iterator\")},5824:(t,r,o)=>{\"use strict\";var n=o(6887),e=o(1899),a=o(626),s=o(2529),i=o(5746),l=o(2497),u=o(2302),c=o(5981),h=o(7457),b=o(1052),f=o(941),g=o(6059),d=o(9678),p=o(4529),T=o(6935),v=o(1887),y=o(9290),m=o(4771),w=o(946),E=o(684),S=o(7857),x=o(9677),N=o(5988),O=o(6760),q=o(2029),j=o(9754),B=o(8726),P=o(4262),D=o(7748),I=o(9418),k=o(9813),A=o(1477),M=o(6349),F=o(904),_=o(5402),C=o(3610).forEach,z=P(\"hidden\"),L=\"Symbol\",W=k(\"toPrimitive\"),J=_.set,U=_.getterFor(L),G=Object.prototype,Q=e.Symbol,R=a(\"JSON\",\"stringify\"),V=x.f,X=N.f,Y=E.f,H=O.f,K=B(\"symbols\"),Z=B(\"op-symbols\"),$=B(\"string-to-symbol-registry\"),tt=B(\"symbol-to-string-registry\"),rt=B(\"wks\"),ot=e.QObject,nt=!ot||!ot.prototype||!ot.prototype.findChild,et=i&&c((function(){return 7!=y(X({},\"a\",{get:function(){return X(this,\"a\",{value:7}).a}})).a}))?function(t,r,o){var n=V(G,r);n&&delete G[r],X(t,r,o),n&&t!==G&&X(G,r,n)}:X,at=function(t,r){var o=K[t]=y(Q.prototype);return J(o,{type:L,tag:t,description:r}),i||(o.description=r),o},st=u?function(t){return\"symbol\"==typeof t}:function(t){return Object(t)instanceof Q},it=function(t,r,o){t===G&&it(Z,r,o),g(t);var n=T(r,!0);return g(o),h(K,n)?(o.enumerable?(h(t,z)&&t[z][n]&&(t[z][n]=!1),o=y(o,{enumerable:v(0,!1)})):(h(t,z)||X(t,z,v(1,{})),t[z][n]=!0),et(t,n,o)):X(t,n,o)},lt=function(t,r){g(t);var o=p(r),n=m(o).concat(bt(o));return C(n,(function(r){i&&!ut.call(o,r)||it(t,r,o[r])})),t},ut=function(t){var r=T(t,!0),o=H.call(this,r);return!(this===G&&h(K,r)&&!h(Z,r))&&(!(o||!h(this,r)||!h(K,r)||h(this,z)&&this[z][r])||o)},ct=function(t,r){var o=p(t),n=T(r,!0);if(o!==G||!h(K,n)||h(Z,n)){var e=V(o,n);return!e||!h(K,n)||h(o,z)&&o[z][n]||(e.enumerable=!0),e}},ht=function(t){var r=Y(p(t)),o=[];return C(r,(function(t){h(K,t)||h(D,t)||o.push(t)})),o},bt=function(t){var r=t===G,o=Y(r?Z:p(t)),n=[];return C(o,(function(t){!h(K,t)||r&&!h(G,t)||n.push(K[t])})),n};l||(j((Q=function(){if(this instanceof Q)throw TypeError(\"Symbol is not a constructor\");var t=arguments.length&&void 0!==arguments[0]?String(arguments[0]):void 0,r=I(t),o=function(t){this===G&&o.call(Z,t),h(this,z)&&h(this[z],r)&&(this[z][r]=!1),et(this,r,v(1,t))};return i&&nt&&et(G,r,{configurable:!0,set:o}),at(r,t)}).prototype,\"toString\",(function(){return U(this).tag})),j(Q,\"withoutSetter\",(function(t){return at(I(t),t)})),O.f=ut,N.f=it,x.f=ct,w.f=E.f=ht,S.f=bt,A.f=function(t){return at(k(t),t)},i&&(X(Q.prototype,\"description\",{configurable:!0,get:function(){return U(this).description}}),s||j(G,\"propertyIsEnumerable\",ut,{unsafe:!0}))),n({global:!0,wrap:!0,forced:!l,sham:!l},{Symbol:Q}),C(m(rt),(function(t){M(t)})),n({target:L,stat:!0,forced:!l},{for:function(t){var r=String(t);if(h($,r))return $[r];var o=Q(r);return $[r]=o,tt[o]=r,o},keyFor:function(t){if(!st(t))throw TypeError(t+\" is not a symbol\");if(h(tt,t))return tt[t]},useSetter:function(){nt=!0},useSimple:function(){nt=!1}}),n({target:\"Object\",stat:!0,forced:!l,sham:!i},{create:function(t,r){return void 0===r?y(t):lt(y(t),r)},defineProperty:it,defineProperties:lt,getOwnPropertyDescriptor:ct}),n({target:\"Object\",stat:!0,forced:!l},{getOwnPropertyNames:ht,getOwnPropertySymbols:bt}),n({target:\"Object\",stat:!0,forced:c((function(){S.f(1)}))},{getOwnPropertySymbols:function(t){return S.f(d(t))}}),R&&n({target:\"JSON\",stat:!0,forced:!l||c((function(){var t=Q();return\"[null]\"!=R([t])||\"{}\"!=R({a:t})||\"{}\"!=R(Object(t))}))},{stringify:function(t,r,o){for(var n,e=[t],a=1;arguments.length>a;)e.push(arguments[a++]);if(n=r,(f(r)||void 0!==t)&&!st(t))return b(r)||(r=function(t,r){if(\"function\"==typeof n&&(r=n.call(this,t,r)),!st(r))return r}),e[1]=r,R.apply(null,e)}}),Q.prototype[W]||q(Q.prototype,W,Q.prototype.valueOf),F(Q,L),D[z]=!0},5915:(t,r,o)=>{o(6349)(\"matchAll\")},8394:(t,r,o)=>{o(6349)(\"match\")},1766:(t,r,o)=>{o(6349)(\"replace\")},2737:(t,r,o)=>{o(6349)(\"search\")},9911:(t,r,o)=>{o(6349)(\"species\")},4315:(t,r,o)=>{o(6349)(\"split\")},3131:(t,r,o)=>{o(6349)(\"toPrimitive\")},4714:(t,r,o)=>{o(6349)(\"toStringTag\")},659:(t,r,o)=>{o(6349)(\"unscopables\")},2547:(t,r,o)=>{var n=o(7473);t.exports=n}},r={};function o(n){var e=r[n];if(void 0!==e)return e.exports;var a=r[n]={exports:{}};return t[n](a,a.exports,o),a.exports}o.n=t=>{var r=t&&t.__esModule?()=>t.default:()=>t;return o.d(r,{a:r}),r},o.d=(t,r)=>{for(var n in r)o.o(r,n)&&!o.o(t,n)&&Object.defineProperty(t,n,{enumerable:!0,get:r[n]})},o.g=function(){if(\"object\"==typeof globalThis)return globalThis;try{return this||new Function(\"return this\")()}catch(t){if(\"object\"==typeof window)return window}}(),o.o=(t,r)=>Object.prototype.hasOwnProperty.call(t,r),(()=>{\"use strict\";var t=o(2547),r=o.n(t)()(\"test\");globalThis.assert.strictEqual(globalThis.addon.asBool(!1),!1),globalThis.assert.strictEqual(globalThis.addon.asBool(!0),!0),globalThis.assert.throws((function(){return globalThis.addon.asBool(void 0)})),globalThis.assert.throws((function(){return globalThis.addon.asBool(null)})),globalThis.assert.throws((function(){return globalThis.addon.asBool(Number.NaN)})),globalThis.assert.throws((function(){return globalThis.addon.asBool(0)})),globalThis.assert.throws((function(){return globalThis.addon.asBool(\"\")})),globalThis.assert.throws((function(){return globalThis.addon.asBool(\"0\")})),globalThis.assert.throws((function(){return globalThis.addon.asBool(1)})),globalThis.assert.throws((function(){return globalThis.addon.asBool(\"1\")})),globalThis.assert.throws((function(){return globalThis.addon.asBool(\"true\")})),globalThis.assert.throws((function(){return globalThis.addon.asBool({})})),globalThis.assert.throws((function(){return globalThis.addon.asBool([])})),globalThis.assert.throws((function(){return globalThis.addon.asBool(r)})),[globalThis.addon.asInt32,globalThis.addon.asUInt32,globalThis.addon.asInt64].forEach((function(t){globalThis.assert.strictEqual(t(0),0),globalThis.assert.strictEqual(t(1),1),globalThis.assert.strictEqual(t(1),1),globalThis.assert.strictEqual(t(1.1),1),globalThis.assert.strictEqual(t(1.9),1),globalThis.assert.strictEqual(t(.9),0),globalThis.assert.strictEqual(t(999.9),999),globalThis.assert.throws((function(){return t(void 0)})),globalThis.assert.throws((function(){return t(null)})),globalThis.assert.throws((function(){return t(!1)})),globalThis.assert.throws((function(){return t(\"\")})),globalThis.assert.throws((function(){return t(\"1\")})),globalThis.assert.throws((function(){return t({})})),globalThis.assert.throws((function(){return t([])})),globalThis.assert.throws((function(){return t(r)}))})),globalThis.assert.strictEqual(globalThis.addon.asInt32(-1),-1),globalThis.assert.strictEqual(globalThis.addon.asInt64(-1),-1),globalThis.assert.strictEqual(globalThis.addon.asUInt32(-1),Math.pow(2,32)-1),globalThis.assert.strictEqual(globalThis.addon.asDouble(0),0),globalThis.assert.strictEqual(globalThis.addon.asDouble(1),1),globalThis.assert.strictEqual(globalThis.addon.asDouble(1),1),globalThis.assert.strictEqual(globalThis.addon.asDouble(1.1),1.1),globalThis.assert.strictEqual(globalThis.addon.asDouble(1.9),1.9),globalThis.assert.strictEqual(globalThis.addon.asDouble(.9),.9),globalThis.assert.strictEqual(globalThis.addon.asDouble(999.9),999.9),globalThis.assert.strictEqual(globalThis.addon.asDouble(-1),-1),globalThis.assert.ok(Number.isNaN(globalThis.addon.asDouble(Number.NaN))),globalThis.assert.throws((function(){return globalThis.addon.asDouble(void 0)})),globalThis.assert.throws((function(){return globalThis.addon.asDouble(null)})),globalThis.assert.throws((function(){return globalThis.addon.asDouble(!1)})),globalThis.assert.throws((function(){return globalThis.addon.asDouble(\"\")})),globalThis.assert.throws((function(){return globalThis.addon.asDouble(\"1\")})),globalThis.assert.throws((function(){return globalThis.addon.asDouble({})})),globalThis.assert.throws((function(){return globalThis.addon.asDouble([])})),globalThis.assert.throws((function(){return globalThis.addon.asDouble(r)})),globalThis.assert.strictEqual(globalThis.addon.asString(\"\"),\"\"),globalThis.assert.strictEqual(globalThis.addon.asString(\"test\"),\"test\"),globalThis.assert.throws((function(){return globalThis.addon.asString(void 0)})),globalThis.assert.throws((function(){return globalThis.addon.asString(null)})),globalThis.assert.throws((function(){return globalThis.addon.asString(!1)})),globalThis.assert.throws((function(){return globalThis.addon.asString(1)})),globalThis.assert.throws((function(){return globalThis.addon.asString(1.1)})),globalThis.assert.throws((function(){return globalThis.addon.asString(Number.NaN)})),globalThis.assert.throws((function(){return globalThis.addon.asString({})})),globalThis.assert.throws((function(){return globalThis.addon.asString([])})),globalThis.assert.throws((function(){return globalThis.addon.asString(r)})),globalThis.assert.strictEqual(globalThis.addon.toBool(!0),!0),globalThis.assert.strictEqual(globalThis.addon.toBool(1),!0),globalThis.assert.strictEqual(globalThis.addon.toBool(-1),!0),globalThis.assert.strictEqual(globalThis.addon.toBool(\"true\"),!0),globalThis.assert.strictEqual(globalThis.addon.toBool(\"false\"),!0),globalThis.assert.strictEqual(globalThis.addon.toBool({}),!0),globalThis.assert.strictEqual(globalThis.addon.toBool([]),!0),globalThis.assert.strictEqual(globalThis.addon.toBool(r),!0),globalThis.assert.strictEqual(globalThis.addon.toBool(!1),!1),globalThis.assert.strictEqual(globalThis.addon.toBool(void 0),!1),globalThis.assert.strictEqual(globalThis.addon.toBool(null),!1),globalThis.assert.strictEqual(globalThis.addon.toBool(0),!1),globalThis.assert.strictEqual(globalThis.addon.toBool(Number.NaN),!1),globalThis.assert.strictEqual(globalThis.addon.toBool(\"\"),!1),globalThis.assert.strictEqual(globalThis.addon.toNumber(0),0),globalThis.assert.strictEqual(globalThis.addon.toNumber(1),1),globalThis.assert.strictEqual(globalThis.addon.toNumber(1.1),1.1),globalThis.assert.strictEqual(globalThis.addon.toNumber(-1),-1),globalThis.assert.strictEqual(globalThis.addon.toNumber(\"0\"),0),globalThis.assert.strictEqual(globalThis.addon.toNumber(\"1\"),1),globalThis.assert.strictEqual(globalThis.addon.toNumber(\"1.1\"),1.1),globalThis.assert.strictEqual(globalThis.addon.toNumber([]),0),globalThis.assert.strictEqual(globalThis.addon.toNumber(!1),0),globalThis.assert.strictEqual(globalThis.addon.toNumber(null),0),globalThis.assert.strictEqual(globalThis.addon.toNumber(\"\"),0),globalThis.assert.ok(Number.isNaN(globalThis.addon.toNumber(Number.NaN))),globalThis.assert.ok(Number.isNaN(globalThis.addon.toNumber({}))),globalThis.assert.ok(Number.isNaN(globalThis.addon.toNumber(void 0))),globalThis.assert.throws((function(){return globalThis.addon.toNumber(r)}),TypeError),globalThis.assert.ok(!Number.isNaN(globalThis.addon.toObject(Number.NaN))),globalThis.assert.strictEqual(globalThis.addon.toString(\"\"),\"\"),globalThis.assert.strictEqual(globalThis.addon.toString(\"test\"),\"test\"),globalThis.assert.strictEqual(globalThis.addon.toString(void 0),\"undefined\"),globalThis.assert.strictEqual(globalThis.addon.toString(null),\"null\"),globalThis.assert.strictEqual(globalThis.addon.toString(!1),\"false\"),globalThis.assert.strictEqual(globalThis.addon.toString(!0),\"true\"),globalThis.assert.strictEqual(globalThis.addon.toString(0),\"0\"),globalThis.assert.strictEqual(globalThis.addon.toString(1.1),\"1.1\"),globalThis.assert.strictEqual(globalThis.addon.toString(Number.NaN),\"NaN\"),globalThis.assert.strictEqual(globalThis.addon.toString({}),\"[object Object]\"),globalThis.assert.strictEqual(globalThis.addon.toString({toString:function(){return\"test\"}}),\"test\"),globalThis.assert.strictEqual(globalThis.addon.toString([]),\"\"),globalThis.assert.strictEqual(globalThis.addon.toString([1,2,3]),\"1,2,3\"),globalThis.assert.throws((function(){return globalThis.addon.toString(r)}))})()})();",
                                       "https://www.didi.com/test_conversions.js",
                                       &result), NAPIOK);
}
