#include <common.h>
#include <js_native_api_test.h>

EXTERN_C_START

static NAPIValue TestUtf8(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValueType valuetype;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));

    NAPI_ASSERT(env, valuetype == NAPIString, "Wrong type of argment. Expects a string.");

    char buffer[128];
    size_t buffer_size = 128;
    size_t copied;

    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], buffer, buffer_size, &copied));

    NAPIValue output;
    NAPI_CALL(env, napi_create_string_utf8(env, buffer, -1, &output));

    return output;
}

static NAPIValue TestUtf16(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValueType valuetype;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));

    NAPI_ASSERT(env, valuetype == NAPIString, "Wrong type of argment. Expects a string.");

    char16_t buffer[128];
    size_t buffer_size = 128;
    size_t copied;

    NAPI_CALL(env, napi_get_value_string_utf16(env, args[0], buffer, buffer_size, &copied));

    NAPIValue output;
    NAPI_CALL(env, napi_create_string_utf16(env, buffer, copied, &output));

    return output;
}

static NAPIValue TestUtf8Insufficient(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValueType valuetype;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));

    NAPI_ASSERT(env, valuetype == NAPIString, "Wrong type of argment. Expects a string.");

    char buffer[4];
    size_t buffer_size = 4;
    size_t copied;

    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], buffer, buffer_size, &copied));

    NAPIValue output;
    NAPI_CALL(env, napi_create_string_utf8(env, buffer, -1, &output));

    return output;
}

static NAPIValue TestUtf16Insufficient(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValueType valuetype;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));

    NAPI_ASSERT(env, valuetype == NAPIString, "Wrong type of argment. Expects a string.");

    char16_t buffer[4];
    size_t buffer_size = 4;
    size_t copied;

    NAPI_CALL(env, napi_get_value_string_utf16(env, args[0], buffer, buffer_size, &copied));

    NAPIValue output;
    NAPI_CALL(env, napi_create_string_utf16(env, buffer, copied, &output));

    return output;
}

static NAPIValue Utf16Length(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValueType valuetype;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));

    NAPI_ASSERT(env, valuetype == NAPIString, "Wrong type of argment. Expects a string.");

    size_t length;
    NAPI_CALL(env, napi_get_value_string_utf16(env, args[0], nullptr, 0, &length));

    NAPIValue output;
    NAPI_CALL(env, napi_create_uint32(env, (uint32_t)length, &output));

    return output;
}

static NAPIValue Utf8Length(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValueType valuetype;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));

    NAPI_ASSERT(env, valuetype == NAPIString, "Wrong type of argment. Expects a string.");

    size_t length;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], nullptr, 0, &length));

    NAPIValue output;
    NAPI_CALL(env, napi_create_uint32(env, (uint32_t)length, &output));

    return output;
}

static NAPIValue TestMemoryCorruption(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc == 1, "Wrong number of arguments");

    char buf[10] = {0};
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], buf, 0, nullptr));

    char zero[10] = {0};
    if (memcmp(buf, zero, sizeof(buf)) != 0)
    {
        NAPI_CALL(env, napi_throw_error(env, nullptr, "Buffer overwritten"));
    }

    return nullptr;
}

EXTERN_C_END

TEST_F(Test, TestString)
{
    NAPIPropertyDescriptor properties[] = {
        DECLARE_NAPI_PROPERTY("TestUtf8", TestUtf8),
        DECLARE_NAPI_PROPERTY("TestUtf8Insufficient", TestUtf8Insufficient),
        DECLARE_NAPI_PROPERTY("TestUtf16", TestUtf16),
        DECLARE_NAPI_PROPERTY("TestUtf16Insufficient", TestUtf16Insufficient),
        DECLARE_NAPI_PROPERTY("Utf16Length", Utf16Length),
        DECLARE_NAPI_PROPERTY("Utf8Length", Utf8Length),
        DECLARE_NAPI_PROPERTY("TestMemoryCorruption", TestMemoryCorruption),
    };

    ASSERT_EQ(napi_define_properties(globalEnv, exports, sizeof(properties) / sizeof(*properties), properties), NAPIOK);

    NAPIValue result;
    ASSERT_EQ(
        NAPIRunScriptWithSourceUrl(
            globalEnv,
            "(()=>{var t={4900:(t,r,e)=>{e(186);var o=e(5703);t.exports=o(\"Array\").slice},9601:(t,r,e)=>{var "
            "o=e(4900),a=Array.prototype;t.exports=function(t){var r=t.slice;return t===a||t instanceof "
            "Array&&r===a.slice?o:r}},3916:t=>{t.exports=function(t){if(\"function\"!=typeof t)throw "
            "TypeError(String(t)+\" is not a function\");return t}},6059:(t,r,e)=>{var "
            "o=e(941);t.exports=function(t){if(!o(t))throw TypeError(String(t)+\" is not an object\");return "
            "t}},568:(t,r,e)=>{var o=e(5981),a=e(9813),s=e(3385),n=a(\"species\");t.exports=function(t){return "
            "s>=51||!o((function(){var "
            "r=[];return(r.constructor={})[n]=function(){return{foo:1}},1!==r[t](Boolean).foo}))}},2532:t=>{var "
            "r={}.toString;t.exports=function(t){return r.call(t).slice(8,-1)}},2029:(t,r,e)=>{var "
            "o=e(5746),a=e(5988),s=e(1887);t.exports=o?function(t,r,e){return a.f(t,r,s(1,e))}:function(t,r,e){return "
            "t[r]=e,t}},1887:t=>{t.exports=function(t,r){return{enumerable:!(1&t),configurable:!(2&t),writable:!(4&t),"
            "value:r}}},5449:(t,r,e)=>{\"use strict\";var o=e(6935),a=e(5988),s=e(1887);t.exports=function(t,r,e){var "
            "n=o(r);n in t?a.f(t,n,s(0,e)):t[n]=e}},5746:(t,r,e)=>{var o=e(5981);t.exports=!o((function(){return "
            "7!=Object.defineProperty({},1,{get:function(){return 7}})[1]}))},1333:(t,r,e)=>{var "
            "o=e(1899),a=e(941),s=o.document,n=a(s)&&a(s.createElement);t.exports=function(t){return "
            "n?s.createElement(t):{}}},6049:(t,r,e)=>{var "
            "o=e(2532),a=e(1899);t.exports=\"process\"==o(a.process)},2861:(t,r,e)=>{var "
            "o=e(626);t.exports=o(\"navigator\",\"userAgent\")||\"\"},3385:(t,r,e)=>{var "
            "o,a,s=e(1899),n=e(2861),l=s.process,i=l&&l.versions,c=i&&i.v8;c?a=(o=c.split(\".\"))[0]+o[1]:n&&(!(o=n."
            "match(/Edge\\/(\\d+)/))||o[1]>=74)&&(o=n.match(/Chrome\\/(\\d+)/"
            "))&&(a=o[1]),t.exports=a&&+a},5703:(t,r,e)=>{var o=e(4058);t.exports=function(t){return "
            "o[t+\"Prototype\"]}},6887:(t,r,e)=>{\"use strict\";var "
            "o=e(1899),a=e(9677).f,s=e(7252),n=e(4058),l=e(6843),i=e(2029),c=e(7457),u=function(t){var "
            "r=function(r,e,o){if(this instanceof t){switch(arguments.length){case 0:return new t;case 1:return new "
            "t(r);case 2:return new t(r,e)}return new t(r,e,o)}return t.apply(this,arguments)};return "
            "r.prototype=t.prototype,r};t.exports=function(t,r){var "
            "e,f,h,g,p,b,d,T,v=t.target,y=t.global,E=t.stat,x=t.proto,q=y?o:E?o[v]:(o[v]||{}).prototype,U=y?n:n[v]||(n["
            "v]={}),m=U.prototype;for(h in "
            "r)e=!s(y?h:v+(E?\".\":\"#\")+h,t.forced)&&q&&c(q,h),p=U[h],e&&(b=t.noTargetGet?(T=a(q,h))&&T.value:q[h]),"
            "g=e&&b?b:r[h],e&&typeof p==typeof g||(d=t.bind&&e?l(g,o):t.wrap&&e?u(g):x&&\"function\"==typeof "
            "g?l(Function.call,g):g,(t.sham||g&&g.sham||p&&p.sham)&&i(d,\"sham\",!0),U[h]=d,x&&(c(n,f=v+\"Prototype\")|"
            "|i(n,f,{}),n[f][h]=g,t.real&&m&&!m[h]&&i(m,h,g)))}},5981:t=>{t.exports=function(t){try{return!!t()}catch("
            "t){return!0}}},6843:(t,r,e)=>{var o=e(3916);t.exports=function(t,r,e){if(o(t),void 0===r)return "
            "t;switch(e){case 0:return function(){return t.call(r)};case 1:return function(e){return t.call(r,e)};case "
            "2:return function(e,o){return t.call(r,e,o)};case 3:return function(e,o,a){return t.call(r,e,o,a)}}return "
            "function(){return t.apply(r,arguments)}}},626:(t,r,e)=>{var "
            "o=e(4058),a=e(1899),s=function(t){return\"function\"==typeof t?t:void 0};t.exports=function(t,r){return "
            "arguments.length<2?s(o[t])||s(a[t]):o[t]&&o[t][r]||a[t]&&a[t][r]}},1899:(t,r,e)=>{var "
            "o=function(t){return t&&t.Math==Math&&t};t.exports=o(\"object\"==typeof "
            "globalThis&&globalThis)||o(\"object\"==typeof window&&window)||o(\"object\"==typeof "
            "self&&self)||o(\"object\"==typeof e.g&&e.g)||function(){return this}()||Function(\"return "
            "this\")()},7457:(t,r,e)=>{var o=e(9678),a={}.hasOwnProperty;t.exports=function(t,r){return "
            "a.call(o(t),r)}},2840:(t,r,e)=>{var o=e(5746),a=e(5981),s=e(1333);t.exports=!o&&!a((function(){return "
            "7!=Object.defineProperty(s(\"div\"),\"a\",{get:function(){return 7}}).a}))},7026:(t,r,e)=>{var "
            "o=e(5981),a=e(2532),s=\"\".split;t.exports=o((function(){return!Object(\"z\").propertyIsEnumerable(0)}))?"
            "function(t){return\"String\"==a(t)?s.call(t,\"\"):Object(t)}:Object},1052:(t,r,e)=>{var "
            "o=e(2532);t.exports=Array.isArray||function(t){return\"Array\"==o(t)}},7252:(t,r,e)=>{var "
            "o=e(5981),a=/#|\\.prototype\\./,s=function(t,r){var e=l[n(t)];return e==c||e!=i&&(\"function\"==typeof "
            "r?o(r):!!r)},n=s.normalize=function(t){return "
            "String(t).replace(a,\".\").toLowerCase()},l=s.data={},i=s.NATIVE=\"N\",c=s.POLYFILL=\"P\";t.exports=s},"
            "941:t=>{t.exports=function(t){return\"object\"==typeof t?null!==t:\"function\"==typeof "
            "t}},2529:t=>{t.exports=!0},2497:(t,r,e)=>{var "
            "o=e(6049),a=e(3385),s=e(5981);t.exports=!!Object.getOwnPropertySymbols&&!s((function(){return!Symbol.sham&"
            "&(o?38===a:a>37&&a<41)}))},5988:(t,r,e)=>{var "
            "o=e(5746),a=e(2840),s=e(6059),n=e(6935),l=Object.defineProperty;r.f=o?l:function(t,r,e){if(s(t),r=n(r,!0),"
            "s(e),a)try{return l(t,r,e)}catch(t){}if(\"get\"in e||\"set\"in e)throw TypeError(\"Accessors not "
            "supported\");return\"value\"in e&&(t[r]=e.value),t}},9677:(t,r,e)=>{var "
            "o=e(5746),a=e(6760),s=e(1887),n=e(4529),l=e(6935),i=e(7457),c=e(2840),u=Object.getOwnPropertyDescriptor;r."
            "f=o?u:function(t,r){if(t=n(t),r=l(r,!0),c)try{return u(t,r)}catch(t){}if(i(t,r))return "
            "s(!a.f.call(t,r),t[r])}},6760:(t,r)=>{\"use strict\";var "
            "e={}.propertyIsEnumerable,o=Object.getOwnPropertyDescriptor,a=o&&!e.call({1:2},1);r.f=a?function(t){var "
            "r=o(this,t);return!!r&&r.enumerable}:e},4058:t=>{t.exports={}},8219:t=>{t.exports=function(t){if(null==t)"
            "throw TypeError(\"Can't call method on \"+t);return t}},4911:(t,r,e)=>{var "
            "o=e(1899),a=e(2029);t.exports=function(t,r){try{a(o,t,r)}catch(e){o[t]=r}return r}},3030:(t,r,e)=>{var "
            "o=e(1899),a=e(4911),s=\"__core-js_shared__\",n=o[s]||a(s,{});t.exports=n},8726:(t,r,e)=>{var "
            "o=e(2529),a=e(3030);(t.exports=function(t,r){return a[t]||(a[t]=void "
            "0!==r?r:{})})(\"versions\",[]).push({version:\"3.11.1\",mode:o?\"pure\":\"global\",copyright:\"© 2021 "
            "Denis Pushkarev (zloirock.ru)\"})},9413:(t,r,e)=>{var "
            "o=e(8459),a=Math.max,s=Math.min;t.exports=function(t,r){var e=o(t);return "
            "e<0?a(e+r,0):s(e,r)}},4529:(t,r,e)=>{var o=e(7026),a=e(8219);t.exports=function(t){return "
            "o(a(t))}},8459:t=>{var r=Math.ceil,e=Math.floor;t.exports=function(t){return "
            "isNaN(t=+t)?0:(t>0?e:r)(t)}},3057:(t,r,e)=>{var o=e(8459),a=Math.min;t.exports=function(t){return "
            "t>0?a(o(t),9007199254740991):0}},9678:(t,r,e)=>{var o=e(8219);t.exports=function(t){return "
            "Object(o(t))}},6935:(t,r,e)=>{var o=e(941);t.exports=function(t,r){if(!o(t))return t;var "
            "e,a;if(r&&\"function\"==typeof(e=t.toString)&&!o(a=e.call(t)))return "
            "a;if(\"function\"==typeof(e=t.valueOf)&&!o(a=e.call(t)))return "
            "a;if(!r&&\"function\"==typeof(e=t.toString)&&!o(a=e.call(t)))return a;throw TypeError(\"Can't convert "
            "object to primitive value\")}},9418:t=>{var "
            "r=0,e=Math.random();t.exports=function(t){return\"Symbol(\"+String(void "
            "0===t?\"\":t)+\")_\"+(++r+e).toString(36)}},2302:(t,r,e)=>{var "
            "o=e(2497);t.exports=o&&!Symbol.sham&&\"symbol\"==typeof Symbol.iterator},9813:(t,r,e)=>{var "
            "o=e(1899),a=e(8726),s=e(7457),n=e(9418),l=e(2497),i=e(2302),c=a(\"wks\"),u=o.Symbol,f=i?u:u&&u."
            "withoutSetter||n;t.exports=function(t){return s(c,t)&&(l||\"string\"==typeof "
            "c[t])||(l&&s(u,t)?c[t]=u[t]:c[t]=f(\"Symbol.\"+t)),c[t]}},186:(t,r,e)=>{\"use strict\";var "
            "o=e(6887),a=e(941),s=e(1052),n=e(9413),l=e(3057),i=e(4529),c=e(5449),u=e(9813),f=e(568)(\"slice\"),h=u("
            "\"species\"),g=[].slice,p=Math.max;o({target:\"Array\",proto:!0,forced:!f},{slice:function(t,r){var "
            "e,o,u,f=i(this),b=l(f.length),d=n(t,b),T=n(void "
            "0===r?b:r,b);if(s(f)&&(\"function\"!=typeof(e=f.constructor)||e!==Array&&!s(e.prototype)?a(e)&&null===(e="
            "e[h])&&(e=void 0):e=void 0,e===Array||void 0===e))return g.call(f,d,T);for(o=new(void "
            "0===e?Array:e)(p(T-d,0)),u=0;d<T;d++,u++)d in f&&c(o,u,f[d]);return o.length=u,o}})},2073:(t,r,e)=>{var "
            "o=e(9601);t.exports=o}},r={};function e(o){var a=r[o];if(void 0!==a)return a.exports;var "
            "s=r[o]={exports:{}};return t[o](s,s.exports,e),s.exports}e.n=t=>{var "
            "r=t&&t.__esModule?()=>t.default:()=>t;return e.d(r,{a:r}),r},e.d=(t,r)=>{for(var o in "
            "r)e.o(r,o)&&!e.o(t,o)&&Object.defineProperty(t,o,{enumerable:!0,get:r[o]})},e.g=function(){if(\"object\"=="
            "typeof globalThis)return globalThis;try{return this||new Function(\"return "
            "this\")()}catch(t){if(\"object\"==typeof window)return "
            "window}}(),e.o=(t,r)=>Object.prototype.hasOwnProperty.call(t,r),(()=>{\"use strict\";var "
            "t=e(2073),r=e.n(t),o=\"\";globalThis.assert.strictEqual(globalThis.addon.TestUtf8(o),o),globalThis.assert."
            "strictEqual(globalThis.addon.TestUtf16(o),o),globalThis.assert.strictEqual(globalThis.addon.Utf16Length(o)"
            ",0),globalThis.assert.strictEqual(globalThis.addon.Utf8Length(o),0);var a=\"hello "
            "world\";globalThis.assert.strictEqual(globalThis.addon.TestUtf8(a),a),globalThis.assert.strictEqual("
            "globalThis.addon.TestUtf16(a),a),globalThis.assert.strictEqual(globalThis.addon.TestUtf8Insufficient(a),r("
            ")(a).call(a,0,3)),globalThis.assert.strictEqual(globalThis.addon.TestUtf16Insufficient(a),r()(a).call(a,0,"
            "3)),globalThis.assert.strictEqual(globalThis.addon.Utf16Length(a),11),globalThis.assert.strictEqual("
            "globalThis.addon.Utf8Length(a),11);var "
            "s=\"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789\";globalThis.assert.strictEqual("
            "globalThis.addon.TestUtf8(s),s),globalThis.assert.strictEqual(globalThis.addon.TestUtf16(s),s),globalThis."
            "assert.strictEqual(globalThis.addon.TestUtf8Insufficient(s),r()(s).call(s,0,3)),globalThis.assert."
            "strictEqual(globalThis.addon.TestUtf16Insufficient(s),r()(s).call(s,0,3)),globalThis.assert.strictEqual("
            "globalThis.addon.Utf16Length(s),62),globalThis.assert.strictEqual(globalThis.addon.Utf8Length(s),62);var "
            "n=\"?!@#$%^&*()_+-=[]{}/"
            ".,<>'\\\"\\\\\";globalThis.assert.strictEqual(globalThis.addon.TestUtf8(n),n),globalThis.assert."
            "strictEqual(globalThis.addon.TestUtf16(n),n),globalThis.assert.strictEqual(globalThis.addon."
            "TestUtf8Insufficient(n),r()(n).call(n,0,3)),globalThis.assert.strictEqual(globalThis.addon."
            "TestUtf16Insufficient(n),r()(n).call(n,0,3)),globalThis.assert.strictEqual(globalThis.addon.Utf16Length(n)"
            ",27),globalThis.assert.strictEqual(globalThis.addon.Utf8Length(n),27);var "
            "l=\"¡¢£¤¥¦§¨©ª«¬\u00AD®¯°±²³´µ¶·¸¹º»¼½¾¿\";globalThis.assert.strictEqual(globalThis.addon.TestUtf8(l),l),"
            "globalThis.assert.strictEqual(globalThis.addon.TestUtf16(l),l),globalThis.assert.strictEqual(globalThis."
            "addon.TestUtf8Insufficient(l),r()(l).call(l,0,1)),globalThis.assert.strictEqual(globalThis.addon."
            "TestUtf16Insufficient(l),r()(l).call(l,0,3)),globalThis.assert.strictEqual(globalThis.addon.Utf16Length(l)"
            ",31),globalThis.assert.strictEqual(globalThis.addon.Utf8Length(l),62);var "
            "i=\"ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþ\";globalThis.assert.strictEqual("
            "globalThis.addon.TestUtf8(i),i),globalThis.assert.strictEqual(globalThis.addon.TestUtf16(i),i),globalThis."
            "assert.strictEqual(globalThis.addon.TestUtf8Insufficient(i),r()(i).call(i,0,1)),globalThis.assert."
            "strictEqual(globalThis.addon.TestUtf16Insufficient(i),r()(i).call(i,0,3)),globalThis.assert.strictEqual("
            "globalThis.addon.Utf16Length(i),63),globalThis.assert.strictEqual(globalThis.addon.Utf8Length(i),126);var "
            "c=\" ℁ Ȃ‑\";globalThis.assert.strictEqual(globalThis.addon.TestUtf8(c),c),globalThis.assert.strictEqual("
            "globalThis.addon.TestUtf16(c),c),globalThis.assert.strictEqual(globalThis.addon.TestUtf8Insufficient(c),r("
            ")(c).call(c,0,1)),globalThis.assert.strictEqual(globalThis.addon.TestUtf16Insufficient(c),r()(c).call(c,0,"
            "3)),globalThis.assert.strictEqual(globalThis.addon.Utf16Length(c),5),globalThis.assert.strictEqual("
            "globalThis.addon.Utf8Length(c),14),globalThis.addon.TestMemoryCorruption(\" \".repeat(65536))})()})();",
            "https://www.didi.com/test_string.js", &result),
        NAPIOK);
}