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
        DECLARE_NAPI_PROPERTY("Utf8Length", Utf8Length),
        DECLARE_NAPI_PROPERTY("TestMemoryCorruption", TestMemoryCorruption),
    };

    ASSERT_EQ(napi_define_properties(globalEnv, exports, sizeof(properties) / sizeof(*properties), properties), NAPIOK);

    NAPIValue result;
    ASSERT_EQ(
        NAPIRunScript(
            globalEnv,
            "(()=>{var t={4900:(t,r,e)=>{e(186);var o=e(5703);t.exports=o(\"Array\").slice},9601:(t,r,e)=>{var "
            "o=e(4900),n=Array.prototype;t.exports=function(t){var r=t.slice;return t===n||t instanceof "
            "Array&&r===n.slice?o:r}},3916:t=>{t.exports=function(t){if(\"function\"!=typeof t)throw "
            "TypeError(String(t)+\" is not a function\");return t}},6059:(t,r,e)=>{var "
            "o=e(941);t.exports=function(t){if(!o(t))throw TypeError(String(t)+\" is not an object\");return "
            "t}},568:(t,r,e)=>{var o=e(5981),n=e(9813),a=e(3385),s=n(\"species\");t.exports=function(t){return "
            "a>=51||!o((function(){var "
            "r=[];return(r.constructor={})[s]=function(){return{foo:1}},1!==r[t](Boolean).foo}))}},2532:t=>{var "
            "r={}.toString;t.exports=function(t){return r.call(t).slice(8,-1)}},2029:(t,r,e)=>{var "
            "o=e(5746),n=e(5988),a=e(1887);t.exports=o?function(t,r,e){return n.f(t,r,a(1,e))}:function(t,r,e){return "
            "t[r]=e,t}},1887:t=>{t.exports=function(t,r){return{enumerable:!(1&t),configurable:!(2&t),writable:!(4&t),"
            "value:r}}},5449:(t,r,e)=>{\"use strict\";var o=e(6935),n=e(5988),a=e(1887);t.exports=function(t,r,e){var "
            "s=o(r);s in t?n.f(t,s,a(0,e)):t[s]=e}},5746:(t,r,e)=>{var o=e(5981);t.exports=!o((function(){return "
            "7!=Object.defineProperty({},1,{get:function(){return 7}})[1]}))},1333:(t,r,e)=>{var "
            "o=e(1899),n=e(941),a=o.document,s=n(a)&&n(a.createElement);t.exports=function(t){return "
            "s?a.createElement(t):{}}},6049:(t,r,e)=>{var "
            "o=e(2532),n=e(1899);t.exports=\"process\"==o(n.process)},2861:(t,r,e)=>{var "
            "o=e(626);t.exports=o(\"navigator\",\"userAgent\")||\"\"},3385:(t,r,e)=>{var "
            "o,n,a=e(1899),s=e(2861),i=a.process,l=i&&i.versions,u=l&&l.v8;u?n=(o=u.split(\".\"))[0]+o[1]:s&&(!(o=s."
            "match(/Edge\\/(\\d+)/))||o[1]>=74)&&(o=s.match(/Chrome\\/(\\d+)/"
            "))&&(n=o[1]),t.exports=n&&+n},5703:(t,r,e)=>{var o=e(4058);t.exports=function(t){return "
            "o[t+\"Prototype\"]}},6887:(t,r,e)=>{\"use strict\";var "
            "o=e(1899),n=e(9677).f,a=e(7252),s=e(4058),i=e(6843),l=e(2029),u=e(7457),c=function(t){var "
            "r=function(r,e,o){if(this instanceof t){switch(arguments.length){case 0:return new t;case 1:return new "
            "t(r);case 2:return new t(r,e)}return new t(r,e,o)}return t.apply(this,arguments)};return "
            "r.prototype=t.prototype,r};t.exports=function(t,r){var "
            "e,f,p,h,v,g,b,d,y=t.target,T=t.global,x=t.stat,m=t.proto,w=T?o:x?o[y]:(o[y]||{}).prototype,E=T?s:s[y]||(s["
            "y]={}),j=E.prototype;for(p in "
            "r)e=!a(T?p:y+(x?\".\":\"#\")+p,t.forced)&&w&&u(w,p),v=E[p],e&&(g=t.noTargetGet?(d=n(w,p))&&d.value:w[p]),"
            "h=e&&g?g:r[p],e&&typeof v==typeof h||(b=t.bind&&e?i(h,o):t.wrap&&e?c(h):m&&\"function\"==typeof "
            "h?i(Function.call,h):h,(t.sham||h&&h.sham||v&&v.sham)&&l(b,\"sham\",!0),E[p]=b,m&&(u(s,f=y+\"Prototype\")|"
            "|l(s,f,{}),s[f][p]=h,t.real&&j&&!j[p]&&l(j,p,h)))}},5981:t=>{t.exports=function(t){try{return!!t()}catch("
            "t){return!0}}},6843:(t,r,e)=>{var o=e(3916);t.exports=function(t,r,e){if(o(t),void 0===r)return "
            "t;switch(e){case 0:return function(){return t.call(r)};case 1:return function(e){return t.call(r,e)};case "
            "2:return function(e,o){return t.call(r,e,o)};case 3:return function(e,o,n){return t.call(r,e,o,n)}}return "
            "function(){return t.apply(r,arguments)}}},626:(t,r,e)=>{var "
            "o=e(4058),n=e(1899),a=function(t){return\"function\"==typeof t?t:void 0};t.exports=function(t,r){return "
            "arguments.length<2?a(o[t])||a(n[t]):o[t]&&o[t][r]||n[t]&&n[t][r]}},1899:(t,r,e)=>{var "
            "o=function(t){return t&&t.Math==Math&&t};t.exports=o(\"object\"==typeof "
            "globalThis&&globalThis)||o(\"object\"==typeof window&&window)||o(\"object\"==typeof "
            "self&&self)||o(\"object\"==typeof e.g&&e.g)||function(){return this}()||Function(\"return "
            "this\")()},7457:(t,r,e)=>{var o=e(9678),n={}.hasOwnProperty;t.exports=function(t,r){return "
            "n.call(o(t),r)}},2840:(t,r,e)=>{var o=e(5746),n=e(5981),a=e(1333);t.exports=!o&&!n((function(){return "
            "7!=Object.defineProperty(a(\"div\"),\"a\",{get:function(){return 7}}).a}))},7026:(t,r,e)=>{var "
            "o=e(5981),n=e(2532),a=\"\".split;t.exports=o((function(){return!Object(\"z\").propertyIsEnumerable(0)}))?"
            "function(t){return\"String\"==n(t)?a.call(t,\"\"):Object(t)}:Object},1052:(t,r,e)=>{var "
            "o=e(2532);t.exports=Array.isArray||function(t){return\"Array\"==o(t)}},7252:(t,r,e)=>{var "
            "o=e(5981),n=/#|\\.prototype\\./,a=function(t,r){var e=i[s(t)];return e==u||e!=l&&(\"function\"==typeof "
            "r?o(r):!!r)},s=a.normalize=function(t){return "
            "String(t).replace(n,\".\").toLowerCase()},i=a.data={},l=a.NATIVE=\"N\",u=a.POLYFILL=\"P\";t.exports=a},"
            "941:t=>{t.exports=function(t){return\"object\"==typeof t?null!==t:\"function\"==typeof "
            "t}},2529:t=>{t.exports=!0},2497:(t,r,e)=>{var "
            "o=e(6049),n=e(3385),a=e(5981);t.exports=!!Object.getOwnPropertySymbols&&!a((function(){return!Symbol.sham&"
            "&(o?38===n:n>37&&n<41)}))},5988:(t,r,e)=>{var "
            "o=e(5746),n=e(2840),a=e(6059),s=e(6935),i=Object.defineProperty;r.f=o?i:function(t,r,e){if(a(t),r=s(r,!0),"
            "a(e),n)try{return i(t,r,e)}catch(t){}if(\"get\"in e||\"set\"in e)throw TypeError(\"Accessors not "
            "supported\");return\"value\"in e&&(t[r]=e.value),t}},9677:(t,r,e)=>{var "
            "o=e(5746),n=e(6760),a=e(1887),s=e(4529),i=e(6935),l=e(7457),u=e(2840),c=Object.getOwnPropertyDescriptor;r."
            "f=o?c:function(t,r){if(t=s(t),r=i(r,!0),u)try{return c(t,r)}catch(t){}if(l(t,r))return "
            "a(!n.f.call(t,r),t[r])}},6760:(t,r)=>{\"use strict\";var "
            "e={}.propertyIsEnumerable,o=Object.getOwnPropertyDescriptor,n=o&&!e.call({1:2},1);r.f=n?function(t){var "
            "r=o(this,t);return!!r&&r.enumerable}:e},4058:t=>{t.exports={}},8219:t=>{t.exports=function(t){if(null==t)"
            "throw TypeError(\"Can't call method on \"+t);return t}},4911:(t,r,e)=>{var "
            "o=e(1899),n=e(2029);t.exports=function(t,r){try{n(o,t,r)}catch(e){o[t]=r}return r}},3030:(t,r,e)=>{var "
            "o=e(1899),n=e(4911),a=\"__core-js_shared__\",s=o[a]||n(a,{});t.exports=s},8726:(t,r,e)=>{var "
            "o=e(2529),n=e(3030);(t.exports=function(t,r){return n[t]||(n[t]=void "
            "0!==r?r:{})})(\"versions\",[]).push({version:\"3.11.1\",mode:o?\"pure\":\"global\",copyright:\"© 2021 "
            "Denis Pushkarev (zloirock.ru)\"})},9413:(t,r,e)=>{var "
            "o=e(8459),n=Math.max,a=Math.min;t.exports=function(t,r){var e=o(t);return "
            "e<0?n(e+r,0):a(e,r)}},4529:(t,r,e)=>{var o=e(7026),n=e(8219);t.exports=function(t){return "
            "o(n(t))}},8459:t=>{var r=Math.ceil,e=Math.floor;t.exports=function(t){return "
            "isNaN(t=+t)?0:(t>0?e:r)(t)}},3057:(t,r,e)=>{var o=e(8459),n=Math.min;t.exports=function(t){return "
            "t>0?n(o(t),9007199254740991):0}},9678:(t,r,e)=>{var o=e(8219);t.exports=function(t){return "
            "Object(o(t))}},6935:(t,r,e)=>{var o=e(941);t.exports=function(t,r){if(!o(t))return t;var "
            "e,n;if(r&&\"function\"==typeof(e=t.toString)&&!o(n=e.call(t)))return "
            "n;if(\"function\"==typeof(e=t.valueOf)&&!o(n=e.call(t)))return "
            "n;if(!r&&\"function\"==typeof(e=t.toString)&&!o(n=e.call(t)))return n;throw TypeError(\"Can't convert "
            "object to primitive value\")}},9418:t=>{var "
            "r=0,e=Math.random();t.exports=function(t){return\"Symbol(\"+String(void "
            "0===t?\"\":t)+\")_\"+(++r+e).toString(36)}},2302:(t,r,e)=>{var "
            "o=e(2497);t.exports=o&&!Symbol.sham&&\"symbol\"==typeof Symbol.iterator},9813:(t,r,e)=>{var "
            "o=e(1899),n=e(8726),a=e(7457),s=e(9418),i=e(2497),l=e(2302),u=n(\"wks\"),c=o.Symbol,f=l?c:c&&c."
            "withoutSetter||s;t.exports=function(t){return a(u,t)&&(i||\"string\"==typeof "
            "u[t])||(i&&a(c,t)?u[t]=c[t]:u[t]=f(\"Symbol.\"+t)),u[t]}},186:(t,r,e)=>{\"use strict\";var "
            "o=e(6887),n=e(941),a=e(1052),s=e(9413),i=e(3057),l=e(4529),u=e(5449),c=e(9813),f=e(568)(\"slice\"),p=c("
            "\"species\"),h=[].slice,v=Math.max;o({target:\"Array\",proto:!0,forced:!f},{slice:function(t,r){var "
            "e,o,c,f=l(this),g=i(f.length),b=s(t,g),d=s(void "
            "0===r?g:r,g);if(a(f)&&(\"function\"!=typeof(e=f.constructor)||e!==Array&&!a(e.prototype)?n(e)&&null===(e="
            "e[p])&&(e=void 0):e=void 0,e===Array||void 0===e))return h.call(f,b,d);for(o=new(void "
            "0===e?Array:e)(v(d-b,0)),c=0;b<d;b++,c++)b in f&&u(o,c,f[b]);return o.length=c,o}})},2073:(t,r,e)=>{var "
            "o=e(9601);t.exports=o}},r={};function e(o){var n=r[o];if(void 0!==n)return n.exports;var "
            "a=r[o]={exports:{}};return t[o](a,a.exports,e),a.exports}e.n=t=>{var "
            "r=t&&t.__esModule?()=>t.default:()=>t;return e.d(r,{a:r}),r},e.d=(t,r)=>{for(var o in "
            "r)e.o(r,o)&&!e.o(t,o)&&Object.defineProperty(t,o,{enumerable:!0,get:r[o]})},e.g=function(){if(\"object\"=="
            "typeof globalThis)return globalThis;try{return this||new Function(\"return "
            "this\")()}catch(t){if(\"object\"==typeof window)return "
            "window}}(),e.o=(t,r)=>Object.prototype.hasOwnProperty.call(t,r),(()=>{\"use strict\";var "
            "t=e(2073),r=e.n(t);globalThis.assert.strictEqual(globalThis.addon.TestUtf8(\"\"),\"\"),globalThis.assert."
            "strictEqual(globalThis.addon.Utf8Length(\"\"),0);var o=\"hello "
            "world\";globalThis.assert.strictEqual(globalThis.addon.TestUtf8(o),o),globalThis.assert.strictEqual("
            "globalThis.addon.TestUtf8Insufficient(o),r()(o).call(o,0,3)),globalThis.assert.strictEqual(globalThis."
            "addon.Utf8Length(o),11);var "
            "n=\"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789\";globalThis.assert.strictEqual("
            "globalThis.addon.TestUtf8(n),n),globalThis.assert.strictEqual(globalThis.addon.TestUtf8Insufficient(n),r()"
            "(n).call(n,0,3)),globalThis.assert.strictEqual(globalThis.addon.Utf8Length(n),62);var "
            "a=\"?!@#$%^&*()_+-=[]{}/"
            ".,<>'\\\"\\\\\";globalThis.assert.strictEqual(globalThis.addon.TestUtf8(a),a),globalThis.assert."
            "strictEqual(globalThis.addon.TestUtf8Insufficient(a),r()(a).call(a,0,3)),globalThis.assert.strictEqual("
            "globalThis.addon.Utf8Length(a),27);var "
            "s=\"¡¢£¤¥¦§¨©ª«¬\u00AD®¯°±²³´µ¶·¸¹º»¼½¾¿\";globalThis.assert.strictEqual(globalThis.addon.TestUtf8(s),s),"
            "globalThis.assert.strictEqual(globalThis.addon.TestUtf8Insufficient(s),r()(s).call(s,0,1)),globalThis."
            "assert.strictEqual(globalThis.addon.Utf8Length(s),62);var "
            "i=\"ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþ\";globalThis.assert.strictEqual("
            "globalThis.addon.TestUtf8(i),i),globalThis.assert.strictEqual(globalThis.addon.TestUtf8Insufficient(i),r()"
            "(i).call(i,0,1)),globalThis.assert.strictEqual(globalThis.addon.Utf8Length(i),126);var "
            "l=\" ℁ Ȃ‑\";globalThis.assert.strictEqual(globalThis.addon.TestUtf8(l),l),globalThis.assert.strictEqual("
            "globalThis.addon.TestUtf8Insufficient(l),r()(l).call(l,0,1)),globalThis.assert.strictEqual(globalThis."
            "addon.Utf8Length(l),14),globalThis.addon.TestMemoryCorruption(\" \".repeat(65536))})()})();",
            "https://www.napi.com/test_string.js", &result),
        NAPIOK);
}