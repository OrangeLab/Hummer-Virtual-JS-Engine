#include <js_native_api_test.h>
#include <common.h>

EXTERN_C_START

static NAPIValue checkError(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    bool r;
    NAPI_CALL(env, napi_is_error(env, args[0], &r));

    NAPIValue result;
    NAPI_CALL(env, napi_get_boolean(env, r, &result));

    return result;
}

static NAPIValue throwExistingError(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue message;
    NAPIValue error;
    NAPI_CALL(env, napi_create_string_utf8(
            env, "existing error", NAPI_AUTO_LENGTH, &message));
    NAPI_CALL(env, napi_create_error(env, nullptr, message, &error));
    NAPI_CALL(env, napi_throw(env, error));
    return nullptr;
}

static NAPIValue throwError(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPI_CALL(env, napi_throw_error(env, nullptr, "error"));
    return nullptr;
}

static NAPIValue throwRangeError(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPI_CALL(env, napi_throw_range_error(env, nullptr, "range error"));
    return nullptr;
}

static NAPIValue throwTypeError(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPI_CALL(env, napi_throw_type_error(env, nullptr, "type error"));
    return nullptr;
}

static NAPIValue throwErrorCode(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPI_CALL(env, napi_throw_error(env, "ERR_TEST_CODE", "Error [error]"));
    return nullptr;
}

static NAPIValue throwRangeErrorCode(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPI_CALL(env, napi_throw_range_error(env,
                                          "ERR_TEST_CODE",
                                          "RangeError [range error]"));
    return nullptr;
}

static NAPIValue throwTypeErrorCode(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPI_CALL(env, napi_throw_type_error(env,
                                         "ERR_TEST_CODE",
                                         "TypeError [type error]"));
    return nullptr;
}


static NAPIValue createError(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue result;
    NAPIValue message;
    NAPI_CALL(env, napi_create_string_utf8(
            env, "error", NAPI_AUTO_LENGTH, &message));
    NAPI_CALL(env, napi_create_error(env, nullptr, message, &result));
    return result;
}

static NAPIValue createRangeError(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue result;
    NAPIValue message;
    NAPI_CALL(env, napi_create_string_utf8(
            env, "range error", NAPI_AUTO_LENGTH, &message));
    NAPI_CALL(env, napi_create_range_error(env, nullptr, message, &result));
    return result;
}

static NAPIValue createTypeError(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue result;
    NAPIValue message;
    NAPI_CALL(env, napi_create_string_utf8(
            env, "type error", NAPI_AUTO_LENGTH, &message));
    NAPI_CALL(env, napi_create_type_error(env, nullptr, message, &result));
    return result;
}

static NAPIValue createErrorCode(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue result;
    NAPIValue message;
    NAPIValue code;
    NAPI_CALL(env, napi_create_string_utf8(
            env, "Error [error]", NAPI_AUTO_LENGTH, &message));
    NAPI_CALL(env, napi_create_string_utf8(
            env, "ERR_TEST_CODE", NAPI_AUTO_LENGTH, &code));
    NAPI_CALL(env, napi_create_error(env, code, message, &result));
    return result;
}

static NAPIValue createRangeErrorCode(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue result;
    NAPIValue message;
    NAPIValue code;
    NAPI_CALL(env, napi_create_string_utf8(env,
                                           "RangeError [range error]",
                                           NAPI_AUTO_LENGTH,
                                           &message));
    NAPI_CALL(env, napi_create_string_utf8(
            env, "ERR_TEST_CODE", NAPI_AUTO_LENGTH, &code));
    NAPI_CALL(env, napi_create_range_error(env, code, message, &result));
    return result;
}

static NAPIValue createTypeErrorCode(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIValue result;
    NAPIValue message;
    NAPIValue code;
    NAPI_CALL(env, napi_create_string_utf8(env,
                                           "TypeError [type error]",
                                           NAPI_AUTO_LENGTH,
                                           &message));
    NAPI_CALL(env, napi_create_string_utf8(
            env, "ERR_TEST_CODE", NAPI_AUTO_LENGTH, &code));
    NAPI_CALL(env, napi_create_type_error(env, code, message, &result));
    return result;
}

static NAPIValue throwArbitrary(NAPIEnv env, NAPICallbackInfo info) {
    NAPIValue arbitrary;
    size_t argc = 1;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &arbitrary, nullptr, nullptr));
    NAPI_CALL(env, napi_throw(env, arbitrary));
    return nullptr;
}

EXTERN_C_END

TEST_F(Test, TestError) {
    NAPIPropertyDescriptor descriptors[] = {
            DECLARE_NAPI_PROPERTY("checkError", checkError),
            DECLARE_NAPI_PROPERTY("throwExistingError", throwExistingError),
            DECLARE_NAPI_PROPERTY("throwError", throwError),
            DECLARE_NAPI_PROPERTY("throwRangeError", throwRangeError),
            DECLARE_NAPI_PROPERTY("throwTypeError", throwTypeError),
            DECLARE_NAPI_PROPERTY("throwErrorCode", throwErrorCode),
            DECLARE_NAPI_PROPERTY("throwRangeErrorCode", throwRangeErrorCode),
            DECLARE_NAPI_PROPERTY("throwTypeErrorCode", throwTypeErrorCode),
            DECLARE_NAPI_PROPERTY("throwArbitrary", throwArbitrary),
            DECLARE_NAPI_PROPERTY("createError", createError),
            DECLARE_NAPI_PROPERTY("createRangeError", createRangeError),
            DECLARE_NAPI_PROPERTY("createTypeError", createTypeError),
            DECLARE_NAPI_PROPERTY("createErrorCode", createErrorCode),
            DECLARE_NAPI_PROPERTY("createRangeErrorCode", createRangeErrorCode),
            DECLARE_NAPI_PROPERTY("createTypeErrorCode", createTypeErrorCode),
    };

    ASSERT_EQ(napi_define_properties(
            globalEnv, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors), NAPIOK);

    NAPIValue result;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(globalEnv,
                                         "(()=>{var r={7473:(r,t,e)=>{e(8922),e(5967),e(5824),e(8555),e(2615),e(1732),e(5903),e(1825),e(8394),e(5915),e(1766),e(2737),e(9911),e(4315),e(3131),e(4714),e(659),e(9120),e(5327),e(1502);var o=e(4058);r.exports=o.Symbol},3916:r=>{r.exports=function(r){if(\"function\"!=typeof r)throw TypeError(String(r)+\" is not a function\");return r}},6059:(r,t,e)=>{var o=e(941);r.exports=function(r){if(!o(r))throw TypeError(String(r)+\" is not an object\");return r}},1692:(r,t,e)=>{var o=e(4529),n=e(3057),a=e(9413),i=function(r){return function(t,e,i){var c,s=o(t),u=n(s.length),l=a(i,u);if(r&&e!=e){for(;u>l;)if((c=s[l++])!=c)return!0}else for(;u>l;l++)if((r||l in s)&&s[l]===e)return r||l||0;return!r&&-1}};r.exports={includes:i(!0),indexOf:i(!1)}},3610:(r,t,e)=>{var o=e(6843),n=e(7026),a=e(9678),i=e(3057),c=e(4692),s=[].push,u=function(r){var t=1==r,e=2==r,u=3==r,l=4==r,f=6==r,p=7==r,h=5==r||f;return function(y,b,g,d){for(var v,E,m=a(y),w=n(m),T=o(b,g,3),x=i(w.length),O=0,S=d||c,j=t?S(y,x):e||p?S(y,0):void 0;x>O;O++)if((h||O in w)&&(E=T(v=w[O],O,m),r))if(t)j[O]=E;else if(E)switch(r){case 3:return!0;case 5:return v;case 6:return O;case 2:s.call(j,v)}else switch(r){case 4:return!1;case 7:s.call(j,v)}return f?-1:u||l?l:j}};r.exports={forEach:u(0),map:u(1),filter:u(2),some:u(3),every:u(4),find:u(5),findIndex:u(6),filterOut:u(7)}},568:(r,t,e)=>{var o=e(5981),n=e(9813),a=e(3385),i=n(\"species\");r.exports=function(r){return a>=51||!o((function(){var t=[];return(t.constructor={})[i]=function(){return{foo:1}},1!==t[r](Boolean).foo}))}},4692:(r,t,e)=>{var o=e(941),n=e(1052),a=e(9813)(\"species\");r.exports=function(r,t){var e;return n(r)&&(\"function\"!=typeof(e=r.constructor)||e!==Array&&!n(e.prototype)?o(e)&&null===(e=e[a])&&(e=void 0):e=void 0),new(void 0===e?Array:e)(0===t?0:t)}},2532:r=>{var t={}.toString;r.exports=function(r){return t.call(r).slice(8,-1)}},9697:(r,t,e)=>{var o=e(2885),n=e(2532),a=e(9813)(\"toStringTag\"),i=\"Arguments\"==n(function(){return arguments}());r.exports=o?n:function(r){var t,e,o;return void 0===r?\"Undefined\":null===r?\"Null\":\"string\"==typeof(e=function(r,t){try{return r[t]}catch(r){}}(t=Object(r),a))?e:i?n(t):\"Object\"==(o=n(t))&&\"function\"==typeof t.callee?\"Arguments\":o}},2029:(r,t,e)=>{var o=e(5746),n=e(5988),a=e(1887);r.exports=o?function(r,t,e){return n.f(r,t,a(1,e))}:function(r,t,e){return r[t]=e,r}},1887:r=>{r.exports=function(r,t){return{enumerable:!(1&r),configurable:!(2&r),writable:!(4&r),value:t}}},5449:(r,t,e)=>{\"use strict\";var o=e(6935),n=e(5988),a=e(1887);r.exports=function(r,t,e){var i=o(t);i in r?n.f(r,i,a(0,e)):r[i]=e}},6349:(r,t,e)=>{var o=e(4058),n=e(7457),a=e(1477),i=e(5988).f;r.exports=function(r){var t=o.Symbol||(o.Symbol={});n(t,r)||i(t,r,{value:a.f(r)})}},5746:(r,t,e)=>{var o=e(5981);r.exports=!o((function(){return 7!=Object.defineProperty({},1,{get:function(){return 7}})[1]}))},1333:(r,t,e)=>{var o=e(1899),n=e(941),a=o.document,i=n(a)&&n(a.createElement);r.exports=function(r){return i?a.createElement(r):{}}},6049:(r,t,e)=>{var o=e(2532),n=e(1899);r.exports=\"process\"==o(n.process)},2861:(r,t,e)=>{var o=e(626);r.exports=o(\"navigator\",\"userAgent\")||\"\"},3385:(r,t,e)=>{var o,n,a=e(1899),i=e(2861),c=a.process,s=c&&c.versions,u=s&&s.v8;u?n=(o=u.split(\".\"))[0]+o[1]:i&&(!(o=i.match(/Edge\\/(\\d+)/))||o[1]>=74)&&(o=i.match(/Chrome\\/(\\d+)/))&&(n=o[1]),r.exports=n&&+n},6759:r=>{r.exports=[\"constructor\",\"hasOwnProperty\",\"isPrototypeOf\",\"propertyIsEnumerable\",\"toLocaleString\",\"toString\",\"valueOf\"]},6887:(r,t,e)=>{\"use strict\";var o=e(1899),n=e(9677).f,a=e(7252),i=e(4058),c=e(6843),s=e(2029),u=e(7457),l=function(r){var t=function(t,e,o){if(this instanceof r){switch(arguments.length){case 0:return new r;case 1:return new r(t);case 2:return new r(t,e)}return new r(t,e,o)}return r.apply(this,arguments)};return t.prototype=r.prototype,t};r.exports=function(r,t){var e,f,p,h,y,b,g,d,v=r.target,E=r.global,m=r.stat,w=r.proto,T=E?o:m?o[v]:(o[v]||{}).prototype,x=E?i:i[v]||(i[v]={}),O=x.prototype;for(p in t)e=!a(E?p:v+(m?\".\":\"#\")+p,r.forced)&&T&&u(T,p),y=x[p],e&&(b=r.noTargetGet?(d=n(T,p))&&d.value:T[p]),h=e&&b?b:t[p],e&&typeof y==typeof h||(g=r.bind&&e?c(h,o):r.wrap&&e?l(h):w&&\"function\"==typeof h?c(Function.call,h):h,(r.sham||h&&h.sham||y&&y.sham)&&s(g,\"sham\",!0),x[p]=g,w&&(u(i,f=v+\"Prototype\")||s(i,f,{}),i[f][p]=h,r.real&&O&&!O[p]&&s(O,p,h)))}},5981:r=>{r.exports=function(r){try{return!!r()}catch(r){return!0}}},6843:(r,t,e)=>{var o=e(3916);r.exports=function(r,t,e){if(o(r),void 0===t)return r;switch(e){case 0:return function(){return r.call(t)};case 1:return function(e){return r.call(t,e)};case 2:return function(e,o){return r.call(t,e,o)};case 3:return function(e,o,n){return r.call(t,e,o,n)}}return function(){return r.apply(t,arguments)}}},626:(r,t,e)=>{var o=e(4058),n=e(1899),a=function(r){return\"function\"==typeof r?r:void 0};r.exports=function(r,t){return arguments.length<2?a(o[r])||a(n[r]):o[r]&&o[r][t]||n[r]&&n[r][t]}},1899:(r,t,e)=>{var o=function(r){return r&&r.Math==Math&&r};r.exports=o(\"object\"==typeof globalThis&&globalThis)||o(\"object\"==typeof window&&window)||o(\"object\"==typeof self&&self)||o(\"object\"==typeof e.g&&e.g)||function(){return this}()||Function(\"return this\")()},7457:(r,t,e)=>{var o=e(9678),n={}.hasOwnProperty;r.exports=function(r,t){return n.call(o(r),t)}},7748:r=>{r.exports={}},5463:(r,t,e)=>{var o=e(626);r.exports=o(\"document\",\"documentElement\")},2840:(r,t,e)=>{var o=e(5746),n=e(5981),a=e(1333);r.exports=!o&&!n((function(){return 7!=Object.defineProperty(a(\"div\"),\"a\",{get:function(){return 7}}).a}))},7026:(r,t,e)=>{var o=e(5981),n=e(2532),a=\"\".split;r.exports=o((function(){return!Object(\"z\").propertyIsEnumerable(0)}))?function(r){return\"String\"==n(r)?a.call(r,\"\"):Object(r)}:Object},1302:(r,t,e)=>{var o=e(3030),n=Function.toString;\"function\"!=typeof o.inspectSource&&(o.inspectSource=function(r){return n.call(r)}),r.exports=o.inspectSource},5402:(r,t,e)=>{var o,n,a,i=e(8019),c=e(1899),s=e(941),u=e(2029),l=e(7457),f=e(3030),p=e(4262),h=e(7748),y=\"Object already initialized\",b=c.WeakMap;if(i){var g=f.state||(f.state=new b),d=g.get,v=g.has,E=g.set;o=function(r,t){if(v.call(g,r))throw new TypeError(y);return t.facade=r,E.call(g,r,t),t},n=function(r){return d.call(g,r)||{}},a=function(r){return v.call(g,r)}}else{var m=p(\"state\");h[m]=!0,o=function(r,t){if(l(r,m))throw new TypeError(y);return t.facade=r,u(r,m,t),t},n=function(r){return l(r,m)?r[m]:{}},a=function(r){return l(r,m)}}r.exports={set:o,get:n,has:a,enforce:function(r){return a(r)?n(r):o(r,{})},getterFor:function(r){return function(t){var e;if(!s(t)||(e=n(t)).type!==r)throw TypeError(\"Incompatible receiver, \"+r+\" required\");return e}}}},1052:(r,t,e)=>{var o=e(2532);r.exports=Array.isArray||function(r){return\"Array\"==o(r)}},7252:(r,t,e)=>{var o=e(5981),n=/#|\\.prototype\\./,a=function(r,t){var e=c[i(r)];return e==u||e!=s&&(\"function\"==typeof t?o(t):!!t)},i=a.normalize=function(r){return String(r).replace(n,\".\").toLowerCase()},c=a.data={},s=a.NATIVE=\"N\",u=a.POLYFILL=\"P\";r.exports=a},941:r=>{r.exports=function(r){return\"object\"==typeof r?null!==r:\"function\"==typeof r}},2529:r=>{r.exports=!0},2497:(r,t,e)=>{var o=e(6049),n=e(3385),a=e(5981);r.exports=!!Object.getOwnPropertySymbols&&!a((function(){return!Symbol.sham&&(o?38===n:n>37&&n<41)}))},8019:(r,t,e)=>{var o=e(1899),n=e(1302),a=o.WeakMap;r.exports=\"function\"==typeof a&&/native code/.test(n(a))},9290:(r,t,e)=>{var o,n=e(6059),a=e(9938),i=e(6759),c=e(7748),s=e(5463),u=e(1333),l=e(4262)(\"IE_PROTO\"),f=function(){},p=function(r){return\"<script>\"+r+\"<\\/script>\"},h=function(){try{o=document.domain&&new ActiveXObject(\"htmlfile\")}catch(r){}var r,t;h=o?function(r){r.write(p(\"\")),r.close();var t=r.parentWindow.Object;return r=null,t}(o):((t=u(\"iframe\")).style.display=\"none\",s.appendChild(t),t.src=String(\"javascript:\"),(r=t.contentWindow.document).open(),r.write(p(\"document.F=Object\")),r.close(),r.F);for(var e=i.length;e--;)delete h.prototype[i[e]];return h()};c[l]=!0,r.exports=Object.create||function(r,t){var e;return null!==r?(f.prototype=n(r),e=new f,f.prototype=null,e[l]=r):e=h(),void 0===t?e:a(e,t)}},9938:(r,t,e)=>{var o=e(5746),n=e(5988),a=e(6059),i=e(4771);r.exports=o?Object.defineProperties:function(r,t){a(r);for(var e,o=i(t),c=o.length,s=0;c>s;)n.f(r,e=o[s++],t[e]);return r}},5988:(r,t,e)=>{var o=e(5746),n=e(2840),a=e(6059),i=e(6935),c=Object.defineProperty;t.f=o?c:function(r,t,e){if(a(r),t=i(t,!0),a(e),n)try{return c(r,t,e)}catch(r){}if(\"get\"in e||\"set\"in e)throw TypeError(\"Accessors not supported\");return\"value\"in e&&(r[t]=e.value),r}},9677:(r,t,e)=>{var o=e(5746),n=e(6760),a=e(1887),i=e(4529),c=e(6935),s=e(7457),u=e(2840),l=Object.getOwnPropertyDescriptor;t.f=o?l:function(r,t){if(r=i(r),t=c(t,!0),u)try{return l(r,t)}catch(r){}if(s(r,t))return a(!n.f.call(r,t),r[t])}},684:(r,t,e)=>{var o=e(4529),n=e(946).f,a={}.toString,i=\"object\"==typeof window&&window&&Object.getOwnPropertyNames?Object.getOwnPropertyNames(window):[];r.exports.f=function(r){return i&&\"[object Window]\"==a.call(r)?function(r){try{return n(r)}catch(r){return i.slice()}}(r):n(o(r))}},946:(r,t,e)=>{var o=e(5629),n=e(6759).concat(\"length\",\"prototype\");t.f=Object.getOwnPropertyNames||function(r){return o(r,n)}},7857:(r,t)=>{t.f=Object.getOwnPropertySymbols},5629:(r,t,e)=>{var o=e(7457),n=e(4529),a=e(1692).indexOf,i=e(7748);r.exports=function(r,t){var e,c=n(r),s=0,u=[];for(e in c)!o(i,e)&&o(c,e)&&u.push(e);for(;t.length>s;)o(c,e=t[s++])&&(~a(u,e)||u.push(e));return u}},4771:(r,t,e)=>{var o=e(5629),n=e(6759);r.exports=Object.keys||function(r){return o(r,n)}},6760:(r,t)=>{\"use strict\";var e={}.propertyIsEnumerable,o=Object.getOwnPropertyDescriptor,n=o&&!e.call({1:2},1);t.f=n?function(r){var t=o(this,r);return!!t&&t.enumerable}:e},5623:(r,t,e)=>{\"use strict\";var o=e(2885),n=e(9697);r.exports=o?{}.toString:function(){return\"[object \"+n(this)+\"]\"}},4058:r=>{r.exports={}},9754:(r,t,e)=>{var o=e(2029);r.exports=function(r,t,e,n){n&&n.enumerable?r[t]=e:o(r,t,e)}},8219:r=>{r.exports=function(r){if(null==r)throw TypeError(\"Can't call method on \"+r);return r}},4911:(r,t,e)=>{var o=e(1899),n=e(2029);r.exports=function(r,t){try{n(o,r,t)}catch(e){o[r]=t}return t}},904:(r,t,e)=>{var o=e(2885),n=e(5988).f,a=e(2029),i=e(7457),c=e(5623),s=e(9813)(\"toStringTag\");r.exports=function(r,t,e,u){if(r){var l=e?r:r.prototype;i(l,s)||n(l,s,{configurable:!0,value:t}),u&&!o&&a(l,\"toString\",c)}}},4262:(r,t,e)=>{var o=e(8726),n=e(9418),a=o(\"keys\");r.exports=function(r){return a[r]||(a[r]=n(r))}},3030:(r,t,e)=>{var o=e(1899),n=e(4911),a=\"__core-js_shared__\",i=o[a]||n(a,{});r.exports=i},8726:(r,t,e)=>{var o=e(2529),n=e(3030);(r.exports=function(r,t){return n[r]||(n[r]=void 0!==t?t:{})})(\"versions\",[]).push({version:\"3.11.1\",mode:o?\"pure\":\"global\",copyright:\"© 2021 Denis Pushkarev (zloirock.ru)\"})},9413:(r,t,e)=>{var o=e(8459),n=Math.max,a=Math.min;r.exports=function(r,t){var e=o(r);return e<0?n(e+t,0):a(e,t)}},4529:(r,t,e)=>{var o=e(7026),n=e(8219);r.exports=function(r){return o(n(r))}},8459:r=>{var t=Math.ceil,e=Math.floor;r.exports=function(r){return isNaN(r=+r)?0:(r>0?e:t)(r)}},3057:(r,t,e)=>{var o=e(8459),n=Math.min;r.exports=function(r){return r>0?n(o(r),9007199254740991):0}},9678:(r,t,e)=>{var o=e(8219);r.exports=function(r){return Object(o(r))}},6935:(r,t,e)=>{var o=e(941);r.exports=function(r,t){if(!o(r))return r;var e,n;if(t&&\"function\"==typeof(e=r.toString)&&!o(n=e.call(r)))return n;if(\"function\"==typeof(e=r.valueOf)&&!o(n=e.call(r)))return n;if(!t&&\"function\"==typeof(e=r.toString)&&!o(n=e.call(r)))return n;throw TypeError(\"Can't convert object to primitive value\")}},2885:(r,t,e)=>{var o={};o[e(9813)(\"toStringTag\")]=\"z\",r.exports=\"[object z]\"===String(o)},9418:r=>{var t=0,e=Math.random();r.exports=function(r){return\"Symbol(\"+String(void 0===r?\"\":r)+\")_\"+(++t+e).toString(36)}},2302:(r,t,e)=>{var o=e(2497);r.exports=o&&!Symbol.sham&&\"symbol\"==typeof Symbol.iterator},1477:(r,t,e)=>{var o=e(9813);t.f=o},9813:(r,t,e)=>{var o=e(1899),n=e(8726),a=e(7457),i=e(9418),c=e(2497),s=e(2302),u=n(\"wks\"),l=o.Symbol,f=s?l:l&&l.withoutSetter||i;r.exports=function(r){return a(u,r)&&(c||\"string\"==typeof u[r])||(c&&a(l,r)?u[r]=l[r]:u[r]=f(\"Symbol.\"+r)),u[r]}},8922:(r,t,e)=>{\"use strict\";var o=e(6887),n=e(5981),a=e(1052),i=e(941),c=e(9678),s=e(3057),u=e(5449),l=e(4692),f=e(568),p=e(9813),h=e(3385),y=p(\"isConcatSpreadable\"),b=9007199254740991,g=\"Maximum allowed index exceeded\",d=h>=51||!n((function(){var r=[];return r[y]=!1,r.concat()[0]!==r})),v=f(\"concat\"),E=function(r){if(!i(r))return!1;var t=r[y];return void 0!==t?!!t:a(r)};o({target:\"Array\",proto:!0,forced:!d||!v},{concat:function(r){var t,e,o,n,a,i=c(this),f=l(i,0),p=0;for(t=-1,o=arguments.length;t<o;t++)if(E(a=-1===t?i:arguments[t])){if(p+(n=s(a.length))>b)throw TypeError(g);for(e=0;e<n;e++,p++)e in a&&u(f,p,a[e])}else{if(p>=b)throw TypeError(g);u(f,p++,a)}return f.length=p,f}})},9120:(r,t,e)=>{var o=e(1899);e(904)(o.JSON,\"JSON\",!0)},5327:()=>{},5967:()=>{},1502:()=>{},8555:(r,t,e)=>{e(6349)(\"asyncIterator\")},2615:()=>{},1732:(r,t,e)=>{e(6349)(\"hasInstance\")},5903:(r,t,e)=>{e(6349)(\"isConcatSpreadable\")},1825:(r,t,e)=>{e(6349)(\"iterator\")},5824:(r,t,e)=>{\"use strict\";var o=e(6887),n=e(1899),a=e(626),i=e(2529),c=e(5746),s=e(2497),u=e(2302),l=e(5981),f=e(7457),p=e(1052),h=e(941),y=e(6059),b=e(9678),g=e(4529),d=e(6935),v=e(1887),E=e(9290),m=e(4771),w=e(946),T=e(684),x=e(7857),O=e(9677),S=e(5988),j=e(6760),P=e(2029),R=e(9754),k=e(8726),q=e(4262),_=e(7748),C=e(9418),M=e(9813),N=e(1477),A=e(6349),I=e(904),F=e(5402),z=e(3610).forEach,D=q(\"hidden\"),L=\"Symbol\",W=M(\"toPrimitive\"),J=F.set,B=F.getterFor(L),U=Object.prototype,G=n.Symbol,Q=a(\"JSON\",\"stringify\"),V=O.f,X=S.f,Y=T.f,H=j.f,K=k(\"symbols\"),Z=k(\"op-symbols\"),$=k(\"string-to-symbol-registry\"),rr=k(\"symbol-to-string-registry\"),tr=k(\"wks\"),er=n.QObject,or=!er||!er.prototype||!er.prototype.findChild,nr=c&&l((function(){return 7!=E(X({},\"a\",{get:function(){return X(this,\"a\",{value:7}).a}})).a}))?function(r,t,e){var o=V(U,t);o&&delete U[t],X(r,t,e),o&&r!==U&&X(U,t,o)}:X,ar=function(r,t){var e=K[r]=E(G.prototype);return J(e,{type:L,tag:r,description:t}),c||(e.description=t),e},ir=u?function(r){return\"symbol\"==typeof r}:function(r){return Object(r)instanceof G},cr=function(r,t,e){r===U&&cr(Z,t,e),y(r);var o=d(t,!0);return y(e),f(K,o)?(e.enumerable?(f(r,D)&&r[D][o]&&(r[D][o]=!1),e=E(e,{enumerable:v(0,!1)})):(f(r,D)||X(r,D,v(1,{})),r[D][o]=!0),nr(r,o,e)):X(r,o,e)},sr=function(r,t){y(r);var e=g(t),o=m(e).concat(pr(e));return z(o,(function(t){c&&!ur.call(e,t)||cr(r,t,e[t])})),r},ur=function(r){var t=d(r,!0),e=H.call(this,t);return!(this===U&&f(K,t)&&!f(Z,t))&&(!(e||!f(this,t)||!f(K,t)||f(this,D)&&this[D][t])||e)},lr=function(r,t){var e=g(r),o=d(t,!0);if(e!==U||!f(K,o)||f(Z,o)){var n=V(e,o);return!n||!f(K,o)||f(e,D)&&e[D][o]||(n.enumerable=!0),n}},fr=function(r){var t=Y(g(r)),e=[];return z(t,(function(r){f(K,r)||f(_,r)||e.push(r)})),e},pr=function(r){var t=r===U,e=Y(t?Z:g(r)),o=[];return z(e,(function(r){!f(K,r)||t&&!f(U,r)||o.push(K[r])})),o};s||(R((G=function(){if(this instanceof G)throw TypeError(\"Symbol is not a constructor\");var r=arguments.length&&void 0!==arguments[0]?String(arguments[0]):void 0,t=C(r),e=function(r){this===U&&e.call(Z,r),f(this,D)&&f(this[D],t)&&(this[D][t]=!1),nr(this,t,v(1,r))};return c&&or&&nr(U,t,{configurable:!0,set:e}),ar(t,r)}).prototype,\"toString\",(function(){return B(this).tag})),R(G,\"withoutSetter\",(function(r){return ar(C(r),r)})),j.f=ur,S.f=cr,O.f=lr,w.f=T.f=fr,x.f=pr,N.f=function(r){return ar(M(r),r)},c&&(X(G.prototype,\"description\",{configurable:!0,get:function(){return B(this).description}}),i||R(U,\"propertyIsEnumerable\",ur,{unsafe:!0}))),o({global:!0,wrap:!0,forced:!s,sham:!s},{Symbol:G}),z(m(tr),(function(r){A(r)})),o({target:L,stat:!0,forced:!s},{for:function(r){var t=String(r);if(f($,t))return $[t];var e=G(t);return $[t]=e,rr[e]=t,e},keyFor:function(r){if(!ir(r))throw TypeError(r+\" is not a symbol\");if(f(rr,r))return rr[r]},useSetter:function(){or=!0},useSimple:function(){or=!1}}),o({target:\"Object\",stat:!0,forced:!s,sham:!c},{create:function(r,t){return void 0===t?E(r):sr(E(r),t)},defineProperty:cr,defineProperties:sr,getOwnPropertyDescriptor:lr}),o({target:\"Object\",stat:!0,forced:!s},{getOwnPropertyNames:fr,getOwnPropertySymbols:pr}),o({target:\"Object\",stat:!0,forced:l((function(){x.f(1)}))},{getOwnPropertySymbols:function(r){return x.f(b(r))}}),Q&&o({target:\"JSON\",stat:!0,forced:!s||l((function(){var r=G();return\"[null]\"!=Q([r])||\"{}\"!=Q({a:r})||\"{}\"!=Q(Object(r))}))},{stringify:function(r,t,e){for(var o,n=[r],a=1;arguments.length>a;)n.push(arguments[a++]);if(o=t,(h(t)||void 0!==r)&&!ir(r))return p(t)||(t=function(r,t){if(\"function\"==typeof o&&(t=o.call(this,r,t)),!ir(t))return t}),n[1]=t,Q.apply(null,n)}}),G.prototype[W]||P(G.prototype,W,G.prototype.valueOf),I(G,L),_[D]=!0},5915:(r,t,e)=>{e(6349)(\"matchAll\")},8394:(r,t,e)=>{e(6349)(\"match\")},1766:(r,t,e)=>{e(6349)(\"replace\")},2737:(r,t,e)=>{e(6349)(\"search\")},9911:(r,t,e)=>{e(6349)(\"species\")},4315:(r,t,e)=>{e(6349)(\"split\")},3131:(r,t,e)=>{e(6349)(\"toPrimitive\")},4714:(r,t,e)=>{e(6349)(\"toStringTag\")},659:(r,t,e)=>{e(6349)(\"unscopables\")},2547:(r,t,e)=>{var o=e(7473);r.exports=o}},t={};function e(o){var n=t[o];if(void 0!==n)return n.exports;var a=t[o]={exports:{}};return r[o](a,a.exports,e),a.exports}e.n=r=>{var t=r&&r.__esModule?()=>r.default:()=>r;return e.d(t,{a:t}),t},e.d=(r,t)=>{for(var o in t)e.o(t,o)&&!e.o(r,o)&&Object.defineProperty(r,o,{enumerable:!0,get:t[o]})},e.g=function(){if(\"object\"==typeof globalThis)return globalThis;try{return this||new Function(\"return this\")()}catch(r){if(\"object\"==typeof window)return window}}(),e.o=(r,t)=>Object.prototype.hasOwnProperty.call(r,t),(()=>{\"use strict\";function r(r,t){if(!(r instanceof t))throw new TypeError(\"Cannot call a class as a function\")}function t(r,e){return(t=Object.setPrototypeOf||function(r,t){return r.__proto__=t,r})(r,e)}function o(r){return(o=Object.setPrototypeOf?Object.getPrototypeOf:function(r){return r.__proto__||Object.getPrototypeOf(r)})(r)}function n(){if(\"undefined\"==typeof Reflect||!Reflect.construct)return!1;if(Reflect.construct.sham)return!1;if(\"function\"==typeof Proxy)return!0;try{return Boolean.prototype.valueOf.call(Reflect.construct(Boolean,[],(function(){}))),!0}catch(r){return!1}}function a(r){return(a=\"function\"==typeof Symbol&&\"symbol\"==typeof Symbol.iterator?function(r){return typeof r}:function(r){return r&&\"function\"==typeof Symbol&&r.constructor===Symbol&&r!==Symbol.prototype?\"symbol\":typeof r})(r)}function i(r,t){return!t||\"object\"!==a(t)&&\"function\"!=typeof t?function(r){if(void 0===r)throw new ReferenceError(\"this hasn't been initialised - super() hasn't been called\");return r}(r):t}function c(r,e,o){return(c=n()?Reflect.construct:function(r,e,o){var n=[null];n.push.apply(n,e);var a=new(Function.bind.apply(r,n));return o&&t(a,o.prototype),a}).apply(null,arguments)}function s(r){var e=\"function\"==typeof Map?new Map:void 0;return(s=function(r){if(null===r||(n=r,-1===Function.toString.call(n).indexOf(\"[native code]\")))return r;var n;if(\"function\"!=typeof r)throw new TypeError(\"Super expression must either be null or a function\");if(void 0!==e){if(e.has(r))return e.get(r);e.set(r,a)}function a(){return c(r,arguments,o(this).constructor)}return a.prototype=Object.create(r.prototype,{constructor:{value:a,enumerable:!1,writable:!0,configurable:!0}}),t(a,r)})(r)}var u=e(2547),l=e.n(u),f=new Error(\"Some error\"),p=new TypeError(\"Some type error\"),h=new SyntaxError(\"Some syntax error\"),y=new RangeError(\"Some type error\"),b=new ReferenceError(\"Some reference error\"),g=new URIError(\"Some URI error\"),d=new EvalError(\"Some eval error\"),v=new(function(e){!function(r,e){if(\"function\"!=typeof e&&null!==e)throw new TypeError(\"Super expression must either be null or a function\");r.prototype=Object.create(e&&e.prototype,{constructor:{value:r,writable:!0,configurable:!0}}),e&&t(r,e)}(u,e);var a,c,s=(a=u,c=n(),function(){var r,t=o(a);if(c){var e=o(this).constructor;r=Reflect.construct(t,arguments,e)}else r=t.apply(this,arguments);return i(this,r)});function u(){return r(this,u),s.apply(this,arguments)}return u}(s(Error)))(\"Some MyError\");globalThis.assert.strictEqual(globalThis.addon.checkError(f),!0),globalThis.assert.strictEqual(globalThis.addon.checkError(p),!0),globalThis.assert.strictEqual(globalThis.addon.checkError(h),!0),globalThis.assert.strictEqual(globalThis.addon.checkError(y),!0),globalThis.assert.strictEqual(globalThis.addon.checkError(b),!0),globalThis.assert.strictEqual(globalThis.addon.checkError(g),!0),globalThis.assert.strictEqual(globalThis.addon.checkError(d),!0),globalThis.assert.strictEqual(globalThis.addon.checkError(v),!0),globalThis.assert.strictEqual(globalThis.addon.checkError({}),!1),globalThis.assert.strictEqual(globalThis.addon.checkError(\"non-object\"),!1),globalThis.assert.throws((function(){globalThis.addon.throwExistingError()})),globalThis.assert.throws((function(){globalThis.addon.throwError()})),globalThis.assert.throws((function(){globalThis.addon.throwRangeError()})),globalThis.assert.throws((function(){globalThis.addon.throwTypeError()})),[42,{},[],l()(\"xyzzy\"),!0,\"ball\",void 0,null,NaN].forEach((function(r){return globalThis.assert.throws((function(){return globalThis.addon.throwArbitrary(r)}))})),globalThis.assert.throws((function(){return globalThis.addon.throwErrorCode()})),globalThis.assert.throws((function(){return globalThis.addon.throwRangeErrorCode()})),globalThis.assert.throws((function(){return globalThis.addon.throwTypeErrorCode()}));var E=globalThis.addon.createError();globalThis.assert.ok(E instanceof Error,\"expected error to be an instance of Error\"),globalThis.assert.strictEqual(E.message,\"error\"),E=globalThis.addon.createRangeError(),globalThis.assert.ok(E instanceof RangeError,\"expected error to be an instance of RangeError\"),globalThis.assert.strictEqual(E.message,\"range error\"),E=globalThis.addon.createTypeError(),globalThis.assert.ok(E instanceof TypeError,\"expected error to be an instance of TypeError\"),globalThis.assert.strictEqual(E.message,\"type error\"),E=globalThis.addon.createErrorCode(),globalThis.assert.ok(E instanceof Error,\"expected error to be an instance of Error\"),globalThis.assert.strictEqual(E.code,\"ERR_TEST_CODE\"),globalThis.assert.strictEqual(E.message,\"Error [error]\"),globalThis.assert.strictEqual(E.name,\"Error\"),E=globalThis.addon.createRangeErrorCode(),globalThis.assert.ok(E instanceof RangeError,\"expected error to be an instance of RangeError\"),globalThis.assert.strictEqual(E.message,\"RangeError [range error]\"),globalThis.assert.strictEqual(E.code,\"ERR_TEST_CODE\"),globalThis.assert.strictEqual(E.name,\"RangeError\"),E=globalThis.addon.createTypeErrorCode(),globalThis.assert.ok(E instanceof TypeError,\"expected error to be an instance of TypeError\"),globalThis.assert.strictEqual(E.message,\"TypeError [type error]\"),globalThis.assert.strictEqual(E.code,\"ERR_TEST_CODE\"),globalThis.assert.strictEqual(E.name,\"TypeError\")})()})();",
                                         "https://www.didi.com/test_error.js",
                                         &result), NAPIOK);
}