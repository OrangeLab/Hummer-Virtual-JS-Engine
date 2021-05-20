#include <js_native_api_test.h>
#include <common.h>

EXTERN_C_START

static NAPIValue testStrictEquals(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 2;
    NAPIValue args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    bool bool_result;
    NAPIValue result;
    NAPI_CALL(env, napi_strict_equals(env, args[0], args[1], &bool_result));
    NAPI_CALL(env, napi_get_boolean(env, bool_result, &result));

    return result;
}

static NAPIValue testGetPrototype(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPIValue result;
    NAPI_CALL(env, napi_get_prototype(env, args[0], &result));

    return result;
}

static NAPIValue doInstanceOf(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 2;
    NAPIValue args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    bool instanceof;
    NAPI_CALL(env, napi_instanceof(env, args[0], args[1], &instanceof));

    NAPIValue result;
    NAPI_CALL(env, napi_get_boolean(env, instanceof, &result));

    return result;
}

static NAPIValue getNull(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue result;
    NAPI_CALL(env, napi_get_null(env, &result));
    return result;
}

static NAPIValue getUndefined(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue result;
    NAPI_CALL(env, napi_get_undefined(env, &result));
    return result;
}

static NAPIValue createNapiError(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue value;
    NAPI_CALL(env, napi_create_string_utf8(env, "xyz", -1, &value));

    double double_value;
    NAPIStatus status = napi_get_value_double(env, value, &double_value);

    NAPI_ASSERT(env, status != NAPIOK, "Failed to produce error condition");

    const NAPIExtendedErrorInfo *error_info = 0;
    NAPI_CALL(env, napi_get_last_error_info(env, &error_info));

    NAPI_ASSERT(env, error_info->errorCode == status,
                "Last error info code should match last status");
    NAPI_ASSERT(env, error_info->errorMessage,
                "Last error info message should not be null");

    return getUndefined(env);
}

static NAPIValue testNapiErrorCleanup(NAPIEnv env, NAPICallbackInfo /*info*/) {
    const NAPIExtendedErrorInfo *error_info = 0;
    NAPI_CALL(env, napi_get_last_error_info(env, &error_info));

    NAPIValue result;
    bool is_ok = error_info->errorCode == NAPIOK;
    NAPI_CALL(env, napi_get_boolean(env, is_ok, &result));

    return result;
}

static NAPIValue testNapiTypeof(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPIValueType argument_type;
    NAPI_CALL(env, napi_typeof(env, args[0], &argument_type));

    NAPIValue result;
    if (argument_type == NAPINumber) {
        NAPI_CALL(env, napi_create_string_utf8(
                env, "number", NAPI_AUTO_LENGTH, &result));
    } else if (argument_type == NAPIString) {
        NAPI_CALL(env, napi_create_string_utf8(
                env, "string", NAPI_AUTO_LENGTH, &result));
    } else if (argument_type == NAPIFunction) {
        NAPI_CALL(env, napi_create_string_utf8(
                env, "function", NAPI_AUTO_LENGTH, &result));
    } else if (argument_type == NAPIObject) {
        NAPI_CALL(env, napi_create_string_utf8(
                env, "object", NAPI_AUTO_LENGTH, &result));
    } else if (argument_type == NAPIBoolean) {
        NAPI_CALL(env, napi_create_string_utf8(
                env, "boolean", NAPI_AUTO_LENGTH, &result));
    } else if (argument_type == NAPIUndefined) {
        NAPI_CALL(env, napi_create_string_utf8(
                env, "undefined", NAPI_AUTO_LENGTH, &result));
    } else if (argument_type == NAPISymbol) {
        NAPI_CALL(env, napi_create_string_utf8(
                env, "symbol", NAPI_AUTO_LENGTH, &result));
    } else if (argument_type == NAPINull) {
        NAPI_CALL(env, napi_create_string_utf8(
                env, "null", NAPI_AUTO_LENGTH, &result));
    }
    return result;
}

static bool deref_item_called = false;
static void deref_item(NAPIEnv env, void *data, void *hint) {
    (void) hint;

    NAPI_ASSERT_RETURN_VOID(env, data == &deref_item_called,
                            "Finalize callback was called with the correct pointer");

    deref_item_called = true;
}

static NAPIValue deref_item_was_called(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue it_was_called;

    NAPI_CALL(env, napi_get_boolean(env, deref_item_called, &it_was_called));

    return it_was_called;
}

static NAPIValue wrap_first_arg(NAPIEnv env,
                                NAPICallbackInfo info,
                                NAPIFinalize finalizer,
                                void *data) {
    size_t argc = 1;
    NAPIValue to_wrap;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &to_wrap, nullptr, nullptr));
    NAPI_CALL(env, napi_wrap(env, to_wrap, data, finalizer, nullptr, nullptr));

    return to_wrap;
}

static NAPIValue wrap(NAPIEnv env, NAPICallbackInfo info) {
    deref_item_called = false;
    return wrap_first_arg(env, info, deref_item, &deref_item_called);
}

static NAPIValue unwrap(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue wrapped;
    void *data;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &wrapped, nullptr, nullptr));
    NAPI_CALL(env, napi_unwrap(env, wrapped, &data));

    return getUndefined(env);
}

static NAPIValue remove_wrap(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue wrapped;
    void *data;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &wrapped, nullptr, nullptr));
    NAPI_CALL(env, napi_remove_wrap(env, wrapped, &data));

    return getUndefined(env);
}

static bool finalize_called = false;
static void test_finalize(NAPIEnv /*globalEnv*/, void */*data*/, void */*hint*/) {
    finalize_called = true;
}

static NAPIValue test_finalize_wrap(NAPIEnv env, NAPICallbackInfo info) {
    return wrap_first_arg(env, info, test_finalize, nullptr);
}

static NAPIValue finalize_was_called(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue it_was_called;

    NAPI_CALL(env, napi_get_boolean(env, finalize_called, &it_was_called));

    return it_was_called;
}

static NAPIValue testNapiRun(NAPIEnv env, NAPICallbackInfo info) {
    NAPIValue script, result;
    size_t argc = 1;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &script, nullptr, nullptr));

    NAPI_CALL(env, napi_run_script(env, script, &result));

    return result;
}

static const char *env_cleanup_finalizer_messages[] = {
        "simple wrap",
        "wrap, removeWrap",
        "first wrap",
        "second wrap"
};

static void cleanup_env_finalizer(NAPIEnv env, void *data, void *hint) {
    (void) env;
    (void) hint;

    printf("finalize at globalEnv cleanup for %s\n",
           env_cleanup_finalizer_messages[(uintptr_t) data]);
}

static NAPIValue env_cleanup_wrap(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 2;
    NAPIValue argv[2];
    uint32_t value;
    uintptr_t ptr_value;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_CALL(env, napi_get_value_uint32(env, argv[1], &value));

    ptr_value = value;
    return wrap_first_arg(env, info, cleanup_env_finalizer, (void *) ptr_value);
}

EXTERN_C_END

TEST_F(Test, TestGeneral) {
    NAPIPropertyDescriptor descriptors[] = {
            DECLARE_NAPI_PROPERTY("testStrictEquals", testStrictEquals),
            DECLARE_NAPI_PROPERTY("testGetPrototype", testGetPrototype),
            DECLARE_NAPI_PROPERTY("testNapiRun", testNapiRun),
            DECLARE_NAPI_PROPERTY("doInstanceOf", doInstanceOf),
            DECLARE_NAPI_PROPERTY("getUndefined", getUndefined),
            DECLARE_NAPI_PROPERTY("getNull", getNull),
            DECLARE_NAPI_PROPERTY("createNapiError", createNapiError),
            DECLARE_NAPI_PROPERTY("testNapiErrorCleanup", testNapiErrorCleanup),
            DECLARE_NAPI_PROPERTY("testNapiTypeof", testNapiTypeof),
            DECLARE_NAPI_PROPERTY("wrap", wrap),
            DECLARE_NAPI_PROPERTY("envCleanupWrap", env_cleanup_wrap),
            DECLARE_NAPI_PROPERTY("unwrap", unwrap),
            DECLARE_NAPI_PROPERTY("removeWrap", remove_wrap),
            DECLARE_NAPI_PROPERTY("testFinalizeWrap", test_finalize_wrap),
            DECLARE_NAPI_PROPERTY("finalizeWasCalled", finalize_was_called),
            DECLARE_NAPI_PROPERTY("derefItemWasCalled", deref_item_was_called),
    };

    ASSERT_EQ(napi_define_properties(
            globalEnv, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors), NAPIOK);

    NAPIValue result;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(globalEnv,
                                         "(()=>{var t={7473:(t,r,e)=>{e(8922),e(5967),e(5824),e(8555),e(2615),e(1732),e(5903),e(1825),e(8394),e(5915),e(1766),e(2737),e(9911),e(4315),e(3131),e(4714),e(659),e(9120),e(5327),e(1502);var n=e(4058);t.exports=n.Symbol},3916:t=>{t.exports=function(t){if(\"function\"!=typeof t)throw TypeError(String(t)+\" is not a function\");return t}},6059:(t,r,e)=>{var n=e(941);t.exports=function(t){if(!n(t))throw TypeError(String(t)+\" is not an object\");return t}},1692:(t,r,e)=>{var n=e(4529),o=e(3057),i=e(9413),a=function(t){return function(r,e,a){var u,c=n(r),s=o(c.length),f=i(a,s);if(t&&e!=e){for(;s>f;)if((u=c[f++])!=u)return!0}else for(;s>f;f++)if((t||f in c)&&c[f]===e)return t||f||0;return!t&&-1}};t.exports={includes:a(!0),indexOf:a(!1)}},3610:(t,r,e)=>{var n=e(6843),o=e(7026),i=e(9678),a=e(3057),u=e(4692),c=[].push,s=function(t){var r=1==t,e=2==t,s=3==t,f=4==t,l=6==t,p=7==t,y=5==t||l;return function(v,h,b,d){for(var g,m,w=i(v),x=o(w),O=n(h,b,3),S=a(x.length),j=0,T=d||u,P=r?T(v,S):e||p?T(v,0):void 0;S>j;j++)if((y||j in x)&&(m=O(g=x[j],j,w),t))if(r)P[j]=m;else if(m)switch(t){case 3:return!0;case 5:return g;case 6:return j;case 2:c.call(P,g)}else switch(t){case 4:return!1;case 7:c.call(P,g)}return l?-1:s||f?f:P}};t.exports={forEach:s(0),map:s(1),filter:s(2),some:s(3),every:s(4),find:s(5),findIndex:s(6),filterOut:s(7)}},568:(t,r,e)=>{var n=e(5981),o=e(9813),i=e(3385),a=o(\"species\");t.exports=function(t){return i>=51||!n((function(){var r=[];return(r.constructor={})[a]=function(){return{foo:1}},1!==r[t](Boolean).foo}))}},4692:(t,r,e)=>{var n=e(941),o=e(1052),i=e(9813)(\"species\");t.exports=function(t,r){var e;return o(t)&&(\"function\"!=typeof(e=t.constructor)||e!==Array&&!o(e.prototype)?n(e)&&null===(e=e[i])&&(e=void 0):e=void 0),new(void 0===e?Array:e)(0===r?0:r)}},2532:t=>{var r={}.toString;t.exports=function(t){return r.call(t).slice(8,-1)}},9697:(t,r,e)=>{var n=e(2885),o=e(2532),i=e(9813)(\"toStringTag\"),a=\"Arguments\"==o(function(){return arguments}());t.exports=n?o:function(t){var r,e,n;return void 0===t?\"Undefined\":null===t?\"Null\":\"string\"==typeof(e=function(t,r){try{return t[r]}catch(t){}}(r=Object(t),i))?e:a?o(r):\"Object\"==(n=o(r))&&\"function\"==typeof r.callee?\"Arguments\":n}},2029:(t,r,e)=>{var n=e(5746),o=e(5988),i=e(1887);t.exports=n?function(t,r,e){return o.f(t,r,i(1,e))}:function(t,r,e){return t[r]=e,t}},1887:t=>{t.exports=function(t,r){return{enumerable:!(1&t),configurable:!(2&t),writable:!(4&t),value:r}}},5449:(t,r,e)=>{\"use strict\";var n=e(6935),o=e(5988),i=e(1887);t.exports=function(t,r,e){var a=n(r);a in t?o.f(t,a,i(0,e)):t[a]=e}},6349:(t,r,e)=>{var n=e(4058),o=e(7457),i=e(1477),a=e(5988).f;t.exports=function(t){var r=n.Symbol||(n.Symbol={});o(r,t)||a(r,t,{value:i.f(t)})}},5746:(t,r,e)=>{var n=e(5981);t.exports=!n((function(){return 7!=Object.defineProperty({},1,{get:function(){return 7}})[1]}))},1333:(t,r,e)=>{var n=e(1899),o=e(941),i=n.document,a=o(i)&&o(i.createElement);t.exports=function(t){return a?i.createElement(t):{}}},6049:(t,r,e)=>{var n=e(2532),o=e(1899);t.exports=\"process\"==n(o.process)},2861:(t,r,e)=>{var n=e(626);t.exports=n(\"navigator\",\"userAgent\")||\"\"},3385:(t,r,e)=>{var n,o,i=e(1899),a=e(2861),u=i.process,c=u&&u.versions,s=c&&c.v8;s?o=(n=s.split(\".\"))[0]+n[1]:a&&(!(n=a.match(/Edge\\/(\\d+)/))||n[1]>=74)&&(n=a.match(/Chrome\\/(\\d+)/))&&(o=n[1]),t.exports=o&&+o},6759:t=>{t.exports=[\"constructor\",\"hasOwnProperty\",\"isPrototypeOf\",\"propertyIsEnumerable\",\"toLocaleString\",\"toString\",\"valueOf\"]},6887:(t,r,e)=>{\"use strict\";var n=e(1899),o=e(9677).f,i=e(7252),a=e(4058),u=e(6843),c=e(2029),s=e(7457),f=function(t){var r=function(r,e,n){if(this instanceof t){switch(arguments.length){case 0:return new t;case 1:return new t(r);case 2:return new t(r,e)}return new t(r,e,n)}return t.apply(this,arguments)};return r.prototype=t.prototype,r};t.exports=function(t,r){var e,l,p,y,v,h,b,d,g=t.target,m=t.global,w=t.stat,x=t.proto,O=m?n:w?n[g]:(n[g]||{}).prototype,S=m?a:a[g]||(a[g]={}),j=S.prototype;for(p in r)e=!i(m?p:g+(w?\".\":\"#\")+p,t.forced)&&O&&s(O,p),v=S[p],e&&(h=t.noTargetGet?(d=o(O,p))&&d.value:O[p]),y=e&&h?h:r[p],e&&typeof v==typeof y||(b=t.bind&&e?u(y,n):t.wrap&&e?f(y):x&&\"function\"==typeof y?u(Function.call,y):y,(t.sham||y&&y.sham||v&&v.sham)&&c(b,\"sham\",!0),S[p]=b,x&&(s(a,l=g+\"Prototype\")||c(a,l,{}),a[l][p]=y,t.real&&j&&!j[p]&&c(j,p,y)))}},5981:t=>{t.exports=function(t){try{return!!t()}catch(t){return!0}}},6843:(t,r,e)=>{var n=e(3916);t.exports=function(t,r,e){if(n(t),void 0===r)return t;switch(e){case 0:return function(){return t.call(r)};case 1:return function(e){return t.call(r,e)};case 2:return function(e,n){return t.call(r,e,n)};case 3:return function(e,n,o){return t.call(r,e,n,o)}}return function(){return t.apply(r,arguments)}}},626:(t,r,e)=>{var n=e(4058),o=e(1899),i=function(t){return\"function\"==typeof t?t:void 0};t.exports=function(t,r){return arguments.length<2?i(n[t])||i(o[t]):n[t]&&n[t][r]||o[t]&&o[t][r]}},1899:(t,r,e)=>{var n=function(t){return t&&t.Math==Math&&t};t.exports=n(\"object\"==typeof globalThis&&globalThis)||n(\"object\"==typeof window&&window)||n(\"object\"==typeof self&&self)||n(\"object\"==typeof e.g&&e.g)||function(){return this}()||Function(\"return this\")()},7457:(t,r,e)=>{var n=e(9678),o={}.hasOwnProperty;t.exports=function(t,r){return o.call(n(t),r)}},7748:t=>{t.exports={}},5463:(t,r,e)=>{var n=e(626);t.exports=n(\"document\",\"documentElement\")},2840:(t,r,e)=>{var n=e(5746),o=e(5981),i=e(1333);t.exports=!n&&!o((function(){return 7!=Object.defineProperty(i(\"div\"),\"a\",{get:function(){return 7}}).a}))},7026:(t,r,e)=>{var n=e(5981),o=e(2532),i=\"\".split;t.exports=n((function(){return!Object(\"z\").propertyIsEnumerable(0)}))?function(t){return\"String\"==o(t)?i.call(t,\"\"):Object(t)}:Object},1302:(t,r,e)=>{var n=e(3030),o=Function.toString;\"function\"!=typeof n.inspectSource&&(n.inspectSource=function(t){return o.call(t)}),t.exports=n.inspectSource},5402:(t,r,e)=>{var n,o,i,a=e(8019),u=e(1899),c=e(941),s=e(2029),f=e(7457),l=e(3030),p=e(4262),y=e(7748),v=\"Object already initialized\",h=u.WeakMap;if(a){var b=l.state||(l.state=new h),d=b.get,g=b.has,m=b.set;n=function(t,r){if(g.call(b,t))throw new TypeError(v);return r.facade=t,m.call(b,t,r),r},o=function(t){return d.call(b,t)||{}},i=function(t){return g.call(b,t)}}else{var w=p(\"state\");y[w]=!0,n=function(t,r){if(f(t,w))throw new TypeError(v);return r.facade=t,s(t,w,r),r},o=function(t){return f(t,w)?t[w]:{}},i=function(t){return f(t,w)}}t.exports={set:n,get:o,has:i,enforce:function(t){return i(t)?o(t):n(t,{})},getterFor:function(t){return function(r){var e;if(!c(r)||(e=o(r)).type!==t)throw TypeError(\"Incompatible receiver, \"+t+\" required\");return e}}}},1052:(t,r,e)=>{var n=e(2532);t.exports=Array.isArray||function(t){return\"Array\"==n(t)}},7252:(t,r,e)=>{var n=e(5981),o=/#|\\.prototype\\./,i=function(t,r){var e=u[a(t)];return e==s||e!=c&&(\"function\"==typeof r?n(r):!!r)},a=i.normalize=function(t){return String(t).replace(o,\".\").toLowerCase()},u=i.data={},c=i.NATIVE=\"N\",s=i.POLYFILL=\"P\";t.exports=i},941:t=>{t.exports=function(t){return\"object\"==typeof t?null!==t:\"function\"==typeof t}},2529:t=>{t.exports=!0},2497:(t,r,e)=>{var n=e(6049),o=e(3385),i=e(5981);t.exports=!!Object.getOwnPropertySymbols&&!i((function(){return!Symbol.sham&&(n?38===o:o>37&&o<41)}))},8019:(t,r,e)=>{var n=e(1899),o=e(1302),i=n.WeakMap;t.exports=\"function\"==typeof i&&/native code/.test(o(i))},9290:(t,r,e)=>{var n,o=e(6059),i=e(9938),a=e(6759),u=e(7748),c=e(5463),s=e(1333),f=e(4262)(\"IE_PROTO\"),l=function(){},p=function(t){return\"<script>\"+t+\"<\\/script>\"},y=function(){try{n=document.domain&&new ActiveXObject(\"htmlfile\")}catch(t){}var t,r;y=n?function(t){t.write(p(\"\")),t.close();var r=t.parentWindow.Object;return t=null,r}(n):((r=s(\"iframe\")).style.display=\"none\",c.appendChild(r),r.src=String(\"javascript:\"),(t=r.contentWindow.document).open(),t.write(p(\"document.F=Object\")),t.close(),t.F);for(var e=a.length;e--;)delete y.prototype[a[e]];return y()};u[f]=!0,t.exports=Object.create||function(t,r){var e;return null!==t?(l.prototype=o(t),e=new l,l.prototype=null,e[f]=t):e=y(),void 0===r?e:i(e,r)}},9938:(t,r,e)=>{var n=e(5746),o=e(5988),i=e(6059),a=e(4771);t.exports=n?Object.defineProperties:function(t,r){i(t);for(var e,n=a(r),u=n.length,c=0;u>c;)o.f(t,e=n[c++],r[e]);return t}},5988:(t,r,e)=>{var n=e(5746),o=e(2840),i=e(6059),a=e(6935),u=Object.defineProperty;r.f=n?u:function(t,r,e){if(i(t),r=a(r,!0),i(e),o)try{return u(t,r,e)}catch(t){}if(\"get\"in e||\"set\"in e)throw TypeError(\"Accessors not supported\");return\"value\"in e&&(t[r]=e.value),t}},9677:(t,r,e)=>{var n=e(5746),o=e(6760),i=e(1887),a=e(4529),u=e(6935),c=e(7457),s=e(2840),f=Object.getOwnPropertyDescriptor;r.f=n?f:function(t,r){if(t=a(t),r=u(r,!0),s)try{return f(t,r)}catch(t){}if(c(t,r))return i(!o.f.call(t,r),t[r])}},684:(t,r,e)=>{var n=e(4529),o=e(946).f,i={}.toString,a=\"object\"==typeof window&&window&&Object.getOwnPropertyNames?Object.getOwnPropertyNames(window):[];t.exports.f=function(t){return a&&\"[object Window]\"==i.call(t)?function(t){try{return o(t)}catch(t){return a.slice()}}(t):o(n(t))}},946:(t,r,e)=>{var n=e(5629),o=e(6759).concat(\"length\",\"prototype\");r.f=Object.getOwnPropertyNames||function(t){return n(t,o)}},7857:(t,r)=>{r.f=Object.getOwnPropertySymbols},5629:(t,r,e)=>{var n=e(7457),o=e(4529),i=e(1692).indexOf,a=e(7748);t.exports=function(t,r){var e,u=o(t),c=0,s=[];for(e in u)!n(a,e)&&n(u,e)&&s.push(e);for(;r.length>c;)n(u,e=r[c++])&&(~i(s,e)||s.push(e));return s}},4771:(t,r,e)=>{var n=e(5629),o=e(6759);t.exports=Object.keys||function(t){return n(t,o)}},6760:(t,r)=>{\"use strict\";var e={}.propertyIsEnumerable,n=Object.getOwnPropertyDescriptor,o=n&&!e.call({1:2},1);r.f=o?function(t){var r=n(this,t);return!!r&&r.enumerable}:e},5623:(t,r,e)=>{\"use strict\";var n=e(2885),o=e(9697);t.exports=n?{}.toString:function(){return\"[object \"+o(this)+\"]\"}},4058:t=>{t.exports={}},9754:(t,r,e)=>{var n=e(2029);t.exports=function(t,r,e,o){o&&o.enumerable?t[r]=e:n(t,r,e)}},8219:t=>{t.exports=function(t){if(null==t)throw TypeError(\"Can't call method on \"+t);return t}},4911:(t,r,e)=>{var n=e(1899),o=e(2029);t.exports=function(t,r){try{o(n,t,r)}catch(e){n[t]=r}return r}},904:(t,r,e)=>{var n=e(2885),o=e(5988).f,i=e(2029),a=e(7457),u=e(5623),c=e(9813)(\"toStringTag\");t.exports=function(t,r,e,s){if(t){var f=e?t:t.prototype;a(f,c)||o(f,c,{configurable:!0,value:r}),s&&!n&&i(f,\"toString\",u)}}},4262:(t,r,e)=>{var n=e(8726),o=e(9418),i=n(\"keys\");t.exports=function(t){return i[t]||(i[t]=o(t))}},3030:(t,r,e)=>{var n=e(1899),o=e(4911),i=\"__core-js_shared__\",a=n[i]||o(i,{});t.exports=a},8726:(t,r,e)=>{var n=e(2529),o=e(3030);(t.exports=function(t,r){return o[t]||(o[t]=void 0!==r?r:{})})(\"versions\",[]).push({version:\"3.11.1\",mode:n?\"pure\":\"global\",copyright:\"© 2021 Denis Pushkarev (zloirock.ru)\"})},9413:(t,r,e)=>{var n=e(8459),o=Math.max,i=Math.min;t.exports=function(t,r){var e=n(t);return e<0?o(e+r,0):i(e,r)}},4529:(t,r,e)=>{var n=e(7026),o=e(8219);t.exports=function(t){return n(o(t))}},8459:t=>{var r=Math.ceil,e=Math.floor;t.exports=function(t){return isNaN(t=+t)?0:(t>0?e:r)(t)}},3057:(t,r,e)=>{var n=e(8459),o=Math.min;t.exports=function(t){return t>0?o(n(t),9007199254740991):0}},9678:(t,r,e)=>{var n=e(8219);t.exports=function(t){return Object(n(t))}},6935:(t,r,e)=>{var n=e(941);t.exports=function(t,r){if(!n(t))return t;var e,o;if(r&&\"function\"==typeof(e=t.toString)&&!n(o=e.call(t)))return o;if(\"function\"==typeof(e=t.valueOf)&&!n(o=e.call(t)))return o;if(!r&&\"function\"==typeof(e=t.toString)&&!n(o=e.call(t)))return o;throw TypeError(\"Can't convert object to primitive value\")}},2885:(t,r,e)=>{var n={};n[e(9813)(\"toStringTag\")]=\"z\",t.exports=\"[object z]\"===String(n)},9418:t=>{var r=0,e=Math.random();t.exports=function(t){return\"Symbol(\"+String(void 0===t?\"\":t)+\")_\"+(++r+e).toString(36)}},2302:(t,r,e)=>{var n=e(2497);t.exports=n&&!Symbol.sham&&\"symbol\"==typeof Symbol.iterator},1477:(t,r,e)=>{var n=e(9813);r.f=n},9813:(t,r,e)=>{var n=e(1899),o=e(8726),i=e(7457),a=e(9418),u=e(2497),c=e(2302),s=o(\"wks\"),f=n.Symbol,l=c?f:f&&f.withoutSetter||a;t.exports=function(t){return i(s,t)&&(u||\"string\"==typeof s[t])||(u&&i(f,t)?s[t]=f[t]:s[t]=l(\"Symbol.\"+t)),s[t]}},8922:(t,r,e)=>{\"use strict\";var n=e(6887),o=e(5981),i=e(1052),a=e(941),u=e(9678),c=e(3057),s=e(5449),f=e(4692),l=e(568),p=e(9813),y=e(3385),v=p(\"isConcatSpreadable\"),h=9007199254740991,b=\"Maximum allowed index exceeded\",d=y>=51||!o((function(){var t=[];return t[v]=!1,t.concat()[0]!==t})),g=l(\"concat\"),m=function(t){if(!a(t))return!1;var r=t[v];return void 0!==r?!!r:i(t)};n({target:\"Array\",proto:!0,forced:!d||!g},{concat:function(t){var r,e,n,o,i,a=u(this),l=f(a,0),p=0;for(r=-1,n=arguments.length;r<n;r++)if(m(i=-1===r?a:arguments[r])){if(p+(o=c(i.length))>h)throw TypeError(b);for(e=0;e<o;e++,p++)e in i&&s(l,p,i[e])}else{if(p>=h)throw TypeError(b);s(l,p++,i)}return l.length=p,l}})},9120:(t,r,e)=>{var n=e(1899);e(904)(n.JSON,\"JSON\",!0)},5327:()=>{},5967:()=>{},1502:()=>{},8555:(t,r,e)=>{e(6349)(\"asyncIterator\")},2615:()=>{},1732:(t,r,e)=>{e(6349)(\"hasInstance\")},5903:(t,r,e)=>{e(6349)(\"isConcatSpreadable\")},1825:(t,r,e)=>{e(6349)(\"iterator\")},5824:(t,r,e)=>{\"use strict\";var n=e(6887),o=e(1899),i=e(626),a=e(2529),u=e(5746),c=e(2497),s=e(2302),f=e(5981),l=e(7457),p=e(1052),y=e(941),v=e(6059),h=e(9678),b=e(4529),d=e(6935),g=e(1887),m=e(9290),w=e(4771),x=e(946),O=e(684),S=e(7857),j=e(9677),T=e(5988),P=e(6760),E=e(2029),_=e(9754),N=e(8726),A=e(4262),M=e(7748),k=e(9418),I=e(9813),q=e(1477),F=e(6349),C=e(904),W=e(5402),R=e(3610).forEach,z=A(\"hidden\"),G=\"Symbol\",L=I(\"toPrimitive\"),D=W.set,J=W.getterFor(G),B=Object.prototype,Q=o.Symbol,U=i(\"JSON\",\"stringify\"),V=j.f,X=T.f,Y=O.f,H=P.f,K=N(\"symbols\"),Z=N(\"op-symbols\"),$=N(\"string-to-symbol-registry\"),tt=N(\"symbol-to-string-registry\"),rt=N(\"wks\"),et=o.QObject,nt=!et||!et.prototype||!et.prototype.findChild,ot=u&&f((function(){return 7!=m(X({},\"a\",{get:function(){return X(this,\"a\",{value:7}).a}})).a}))?function(t,r,e){var n=V(B,r);n&&delete B[r],X(t,r,e),n&&t!==B&&X(B,r,n)}:X,it=function(t,r){var e=K[t]=m(Q.prototype);return D(e,{type:G,tag:t,description:r}),u||(e.description=r),e},at=s?function(t){return\"symbol\"==typeof t}:function(t){return Object(t)instanceof Q},ut=function(t,r,e){t===B&&ut(Z,r,e),v(t);var n=d(r,!0);return v(e),l(K,n)?(e.enumerable?(l(t,z)&&t[z][n]&&(t[z][n]=!1),e=m(e,{enumerable:g(0,!1)})):(l(t,z)||X(t,z,g(1,{})),t[z][n]=!0),ot(t,n,e)):X(t,n,e)},ct=function(t,r){v(t);var e=b(r),n=w(e).concat(pt(e));return R(n,(function(r){u&&!st.call(e,r)||ut(t,r,e[r])})),t},st=function(t){var r=d(t,!0),e=H.call(this,r);return!(this===B&&l(K,r)&&!l(Z,r))&&(!(e||!l(this,r)||!l(K,r)||l(this,z)&&this[z][r])||e)},ft=function(t,r){var e=b(t),n=d(r,!0);if(e!==B||!l(K,n)||l(Z,n)){var o=V(e,n);return!o||!l(K,n)||l(e,z)&&e[z][n]||(o.enumerable=!0),o}},lt=function(t){var r=Y(b(t)),e=[];return R(r,(function(t){l(K,t)||l(M,t)||e.push(t)})),e},pt=function(t){var r=t===B,e=Y(r?Z:b(t)),n=[];return R(e,(function(t){!l(K,t)||r&&!l(B,t)||n.push(K[t])})),n};c||(_((Q=function(){if(this instanceof Q)throw TypeError(\"Symbol is not a constructor\");var t=arguments.length&&void 0!==arguments[0]?String(arguments[0]):void 0,r=k(t),e=function(t){this===B&&e.call(Z,t),l(this,z)&&l(this[z],r)&&(this[z][r]=!1),ot(this,r,g(1,t))};return u&&nt&&ot(B,r,{configurable:!0,set:e}),it(r,t)}).prototype,\"toString\",(function(){return J(this).tag})),_(Q,\"withoutSetter\",(function(t){return it(k(t),t)})),P.f=st,T.f=ut,j.f=ft,x.f=O.f=lt,S.f=pt,q.f=function(t){return it(I(t),t)},u&&(X(Q.prototype,\"description\",{configurable:!0,get:function(){return J(this).description}}),a||_(B,\"propertyIsEnumerable\",st,{unsafe:!0}))),n({global:!0,wrap:!0,forced:!c,sham:!c},{Symbol:Q}),R(w(rt),(function(t){F(t)})),n({target:G,stat:!0,forced:!c},{for:function(t){var r=String(t);if(l($,r))return $[r];var e=Q(r);return $[r]=e,tt[e]=r,e},keyFor:function(t){if(!at(t))throw TypeError(t+\" is not a symbol\");if(l(tt,t))return tt[t]},useSetter:function(){nt=!0},useSimple:function(){nt=!1}}),n({target:\"Object\",stat:!0,forced:!c,sham:!u},{create:function(t,r){return void 0===r?m(t):ct(m(t),r)},defineProperty:ut,defineProperties:ct,getOwnPropertyDescriptor:ft}),n({target:\"Object\",stat:!0,forced:!c},{getOwnPropertyNames:lt,getOwnPropertySymbols:pt}),n({target:\"Object\",stat:!0,forced:f((function(){S.f(1)}))},{getOwnPropertySymbols:function(t){return S.f(h(t))}}),U&&n({target:\"JSON\",stat:!0,forced:!c||f((function(){var t=Q();return\"[null]\"!=U([t])||\"{}\"!=U({a:t})||\"{}\"!=U(Object(t))}))},{stringify:function(t,r,e){for(var n,o=[t],i=1;arguments.length>i;)o.push(arguments[i++]);if(n=r,(y(r)||void 0!==t)&&!at(t))return p(r)||(r=function(t,r){if(\"function\"==typeof n&&(r=n.call(this,t,r)),!at(r))return r}),o[1]=r,U.apply(null,o)}}),Q.prototype[L]||E(Q.prototype,L,Q.prototype.valueOf),C(Q,G),M[z]=!0},5915:(t,r,e)=>{e(6349)(\"matchAll\")},8394:(t,r,e)=>{e(6349)(\"match\")},1766:(t,r,e)=>{e(6349)(\"replace\")},2737:(t,r,e)=>{e(6349)(\"search\")},9911:(t,r,e)=>{e(6349)(\"species\")},4315:(t,r,e)=>{e(6349)(\"split\")},3131:(t,r,e)=>{e(6349)(\"toPrimitive\")},4714:(t,r,e)=>{e(6349)(\"toStringTag\")},659:(t,r,e)=>{e(6349)(\"unscopables\")},2547:(t,r,e)=>{var n=e(7473);t.exports=n}},r={};function e(n){var o=r[n];if(void 0!==o)return o.exports;var i=r[n]={exports:{}};return t[n](i,i.exports,e),i.exports}e.n=t=>{var r=t&&t.__esModule?()=>t.default:()=>t;return e.d(r,{a:r}),r},e.d=(t,r)=>{for(var n in r)e.o(r,n)&&!e.o(t,n)&&Object.defineProperty(t,n,{enumerable:!0,get:r[n]})},e.g=function(){if(\"object\"==typeof globalThis)return globalThis;try{return this||new Function(\"return this\")()}catch(t){if(\"object\"==typeof window)return window}}(),e.o=(t,r)=>Object.prototype.hasOwnProperty.call(t,r),(()=>{\"use strict\";function t(r,e){return(t=Object.setPrototypeOf||function(t,r){return t.__proto__=r,t})(r,e)}function r(t){return(r=Object.setPrototypeOf?Object.getPrototypeOf:function(t){return t.__proto__||Object.getPrototypeOf(t)})(t)}function n(t){return(n=\"function\"==typeof Symbol&&\"symbol\"==typeof Symbol.iterator?function(t){return typeof t}:function(t){return t&&\"function\"==typeof Symbol&&t.constructor===Symbol&&t!==Symbol.prototype?\"symbol\":typeof t})(t)}function o(t,r){return!r||\"object\"!==n(r)&&\"function\"!=typeof r?function(t){if(void 0===t)throw new ReferenceError(\"this hasn't been initialised - super() hasn't been called\");return t}(t):r}function i(t,r){if(!(t instanceof r))throw new TypeError(\"Cannot call a class as a function\")}var a=e(2547),u=e.n(a),c=function t(){i(this,t)},s=function(e){!function(r,e){if(\"function\"!=typeof e&&null!==e)throw new TypeError(\"Super expression must either be null or a function\");r.prototype=Object.create(e&&e.prototype,{constructor:{value:r,writable:!0,configurable:!0}}),e&&t(r,e)}(c,e);var n,a,u=(n=c,a=function(){if(\"undefined\"==typeof Reflect||!Reflect.construct)return!1;if(Reflect.construct.sham)return!1;if(\"function\"==typeof Proxy)return!0;try{return Boolean.prototype.valueOf.call(Reflect.construct(Boolean,[],(function(){}))),!0}catch(t){return!1}}(),function(){var t,e=r(n);if(a){var i=r(this).constructor;t=Reflect.construct(e,arguments,i)}else t=e.apply(this,arguments);return o(this,t)});function c(){return i(this,c),u.apply(this,arguments)}return c}(c),f=new c,l=new s;globalThis.assert.ok(globalThis.addon.testStrictEquals(\"1\",\"1\")),globalThis.assert.strictEqual(globalThis.addon.testStrictEquals(\"1\",1),!1),globalThis.assert.ok(globalThis.addon.testStrictEquals(1,1)),globalThis.assert.strictEqual(globalThis.addon.testGetPrototype(f),Object.getPrototypeOf(f)),globalThis.assert.strictEqual(globalThis.addon.testGetPrototype(l),Object.getPrototypeOf(l)),globalThis.assert.notStrictEqual(globalThis.addon.testGetPrototype(f),globalThis.addon.testGetPrototype(l)),[123,\"test string\",function(){},new Object,!0,void 0,u()()].forEach((function(t){globalThis.assert.strictEqual(globalThis.addon.testNapiTypeof(t),typeof t)})),globalThis.assert.strictEqual(globalThis.addon.testNapiTypeof(null),\"null\");var p={};globalThis.addon.wrap(p),globalThis.assert.throws((function(){return globalThis.addon.wrap(p)})),globalThis.addon.removeWrap(p);var y={};globalThis.addon.wrap(y),globalThis.addon.removeWrap(y),globalThis.addon.wrap(y),globalThis.addon.removeWrap(y)})()})();",
                                         "https://www.didi.com/test_general_test.js",
                                         &result), NAPIOK);

    ASSERT_EQ(NAPIRunScriptWithSourceUrl(globalEnv,
                                         "(()=>{\"use strict\";globalThis.assert.strictEqual(globalThis.addon.getUndefined(),void 0),globalThis.assert.strictEqual(globalThis.addon.getNull(),null)})();",
                                         "https://www.didi.com/test_general_testGlobals.js",
                                         &result), NAPIOK);

    ASSERT_EQ(NAPIRunScriptWithSourceUrl(globalEnv,
                                         "(()=>{var t={7604:(t,r,e)=>{e(1732),e(7633);var n=e(1477);t.exports=n.f(\"hasInstance\")},7473:(t,r,e)=>{e(8922),e(5967),e(5824),e(8555),e(2615),e(1732),e(5903),e(1825),e(8394),e(5915),e(1766),e(2737),e(9911),e(4315),e(3131),e(4714),e(659),e(9120),e(5327),e(1502);var n=e(4058);t.exports=n.Symbol},3916:t=>{t.exports=function(t){if(\"function\"!=typeof t)throw TypeError(String(t)+\" is not a function\");return t}},6059:(t,r,e)=>{var n=e(941);t.exports=function(t){if(!n(t))throw TypeError(String(t)+\" is not an object\");return t}},1692:(t,r,e)=>{var n=e(4529),o=e(3057),i=e(9413),c=function(t){return function(r,e,c){var u,a=n(r),f=o(a.length),s=i(c,f);if(t&&e!=e){for(;f>s;)if((u=a[s++])!=u)return!0}else for(;f>s;s++)if((t||s in a)&&a[s]===e)return t||s||0;return!t&&-1}};t.exports={includes:c(!0),indexOf:c(!1)}},3610:(t,r,e)=>{var n=e(6843),o=e(7026),i=e(9678),c=e(3057),u=e(4692),a=[].push,f=function(t){var r=1==t,e=2==t,f=3==t,s=4==t,p=6==t,l=7==t,v=5==t||p;return function(y,h,b,g){for(var d,m,x=i(y),w=o(x),O=n(h,b,3),S=c(w.length),j=0,P=g||u,E=r?P(y,S):e||l?P(y,0):void 0;S>j;j++)if((v||j in w)&&(m=O(d=w[j],j,x),t))if(r)E[j]=m;else if(m)switch(t){case 3:return!0;case 5:return d;case 6:return j;case 2:a.call(E,d)}else switch(t){case 4:return!1;case 7:a.call(E,d)}return p?-1:f||s?s:E}};t.exports={forEach:f(0),map:f(1),filter:f(2),some:f(3),every:f(4),find:f(5),findIndex:f(6),filterOut:f(7)}},568:(t,r,e)=>{var n=e(5981),o=e(9813),i=e(3385),c=o(\"species\");t.exports=function(t){return i>=51||!n((function(){var r=[];return(r.constructor={})[c]=function(){return{foo:1}},1!==r[t](Boolean).foo}))}},4692:(t,r,e)=>{var n=e(941),o=e(1052),i=e(9813)(\"species\");t.exports=function(t,r){var e;return o(t)&&(\"function\"!=typeof(e=t.constructor)||e!==Array&&!o(e.prototype)?n(e)&&null===(e=e[i])&&(e=void 0):e=void 0),new(void 0===e?Array:e)(0===r?0:r)}},2532:t=>{var r={}.toString;t.exports=function(t){return r.call(t).slice(8,-1)}},9697:(t,r,e)=>{var n=e(2885),o=e(2532),i=e(9813)(\"toStringTag\"),c=\"Arguments\"==o(function(){return arguments}());t.exports=n?o:function(t){var r,e,n;return void 0===t?\"Undefined\":null===t?\"Null\":\"string\"==typeof(e=function(t,r){try{return t[r]}catch(t){}}(r=Object(t),i))?e:c?o(r):\"Object\"==(n=o(r))&&\"function\"==typeof r.callee?\"Arguments\":n}},4160:(t,r,e)=>{var n=e(5981);t.exports=!n((function(){function t(){}return t.prototype.constructor=null,Object.getPrototypeOf(new t)!==t.prototype}))},2029:(t,r,e)=>{var n=e(5746),o=e(5988),i=e(1887);t.exports=n?function(t,r,e){return o.f(t,r,i(1,e))}:function(t,r,e){return t[r]=e,t}},1887:t=>{t.exports=function(t,r){return{enumerable:!(1&t),configurable:!(2&t),writable:!(4&t),value:r}}},5449:(t,r,e)=>{\"use strict\";var n=e(6935),o=e(5988),i=e(1887);t.exports=function(t,r,e){var c=n(r);c in t?o.f(t,c,i(0,e)):t[c]=e}},6349:(t,r,e)=>{var n=e(4058),o=e(7457),i=e(1477),c=e(5988).f;t.exports=function(t){var r=n.Symbol||(n.Symbol={});o(r,t)||c(r,t,{value:i.f(t)})}},5746:(t,r,e)=>{var n=e(5981);t.exports=!n((function(){return 7!=Object.defineProperty({},1,{get:function(){return 7}})[1]}))},1333:(t,r,e)=>{var n=e(1899),o=e(941),i=n.document,c=o(i)&&o(i.createElement);t.exports=function(t){return c?i.createElement(t):{}}},6049:(t,r,e)=>{var n=e(2532),o=e(1899);t.exports=\"process\"==n(o.process)},2861:(t,r,e)=>{var n=e(626);t.exports=n(\"navigator\",\"userAgent\")||\"\"},3385:(t,r,e)=>{var n,o,i=e(1899),c=e(2861),u=i.process,a=u&&u.versions,f=a&&a.v8;f?o=(n=f.split(\".\"))[0]+n[1]:c&&(!(n=c.match(/Edge\\/(\\d+)/))||n[1]>=74)&&(n=c.match(/Chrome\\/(\\d+)/))&&(o=n[1]),t.exports=o&&+o},6759:t=>{t.exports=[\"constructor\",\"hasOwnProperty\",\"isPrototypeOf\",\"propertyIsEnumerable\",\"toLocaleString\",\"toString\",\"valueOf\"]},6887:(t,r,e)=>{\"use strict\";var n=e(1899),o=e(9677).f,i=e(7252),c=e(4058),u=e(6843),a=e(2029),f=e(7457),s=function(t){var r=function(r,e,n){if(this instanceof t){switch(arguments.length){case 0:return new t;case 1:return new t(r);case 2:return new t(r,e)}return new t(r,e,n)}return t.apply(this,arguments)};return r.prototype=t.prototype,r};t.exports=function(t,r){var e,p,l,v,y,h,b,g,d=t.target,m=t.global,x=t.stat,w=t.proto,O=m?n:x?n[d]:(n[d]||{}).prototype,S=m?c:c[d]||(c[d]={}),j=S.prototype;for(l in r)e=!i(m?l:d+(x?\".\":\"#\")+l,t.forced)&&O&&f(O,l),y=S[l],e&&(h=t.noTargetGet?(g=o(O,l))&&g.value:O[l]),v=e&&h?h:r[l],e&&typeof y==typeof v||(b=t.bind&&e?u(v,n):t.wrap&&e?s(v):w&&\"function\"==typeof v?u(Function.call,v):v,(t.sham||v&&v.sham||y&&y.sham)&&a(b,\"sham\",!0),S[l]=b,w&&(f(c,p=d+\"Prototype\")||a(c,p,{}),c[p][l]=v,t.real&&j&&!j[l]&&a(j,l,v)))}},5981:t=>{t.exports=function(t){try{return!!t()}catch(t){return!0}}},6843:(t,r,e)=>{var n=e(3916);t.exports=function(t,r,e){if(n(t),void 0===r)return t;switch(e){case 0:return function(){return t.call(r)};case 1:return function(e){return t.call(r,e)};case 2:return function(e,n){return t.call(r,e,n)};case 3:return function(e,n,o){return t.call(r,e,n,o)}}return function(){return t.apply(r,arguments)}}},626:(t,r,e)=>{var n=e(4058),o=e(1899),i=function(t){return\"function\"==typeof t?t:void 0};t.exports=function(t,r){return arguments.length<2?i(n[t])||i(o[t]):n[t]&&n[t][r]||o[t]&&o[t][r]}},1899:(t,r,e)=>{var n=function(t){return t&&t.Math==Math&&t};t.exports=n(\"object\"==typeof globalThis&&globalThis)||n(\"object\"==typeof window&&window)||n(\"object\"==typeof self&&self)||n(\"object\"==typeof e.g&&e.g)||function(){return this}()||Function(\"return this\")()},7457:(t,r,e)=>{var n=e(9678),o={}.hasOwnProperty;t.exports=function(t,r){return o.call(n(t),r)}},7748:t=>{t.exports={}},5463:(t,r,e)=>{var n=e(626);t.exports=n(\"document\",\"documentElement\")},2840:(t,r,e)=>{var n=e(5746),o=e(5981),i=e(1333);t.exports=!n&&!o((function(){return 7!=Object.defineProperty(i(\"div\"),\"a\",{get:function(){return 7}}).a}))},7026:(t,r,e)=>{var n=e(5981),o=e(2532),i=\"\".split;t.exports=n((function(){return!Object(\"z\").propertyIsEnumerable(0)}))?function(t){return\"String\"==o(t)?i.call(t,\"\"):Object(t)}:Object},1302:(t,r,e)=>{var n=e(3030),o=Function.toString;\"function\"!=typeof n.inspectSource&&(n.inspectSource=function(t){return o.call(t)}),t.exports=n.inspectSource},5402:(t,r,e)=>{var n,o,i,c=e(8019),u=e(1899),a=e(941),f=e(2029),s=e(7457),p=e(3030),l=e(4262),v=e(7748),y=\"Object already initialized\",h=u.WeakMap;if(c){var b=p.state||(p.state=new h),g=b.get,d=b.has,m=b.set;n=function(t,r){if(d.call(b,t))throw new TypeError(y);return r.facade=t,m.call(b,t,r),r},o=function(t){return g.call(b,t)||{}},i=function(t){return d.call(b,t)}}else{var x=l(\"state\");v[x]=!0,n=function(t,r){if(s(t,x))throw new TypeError(y);return r.facade=t,f(t,x,r),r},o=function(t){return s(t,x)?t[x]:{}},i=function(t){return s(t,x)}}t.exports={set:n,get:o,has:i,enforce:function(t){return i(t)?o(t):n(t,{})},getterFor:function(t){return function(r){var e;if(!a(r)||(e=o(r)).type!==t)throw TypeError(\"Incompatible receiver, \"+t+\" required\");return e}}}},1052:(t,r,e)=>{var n=e(2532);t.exports=Array.isArray||function(t){return\"Array\"==n(t)}},7252:(t,r,e)=>{var n=e(5981),o=/#|\\.prototype\\./,i=function(t,r){var e=u[c(t)];return e==f||e!=a&&(\"function\"==typeof r?n(r):!!r)},c=i.normalize=function(t){return String(t).replace(o,\".\").toLowerCase()},u=i.data={},a=i.NATIVE=\"N\",f=i.POLYFILL=\"P\";t.exports=i},941:t=>{t.exports=function(t){return\"object\"==typeof t?null!==t:\"function\"==typeof t}},2529:t=>{t.exports=!0},2497:(t,r,e)=>{var n=e(6049),o=e(3385),i=e(5981);t.exports=!!Object.getOwnPropertySymbols&&!i((function(){return!Symbol.sham&&(n?38===o:o>37&&o<41)}))},8019:(t,r,e)=>{var n=e(1899),o=e(1302),i=n.WeakMap;t.exports=\"function\"==typeof i&&/native code/.test(o(i))},9290:(t,r,e)=>{var n,o=e(6059),i=e(9938),c=e(6759),u=e(7748),a=e(5463),f=e(1333),s=e(4262)(\"IE_PROTO\"),p=function(){},l=function(t){return\"<script>\"+t+\"<\\/script>\"},v=function(){try{n=document.domain&&new ActiveXObject(\"htmlfile\")}catch(t){}var t,r;v=n?function(t){t.write(l(\"\")),t.close();var r=t.parentWindow.Object;return t=null,r}(n):((r=f(\"iframe\")).style.display=\"none\",a.appendChild(r),r.src=String(\"javascript:\"),(t=r.contentWindow.document).open(),t.write(l(\"document.F=Object\")),t.close(),t.F);for(var e=c.length;e--;)delete v.prototype[c[e]];return v()};u[s]=!0,t.exports=Object.create||function(t,r){var e;return null!==t?(p.prototype=o(t),e=new p,p.prototype=null,e[s]=t):e=v(),void 0===r?e:i(e,r)}},9938:(t,r,e)=>{var n=e(5746),o=e(5988),i=e(6059),c=e(4771);t.exports=n?Object.defineProperties:function(t,r){i(t);for(var e,n=c(r),u=n.length,a=0;u>a;)o.f(t,e=n[a++],r[e]);return t}},5988:(t,r,e)=>{var n=e(5746),o=e(2840),i=e(6059),c=e(6935),u=Object.defineProperty;r.f=n?u:function(t,r,e){if(i(t),r=c(r,!0),i(e),o)try{return u(t,r,e)}catch(t){}if(\"get\"in e||\"set\"in e)throw TypeError(\"Accessors not supported\");return\"value\"in e&&(t[r]=e.value),t}},9677:(t,r,e)=>{var n=e(5746),o=e(6760),i=e(1887),c=e(4529),u=e(6935),a=e(7457),f=e(2840),s=Object.getOwnPropertyDescriptor;r.f=n?s:function(t,r){if(t=c(t),r=u(r,!0),f)try{return s(t,r)}catch(t){}if(a(t,r))return i(!o.f.call(t,r),t[r])}},684:(t,r,e)=>{var n=e(4529),o=e(946).f,i={}.toString,c=\"object\"==typeof window&&window&&Object.getOwnPropertyNames?Object.getOwnPropertyNames(window):[];t.exports.f=function(t){return c&&\"[object Window]\"==i.call(t)?function(t){try{return o(t)}catch(t){return c.slice()}}(t):o(n(t))}},946:(t,r,e)=>{var n=e(5629),o=e(6759).concat(\"length\",\"prototype\");r.f=Object.getOwnPropertyNames||function(t){return n(t,o)}},7857:(t,r)=>{r.f=Object.getOwnPropertySymbols},249:(t,r,e)=>{var n=e(7457),o=e(9678),i=e(4262),c=e(4160),u=i(\"IE_PROTO\"),a=Object.prototype;t.exports=c?Object.getPrototypeOf:function(t){return t=o(t),n(t,u)?t[u]:\"function\"==typeof t.constructor&&t instanceof t.constructor?t.constructor.prototype:t instanceof Object?a:null}},5629:(t,r,e)=>{var n=e(7457),o=e(4529),i=e(1692).indexOf,c=e(7748);t.exports=function(t,r){var e,u=o(t),a=0,f=[];for(e in u)!n(c,e)&&n(u,e)&&f.push(e);for(;r.length>a;)n(u,e=r[a++])&&(~i(f,e)||f.push(e));return f}},4771:(t,r,e)=>{var n=e(5629),o=e(6759);t.exports=Object.keys||function(t){return n(t,o)}},6760:(t,r)=>{\"use strict\";var e={}.propertyIsEnumerable,n=Object.getOwnPropertyDescriptor,o=n&&!e.call({1:2},1);r.f=o?function(t){var r=n(this,t);return!!r&&r.enumerable}:e},5623:(t,r,e)=>{\"use strict\";var n=e(2885),o=e(9697);t.exports=n?{}.toString:function(){return\"[object \"+o(this)+\"]\"}},4058:t=>{t.exports={}},9754:(t,r,e)=>{var n=e(2029);t.exports=function(t,r,e,o){o&&o.enumerable?t[r]=e:n(t,r,e)}},8219:t=>{t.exports=function(t){if(null==t)throw TypeError(\"Can't call method on \"+t);return t}},4911:(t,r,e)=>{var n=e(1899),o=e(2029);t.exports=function(t,r){try{o(n,t,r)}catch(e){n[t]=r}return r}},904:(t,r,e)=>{var n=e(2885),o=e(5988).f,i=e(2029),c=e(7457),u=e(5623),a=e(9813)(\"toStringTag\");t.exports=function(t,r,e,f){if(t){var s=e?t:t.prototype;c(s,a)||o(s,a,{configurable:!0,value:r}),f&&!n&&i(s,\"toString\",u)}}},4262:(t,r,e)=>{var n=e(8726),o=e(9418),i=n(\"keys\");t.exports=function(t){return i[t]||(i[t]=o(t))}},3030:(t,r,e)=>{var n=e(1899),o=e(4911),i=\"__core-js_shared__\",c=n[i]||o(i,{});t.exports=c},8726:(t,r,e)=>{var n=e(2529),o=e(3030);(t.exports=function(t,r){return o[t]||(o[t]=void 0!==r?r:{})})(\"versions\",[]).push({version:\"3.11.1\",mode:n?\"pure\":\"global\",copyright:\"© 2021 Denis Pushkarev (zloirock.ru)\"})},9413:(t,r,e)=>{var n=e(8459),o=Math.max,i=Math.min;t.exports=function(t,r){var e=n(t);return e<0?o(e+r,0):i(e,r)}},4529:(t,r,e)=>{var n=e(7026),o=e(8219);t.exports=function(t){return n(o(t))}},8459:t=>{var r=Math.ceil,e=Math.floor;t.exports=function(t){return isNaN(t=+t)?0:(t>0?e:r)(t)}},3057:(t,r,e)=>{var n=e(8459),o=Math.min;t.exports=function(t){return t>0?o(n(t),9007199254740991):0}},9678:(t,r,e)=>{var n=e(8219);t.exports=function(t){return Object(n(t))}},6935:(t,r,e)=>{var n=e(941);t.exports=function(t,r){if(!n(t))return t;var e,o;if(r&&\"function\"==typeof(e=t.toString)&&!n(o=e.call(t)))return o;if(\"function\"==typeof(e=t.valueOf)&&!n(o=e.call(t)))return o;if(!r&&\"function\"==typeof(e=t.toString)&&!n(o=e.call(t)))return o;throw TypeError(\"Can't convert object to primitive value\")}},2885:(t,r,e)=>{var n={};n[e(9813)(\"toStringTag\")]=\"z\",t.exports=\"[object z]\"===String(n)},9418:t=>{var r=0,e=Math.random();t.exports=function(t){return\"Symbol(\"+String(void 0===t?\"\":t)+\")_\"+(++r+e).toString(36)}},2302:(t,r,e)=>{var n=e(2497);t.exports=n&&!Symbol.sham&&\"symbol\"==typeof Symbol.iterator},1477:(t,r,e)=>{var n=e(9813);r.f=n},9813:(t,r,e)=>{var n=e(1899),o=e(8726),i=e(7457),c=e(9418),u=e(2497),a=e(2302),f=o(\"wks\"),s=n.Symbol,p=a?s:s&&s.withoutSetter||c;t.exports=function(t){return i(f,t)&&(u||\"string\"==typeof f[t])||(u&&i(s,t)?f[t]=s[t]:f[t]=p(\"Symbol.\"+t)),f[t]}},8922:(t,r,e)=>{\"use strict\";var n=e(6887),o=e(5981),i=e(1052),c=e(941),u=e(9678),a=e(3057),f=e(5449),s=e(4692),p=e(568),l=e(9813),v=e(3385),y=l(\"isConcatSpreadable\"),h=9007199254740991,b=\"Maximum allowed index exceeded\",g=v>=51||!o((function(){var t=[];return t[y]=!1,t.concat()[0]!==t})),d=p(\"concat\"),m=function(t){if(!c(t))return!1;var r=t[y];return void 0!==r?!!r:i(t)};n({target:\"Array\",proto:!0,forced:!g||!d},{concat:function(t){var r,e,n,o,i,c=u(this),p=s(c,0),l=0;for(r=-1,n=arguments.length;r<n;r++)if(m(i=-1===r?c:arguments[r])){if(l+(o=a(i.length))>h)throw TypeError(b);for(e=0;e<o;e++,l++)e in i&&f(p,l,i[e])}else{if(l>=h)throw TypeError(b);f(p,l++,i)}return p.length=l,p}})},7633:(t,r,e)=>{\"use strict\";var n=e(941),o=e(5988),i=e(249),c=e(9813)(\"hasInstance\"),u=Function.prototype;c in u||o.f(u,c,{value:function(t){if(\"function\"!=typeof this||!n(t))return!1;if(!n(this.prototype))return t instanceof this;for(;t=i(t);)if(this.prototype===t)return!0;return!1}})},9120:(t,r,e)=>{var n=e(1899);e(904)(n.JSON,\"JSON\",!0)},5327:()=>{},5967:()=>{},1502:()=>{},8555:(t,r,e)=>{e(6349)(\"asyncIterator\")},2615:()=>{},1732:(t,r,e)=>{e(6349)(\"hasInstance\")},5903:(t,r,e)=>{e(6349)(\"isConcatSpreadable\")},1825:(t,r,e)=>{e(6349)(\"iterator\")},5824:(t,r,e)=>{\"use strict\";var n=e(6887),o=e(1899),i=e(626),c=e(2529),u=e(5746),a=e(2497),f=e(2302),s=e(5981),p=e(7457),l=e(1052),v=e(941),y=e(6059),h=e(9678),b=e(4529),g=e(6935),d=e(1887),m=e(9290),x=e(4771),w=e(946),O=e(684),S=e(7857),j=e(9677),P=e(5988),E=e(6760),T=e(2029),I=e(9754),A=e(8726),N=e(4262),k=e(7748),M=e(9418),F=e(9813),_=e(1477),C=e(6349),z=e(904),L=e(5402),W=e(3610).forEach,D=N(\"hidden\"),J=\"Symbol\",q=F(\"toPrimitive\"),R=L.set,B=L.getterFor(J),G=Object.prototype,Q=o.Symbol,U=i(\"JSON\",\"stringify\"),V=j.f,X=P.f,Y=O.f,H=E.f,K=A(\"symbols\"),Z=A(\"op-symbols\"),$=A(\"string-to-symbol-registry\"),tt=A(\"symbol-to-string-registry\"),rt=A(\"wks\"),et=o.QObject,nt=!et||!et.prototype||!et.prototype.findChild,ot=u&&s((function(){return 7!=m(X({},\"a\",{get:function(){return X(this,\"a\",{value:7}).a}})).a}))?function(t,r,e){var n=V(G,r);n&&delete G[r],X(t,r,e),n&&t!==G&&X(G,r,n)}:X,it=function(t,r){var e=K[t]=m(Q.prototype);return R(e,{type:J,tag:t,description:r}),u||(e.description=r),e},ct=f?function(t){return\"symbol\"==typeof t}:function(t){return Object(t)instanceof Q},ut=function(t,r,e){t===G&&ut(Z,r,e),y(t);var n=g(r,!0);return y(e),p(K,n)?(e.enumerable?(p(t,D)&&t[D][n]&&(t[D][n]=!1),e=m(e,{enumerable:d(0,!1)})):(p(t,D)||X(t,D,d(1,{})),t[D][n]=!0),ot(t,n,e)):X(t,n,e)},at=function(t,r){y(t);var e=b(r),n=x(e).concat(lt(e));return W(n,(function(r){u&&!ft.call(e,r)||ut(t,r,e[r])})),t},ft=function(t){var r=g(t,!0),e=H.call(this,r);return!(this===G&&p(K,r)&&!p(Z,r))&&(!(e||!p(this,r)||!p(K,r)||p(this,D)&&this[D][r])||e)},st=function(t,r){var e=b(t),n=g(r,!0);if(e!==G||!p(K,n)||p(Z,n)){var o=V(e,n);return!o||!p(K,n)||p(e,D)&&e[D][n]||(o.enumerable=!0),o}},pt=function(t){var r=Y(b(t)),e=[];return W(r,(function(t){p(K,t)||p(k,t)||e.push(t)})),e},lt=function(t){var r=t===G,e=Y(r?Z:b(t)),n=[];return W(e,(function(t){!p(K,t)||r&&!p(G,t)||n.push(K[t])})),n};a||(I((Q=function(){if(this instanceof Q)throw TypeError(\"Symbol is not a constructor\");var t=arguments.length&&void 0!==arguments[0]?String(arguments[0]):void 0,r=M(t),e=function(t){this===G&&e.call(Z,t),p(this,D)&&p(this[D],r)&&(this[D][r]=!1),ot(this,r,d(1,t))};return u&&nt&&ot(G,r,{configurable:!0,set:e}),it(r,t)}).prototype,\"toString\",(function(){return B(this).tag})),I(Q,\"withoutSetter\",(function(t){return it(M(t),t)})),E.f=ft,P.f=ut,j.f=st,w.f=O.f=pt,S.f=lt,_.f=function(t){return it(F(t),t)},u&&(X(Q.prototype,\"description\",{configurable:!0,get:function(){return B(this).description}}),c||I(G,\"propertyIsEnumerable\",ft,{unsafe:!0}))),n({global:!0,wrap:!0,forced:!a,sham:!a},{Symbol:Q}),W(x(rt),(function(t){C(t)})),n({target:J,stat:!0,forced:!a},{for:function(t){var r=String(t);if(p($,r))return $[r];var e=Q(r);return $[r]=e,tt[e]=r,e},keyFor:function(t){if(!ct(t))throw TypeError(t+\" is not a symbol\");if(p(tt,t))return tt[t]},useSetter:function(){nt=!0},useSimple:function(){nt=!1}}),n({target:\"Object\",stat:!0,forced:!a,sham:!u},{create:function(t,r){return void 0===r?m(t):at(m(t),r)},defineProperty:ut,defineProperties:at,getOwnPropertyDescriptor:st}),n({target:\"Object\",stat:!0,forced:!a},{getOwnPropertyNames:pt,getOwnPropertySymbols:lt}),n({target:\"Object\",stat:!0,forced:s((function(){S.f(1)}))},{getOwnPropertySymbols:function(t){return S.f(h(t))}}),U&&n({target:\"JSON\",stat:!0,forced:!a||s((function(){var t=Q();return\"[null]\"!=U([t])||\"{}\"!=U({a:t})||\"{}\"!=U(Object(t))}))},{stringify:function(t,r,e){for(var n,o=[t],i=1;arguments.length>i;)o.push(arguments[i++]);if(n=r,(v(r)||void 0!==t)&&!ct(t))return l(r)||(r=function(t,r){if(\"function\"==typeof n&&(r=n.call(this,t,r)),!ct(r))return r}),o[1]=r,U.apply(null,o)}}),Q.prototype[q]||T(Q.prototype,q,Q.prototype.valueOf),z(Q,J),k[D]=!0},5915:(t,r,e)=>{e(6349)(\"matchAll\")},8394:(t,r,e)=>{e(6349)(\"match\")},1766:(t,r,e)=>{e(6349)(\"replace\")},2737:(t,r,e)=>{e(6349)(\"search\")},9911:(t,r,e)=>{e(6349)(\"species\")},4315:(t,r,e)=>{e(6349)(\"split\")},3131:(t,r,e)=>{e(6349)(\"toPrimitive\")},4714:(t,r,e)=>{e(6349)(\"toStringTag\")},659:(t,r,e)=>{e(6349)(\"unscopables\")},6952:(t,r,e)=>{var n=e(7604);t.exports=n},2547:(t,r,e)=>{var n=e(7473);t.exports=n}},r={};function e(n){var o=r[n];if(void 0!==o)return o.exports;var i=r[n]={exports:{}};return t[n](i,i.exports,e),i.exports}e.n=t=>{var r=t&&t.__esModule?()=>t.default:()=>t;return e.d(r,{a:r}),r},e.d=(t,r)=>{for(var n in r)e.o(r,n)&&!e.o(t,n)&&Object.defineProperty(t,n,{enumerable:!0,get:r[n]})},e.g=function(){if(\"object\"==typeof globalThis)return globalThis;try{return this||new Function(\"return this\")()}catch(t){if(\"object\"==typeof window)return window}}(),e.o=(t,r)=>Object.prototype.hasOwnProperty.call(t,r),(()=>{\"use strict\";var t=e(2547),r=e.n(t),n=e(6952),o=e.n(n);if(void 0!==r()&&\"hasInstance\"in r()&&\"symbol\"==typeof o()){var i=function(t,r){globalThis.assert.strictEqual(globalThis.addon.doInstanceOf(t,r),t instanceof r)},c=function(){},u=function(){};Object.defineProperty(c,o(),{value:t=>\"mark\"in t}),u.prototype=new c;var a=new u,f=new u;a.mark=!0,i(a,u),i(f,u),i(a,c),i(f,c),a=new c,f=new c,a.mark=!0,i(a,u),i(f,u),i(a,c),i(f,c)}})()})();",
                                         "https://www.didi.com/test_general_testInstanceOf.js",
                                         &result), NAPIOK);

    ASSERT_EQ(NAPIRunScriptWithSourceUrl(globalEnv,
                                         "(()=>{\"use strict\";var s=globalThis.addon.testNapiRun(\"(41.92 + 0.08);\");globalThis.assert.strictEqual(s,42),globalThis.assert.throws((function(){return globalThis.addon.testNapiRun({abc:\"def\"})}))})();",
                                         "https://www.didi.com/test_general_testNapiRun.js",
                                         &result), NAPIOK);

    ASSERT_EQ(NAPIRunScriptWithSourceUrl(globalEnv,
                                         "(()=>{\"use strict\";globalThis.addon.createNapiError(),globalThis.assert(globalThis.addon.testNapiErrorCleanup(),\"napi_status cleaned up for second call\")})();",
                                         "https://www.didi.com/test_general_testNapiStatus.js",
                                         &result), NAPIOK);
}