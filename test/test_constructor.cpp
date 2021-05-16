#include <gtest/gtest.h>
#include <napi/js_native_api.h>
#include <common.h>
#include <js_native_api_test.h>

EXTERN_C_START

static double value_ = 1;
static double static_value_ = 10;

static NAPIValue TestDefineClass(NAPIEnv env,
                                 NAPICallbackInfo /*info*/) {
    NAPIStatus status;
    NAPIValue result, return_value;

    NAPIPropertyDescriptor property_descriptor = {
            "TestDefineClass",
            nullptr,
            TestDefineClass,
            nullptr,
            nullptr,
            nullptr,
            static_cast<NAPIPropertyAttributes>(NAPIEnumerable | NAPIStatic),
            nullptr};

    NAPI_CALL(env, napi_create_object(env, &return_value));

    status = napi_define_class(nullptr,
                               "TrackedFunction",
                               NAPI_AUTO_LENGTH,
                               TestDefineClass,
                               nullptr,
                               1,
                               &property_descriptor,
                               &result);

    add_returned_status(env,
                        "envIsNull",
                        return_value,
                        "Invalid argument",
                        NAPIInvalidArg,
                        status);

    napi_define_class(env,
                      nullptr,
                      NAPI_AUTO_LENGTH,
                      TestDefineClass,
                      nullptr,
                      1,
                      &property_descriptor,
                      &result);

    add_last_status(env, "nameIsNull", return_value);

    napi_define_class(env,
                      "TrackedFunction",
                      NAPI_AUTO_LENGTH,
                      nullptr,
                      nullptr,
                      1,
                      &property_descriptor,
                      &result);

    add_last_status(env, "cbIsNull", return_value);

    napi_define_class(env,
                      "TrackedFunction",
                      NAPI_AUTO_LENGTH,
                      TestDefineClass,
                      nullptr,
                      1,
                      &property_descriptor,
                      &result);

    add_last_status(env, "cbDataIsNull", return_value);

    napi_define_class(env,
                      "TrackedFunction",
                      NAPI_AUTO_LENGTH,
                      TestDefineClass,
                      nullptr,
                      1,
                      nullptr,
                      &result);

    add_last_status(env, "propertiesIsNull", return_value);


    napi_define_class(env,
                      "TrackedFunction",
                      NAPI_AUTO_LENGTH,
                      TestDefineClass,
                      nullptr,
                      1,
                      &property_descriptor,
                      nullptr);

    add_last_status(env, "resultIsNull", return_value);

    return return_value;
}

static NAPIValue GetValue(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 0;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, nullptr));

    NAPI_ASSERT(env, argc == 0, "Wrong number of arguments");

    NAPIValue number;
    NAPI_CALL(env, napi_create_double(env, value_, &number));

    return number;
}

static NAPIValue SetValue(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc == 1, "Wrong number of arguments");

    NAPI_CALL(env, napi_get_value_double(env, args[0], &value_));

    return nullptr;
}

static NAPIValue Echo(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc == 1, "Wrong number of arguments");

    return args[0];
}

static NAPIValue New(NAPIEnv env, NAPICallbackInfo info) {
    NAPIValue _this;
    NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &_this, nullptr));

    return _this;
}

static NAPIValue GetStaticValue(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 0;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, nullptr));

    NAPI_ASSERT(env, argc == 0, "Wrong number of arguments");

    NAPIValue number;
    NAPI_CALL(env, napi_create_double(env, static_value_, &number));

    return number;
}


static NAPIValue NewExtra(NAPIEnv env, NAPICallbackInfo info) {
    NAPIValue _this;
    NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &_this, nullptr));

    return _this;
}

EXTERN_C_END

TEST(TestConstructor, DefineClass) {
    NAPIValue global = nullptr;
    ASSERT_EQ(napi_get_global(globalEnv, &global), NAPIOK);

    NAPIValue number, cons;
    ASSERT_EQ(napi_create_double(globalEnv, value_, &number), NAPIOK);

    ASSERT_EQ(napi_define_class(
            globalEnv, "MyObject_Extra", -1, NewExtra, nullptr, 0, nullptr, &cons), NAPIOK);

    NAPIPropertyDescriptor properties[] = {
            {"echo",                    nullptr, Echo,            nullptr, nullptr, nullptr, NAPIEnumerable,        nullptr},
            {"readwriteValue",          nullptr, nullptr,         nullptr, nullptr, number,
                                                                                             static_cast<NAPIPropertyAttributes>(
                                                                                                     NAPIEnumerable |
                                                                                                     NAPIWritable), nullptr},
            {"readonlyValue",           nullptr, nullptr,         nullptr, nullptr, number,  NAPIEnumerable,
                                                                                                                    nullptr},
            {"hiddenValue",             nullptr, nullptr,         nullptr, nullptr, number,  NAPIDefault,           nullptr},
            {"readwriteAccessor1",      nullptr, nullptr, GetValue, SetValue,       nullptr, NAPIDefault,
                                                                                                                    nullptr},
            {"readwriteAccessor2",      nullptr, nullptr, GetValue, SetValue,       nullptr,
                                                                                             NAPIWritable,          nullptr},
            {"readonlyAccessor1",       nullptr, nullptr, GetValue,        nullptr, nullptr, NAPIDefault,
                                                                                                                    nullptr},
            {"readonlyAccessor2",       nullptr, nullptr, GetValue,        nullptr, nullptr, NAPIWritable,
                                                                                                                    nullptr},
            {"staticReadonlyAccessor1", nullptr, nullptr, GetStaticValue,  nullptr, nullptr,
                                                                                             static_cast<NAPIPropertyAttributes>(
                                                                                                     NAPIDefault |
                                                                                                     NAPIStatic),   nullptr},
            {"constructorName",         nullptr, nullptr,         nullptr, nullptr, cons,
                                                                                             static_cast<NAPIPropertyAttributes>(
                                                                                                     NAPIEnumerable |
                                                                                                     NAPIStatic),   nullptr},
            {"TestDefineClass",         nullptr, TestDefineClass, nullptr, nullptr, nullptr,
                                                                                             static_cast<NAPIPropertyAttributes>(
                                                                                                     NAPIEnumerable |
                                                                                                     NAPIStatic),   nullptr},
    };

    ASSERT_EQ(napi_define_class(globalEnv, "MyObject", NAPI_AUTO_LENGTH, New,
                                nullptr, sizeof(properties) / sizeof(*properties), properties, &cons), NAPIOK);

    ASSERT_EQ(napi_set_named_property(globalEnv, global, "addon", cons), NAPIOK);

    NAPIValue result;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(globalEnv,
                                         "(()=>{var r={991:(r,t,e)=>{e(7690);var o=e(5703);r.exports=o(\"Array\").includes},8557:(r,t,e)=>{var o=e(991),n=e(1631),a=Array.prototype,s=String.prototype;r.exports=function(r){var t=r.includes;return r===a||r instanceof Array&&t===a.includes?o:\"string\"==typeof r||r===s||r instanceof String&&t===s.includes?n:t}},1631:(r,t,e)=>{e(1035);var o=e(5703);r.exports=o(\"String\").includes},3916:r=>{r.exports=function(r){if(\"function\"!=typeof r)throw TypeError(String(r)+\" is not a function\");return r}},8479:r=>{r.exports=function(){}},6059:(r,t,e)=>{var o=e(941);r.exports=function(r){if(!o(r))throw TypeError(String(r)+\" is not an object\");return r}},1692:(r,t,e)=>{var o=e(4529),n=e(3057),a=e(9413),s=function(r){return function(t,e,s){var i,c=o(t),u=n(c.length),l=a(s,u);if(r&&e!=e){for(;u>l;)if((i=c[l++])!=i)return!0}else for(;u>l;l++)if((r||l in c)&&c[l]===e)return r||l||0;return!r&&-1}};r.exports={includes:s(!0),indexOf:s(!1)}},2532:r=>{var t={}.toString;r.exports=function(r){return t.call(r).slice(8,-1)}},7772:(r,t,e)=>{var o=e(9813)(\"match\");r.exports=function(r){var t=/./;try{\"/./\"[r](t)}catch(e){try{return t[o]=!1,\"/./\"[r](t)}catch(r){}}return!1}},2029:(r,t,e)=>{var o=e(5746),n=e(5988),a=e(1887);r.exports=o?function(r,t,e){return n.f(r,t,a(1,e))}:function(r,t,e){return r[t]=e,r}},1887:r=>{r.exports=function(r,t){return{enumerable:!(1&r),configurable:!(2&r),writable:!(4&r),value:t}}},5746:(r,t,e)=>{var o=e(5981);r.exports=!o((function(){return 7!=Object.defineProperty({},1,{get:function(){return 7}})[1]}))},1333:(r,t,e)=>{var o=e(1899),n=e(941),a=o.document,s=n(a)&&n(a.createElement);r.exports=function(r){return s?a.createElement(r):{}}},6049:(r,t,e)=>{var o=e(2532),n=e(1899);r.exports=\"process\"==o(n.process)},2861:(r,t,e)=>{var o=e(626);r.exports=o(\"navigator\",\"userAgent\")||\"\"},3385:(r,t,e)=>{var o,n,a=e(1899),s=e(2861),i=a.process,c=i&&i.versions,u=c&&c.v8;u?n=(o=u.split(\".\"))[0]+o[1]:s&&(!(o=s.match(/Edge\\/(\\d+)/))||o[1]>=74)&&(o=s.match(/Chrome\\/(\\d+)/))&&(n=o[1]),r.exports=n&&+n},5703:(r,t,e)=>{var o=e(4058);r.exports=function(r){return o[r+\"Prototype\"]}},6887:(r,t,e)=>{\"use strict\";var o=e(1899),n=e(9677).f,a=e(7252),s=e(4058),i=e(6843),c=e(2029),u=e(7457),l=function(r){var t=function(t,e,o){if(this instanceof r){switch(arguments.length){case 0:return new r;case 1:return new r(t);case 2:return new r(t,e)}return new r(t,e,o)}return r.apply(this,arguments)};return t.prototype=r.prototype,t};r.exports=function(r,t){var e,f,p,h,v,d,y,g,b=r.target,x=r.global,w=r.stat,m=r.proto,T=x?o:w?o[b]:(o[b]||{}).prototype,j=x?s:s[b]||(s[b]={}),E=j.prototype;for(p in t)e=!a(x?p:b+(w?\".\":\"#\")+p,r.forced)&&T&&u(T,p),v=j[p],e&&(d=r.noTargetGet?(g=n(T,p))&&g.value:T[p]),h=e&&d?d:t[p],e&&typeof v==typeof h||(y=r.bind&&e?i(h,o):r.wrap&&e?l(h):m&&\"function\"==typeof h?i(Function.call,h):h,(r.sham||h&&h.sham||v&&v.sham)&&c(y,\"sham\",!0),j[p]=y,m&&(u(s,f=b+\"Prototype\")||c(s,f,{}),s[f][p]=h,r.real&&E&&!E[p]&&c(E,p,h)))}},5981:r=>{r.exports=function(r){try{return!!r()}catch(r){return!0}}},6843:(r,t,e)=>{var o=e(3916);r.exports=function(r,t,e){if(o(r),void 0===t)return r;switch(e){case 0:return function(){return r.call(t)};case 1:return function(e){return r.call(t,e)};case 2:return function(e,o){return r.call(t,e,o)};case 3:return function(e,o,n){return r.call(t,e,o,n)}}return function(){return r.apply(t,arguments)}}},626:(r,t,e)=>{var o=e(4058),n=e(1899),a=function(r){return\"function\"==typeof r?r:void 0};r.exports=function(r,t){return arguments.length<2?a(o[r])||a(n[r]):o[r]&&o[r][t]||n[r]&&n[r][t]}},1899:(r,t,e)=>{var o=function(r){return r&&r.Math==Math&&r};r.exports=o(\"object\"==typeof globalThis&&globalThis)||o(\"object\"==typeof window&&window)||o(\"object\"==typeof self&&self)||o(\"object\"==typeof e.g&&e.g)||function(){return this}()||Function(\"return this\")()},7457:(r,t,e)=>{var o=e(9678),n={}.hasOwnProperty;r.exports=function(r,t){return n.call(o(r),t)}},2840:(r,t,e)=>{var o=e(5746),n=e(5981),a=e(1333);r.exports=!o&&!n((function(){return 7!=Object.defineProperty(a(\"div\"),\"a\",{get:function(){return 7}}).a}))},7026:(r,t,e)=>{var o=e(5981),n=e(2532),a=\"\".split;r.exports=o((function(){return!Object(\"z\").propertyIsEnumerable(0)}))?function(r){return\"String\"==n(r)?a.call(r,\"\"):Object(r)}:Object},7252:(r,t,e)=>{var o=e(5981),n=/#|\\.prototype\\./,a=function(r,t){var e=i[s(r)];return e==u||e!=c&&(\"function\"==typeof t?o(t):!!t)},s=a.normalize=function(r){return String(r).replace(n,\".\").toLowerCase()},i=a.data={},c=a.NATIVE=\"N\",u=a.POLYFILL=\"P\";r.exports=a},941:r=>{r.exports=function(r){return\"object\"==typeof r?null!==r:\"function\"==typeof r}},2529:r=>{r.exports=!0},685:(r,t,e)=>{var o=e(941),n=e(2532),a=e(9813)(\"match\");r.exports=function(r){var t;return o(r)&&(void 0!==(t=r[a])?!!t:\"RegExp\"==n(r))}},2497:(r,t,e)=>{var o=e(6049),n=e(3385),a=e(5981);r.exports=!!Object.getOwnPropertySymbols&&!a((function(){return!Symbol.sham&&(o?38===n:n>37&&n<41)}))},344:(r,t,e)=>{var o=e(685);r.exports=function(r){if(o(r))throw TypeError(\"The method doesn't accept regular expressions\");return r}},5988:(r,t,e)=>{var o=e(5746),n=e(2840),a=e(6059),s=e(6935),i=Object.defineProperty;t.f=o?i:function(r,t,e){if(a(r),t=s(t,!0),a(e),n)try{return i(r,t,e)}catch(r){}if(\"get\"in e||\"set\"in e)throw TypeError(\"Accessors not supported\");return\"value\"in e&&(r[t]=e.value),r}},9677:(r,t,e)=>{var o=e(5746),n=e(6760),a=e(1887),s=e(4529),i=e(6935),c=e(7457),u=e(2840),l=Object.getOwnPropertyDescriptor;t.f=o?l:function(r,t){if(r=s(r),t=i(t,!0),u)try{return l(r,t)}catch(r){}if(c(r,t))return a(!n.f.call(r,t),r[t])}},6760:(r,t)=>{\"use strict\";var e={}.propertyIsEnumerable,o=Object.getOwnPropertyDescriptor,n=o&&!e.call({1:2},1);t.f=n?function(r){var t=o(this,r);return!!t&&t.enumerable}:e},4058:r=>{r.exports={}},8219:r=>{r.exports=function(r){if(null==r)throw TypeError(\"Can't call method on \"+r);return r}},4911:(r,t,e)=>{var o=e(1899),n=e(2029);r.exports=function(r,t){try{n(o,r,t)}catch(e){o[r]=t}return t}},3030:(r,t,e)=>{var o=e(1899),n=e(4911),a=\"__core-js_shared__\",s=o[a]||n(a,{});r.exports=s},8726:(r,t,e)=>{var o=e(2529),n=e(3030);(r.exports=function(r,t){return n[r]||(n[r]=void 0!==t?t:{})})(\"versions\",[]).push({version:\"3.11.1\",mode:o?\"pure\":\"global\",copyright:\"© 2021 Denis Pushkarev (zloirock.ru)\"})},9413:(r,t,e)=>{var o=e(8459),n=Math.max,a=Math.min;r.exports=function(r,t){var e=o(r);return e<0?n(e+t,0):a(e,t)}},4529:(r,t,e)=>{var o=e(7026),n=e(8219);r.exports=function(r){return o(n(r))}},8459:r=>{var t=Math.ceil,e=Math.floor;r.exports=function(r){return isNaN(r=+r)?0:(r>0?e:t)(r)}},3057:(r,t,e)=>{var o=e(8459),n=Math.min;r.exports=function(r){return r>0?n(o(r),9007199254740991):0}},9678:(r,t,e)=>{var o=e(8219);r.exports=function(r){return Object(o(r))}},6935:(r,t,e)=>{var o=e(941);r.exports=function(r,t){if(!o(r))return r;var e,n;if(t&&\"function\"==typeof(e=r.toString)&&!o(n=e.call(r)))return n;if(\"function\"==typeof(e=r.valueOf)&&!o(n=e.call(r)))return n;if(!t&&\"function\"==typeof(e=r.toString)&&!o(n=e.call(r)))return n;throw TypeError(\"Can't convert object to primitive value\")}},9418:r=>{var t=0,e=Math.random();r.exports=function(r){return\"Symbol(\"+String(void 0===r?\"\":r)+\")_\"+(++t+e).toString(36)}},2302:(r,t,e)=>{var o=e(2497);r.exports=o&&!Symbol.sham&&\"symbol\"==typeof Symbol.iterator},9813:(r,t,e)=>{var o=e(1899),n=e(8726),a=e(7457),s=e(9418),i=e(2497),c=e(2302),u=n(\"wks\"),l=o.Symbol,f=c?l:l&&l.withoutSetter||s;r.exports=function(r){return a(u,r)&&(i||\"string\"==typeof u[r])||(i&&a(l,r)?u[r]=l[r]:u[r]=f(\"Symbol.\"+r)),u[r]}},7690:(r,t,e)=>{\"use strict\";var o=e(6887),n=e(1692).includes,a=e(8479);o({target:\"Array\",proto:!0},{includes:function(r){return n(this,r,arguments.length>1?arguments[1]:void 0)}}),a(\"includes\")},1035:(r,t,e)=>{\"use strict\";var o=e(6887),n=e(344),a=e(8219);o({target:\"String\",proto:!0,forced:!e(7772)(\"includes\")},{includes:function(r){return!!~String(a(this)).indexOf(n(r),arguments.length>1?arguments[1]:void 0)}})},3778:(r,t,e)=>{var o=e(8557);r.exports=o}},t={};function e(o){var n=t[o];if(void 0!==n)return n.exports;var a=t[o]={exports:{}};return r[o](a,a.exports,e),a.exports}e.n=r=>{var t=r&&r.__esModule?()=>r.default:()=>r;return e.d(t,{a:t}),t},e.d=(r,t)=>{for(var o in t)e.o(t,o)&&!e.o(r,o)&&Object.defineProperty(r,o,{enumerable:!0,get:t[o]})},e.g=function(){if(\"object\"==typeof globalThis)return globalThis;try{return this||new Function(\"return this\")()}catch(r){if(\"object\"==typeof window)return window}}(),e.o=(r,t)=>Object.prototype.hasOwnProperty.call(r,t),(()=>{\"use strict\";var r=e(3778),t=e.n(r),o=new globalThis.addon;globalThis.assert.strictEqual(o.echo(\"hello\"),\"hello\"),o.readwriteValue=1,globalThis.assert.strictEqual(o.readwriteValue,1),o.readwriteValue=2,globalThis.assert.strictEqual(o.readwriteValue,2),globalThis.assert.ok(o.hiddenValue);var n=[];for(var a in o)n.push(a);globalThis.assert.ok(t()(n).call(n,\"echo\")),globalThis.assert.ok(t()(n).call(n,\"readwriteValue\")),globalThis.assert.ok(t()(n).call(n,\"readonlyValue\")),globalThis.assert.ok(!t()(n).call(n,\"hiddenValue\")),globalThis.assert.ok(!t()(n).call(n,\"readwriteAccessor1\")),globalThis.assert.ok(!t()(n).call(n,\"readwriteAccessor2\")),globalThis.assert.ok(!t()(n).call(n,\"readonlyAccessor1\")),globalThis.assert.ok(!t()(n).call(n,\"readonlyAccessor2\")),o.readwriteAccessor1=1,globalThis.assert.strictEqual(o.readwriteAccessor1,1),globalThis.assert.strictEqual(o.readonlyAccessor1,1),o.readwriteAccessor2=2,globalThis.assert.strictEqual(o.readwriteAccessor2,2),globalThis.assert.strictEqual(o.readonlyAccessor2,2),globalThis.assert.strictEqual(globalThis.addon.staticReadonlyAccessor1,10),globalThis.assert.strictEqual(o.staticReadonlyAccessor1,void 0)})()})();",
                                         "https://www.didi.com/test_constructor_test.js",
                                         &result), NAPIOK);

    ASSERT_EQ(
            NAPIRunScriptWithSourceUrl(globalEnv,
                                       "(()=>{\"use strict\";globalThis.assert.strictEqual(globalThis.addon.name,\"MyObject\")})();",
                                       "https://www.didi.com/test_constructor_test2.js",
                                       &result), NAPIOK);
}