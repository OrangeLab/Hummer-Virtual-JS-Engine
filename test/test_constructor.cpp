#include <common.h>
#include <gtest/gtest.h>
#include <js_native_api_test.h>
#include <napi/js_native_api.h>

EXTERN_C_START

static double value_ = 1;
static double static_value_ = 10;

static NAPIValue GetValue(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 0;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, nullptr));

    NAPI_ASSERT(env, argc == 0, "Wrong number of arguments");

    NAPIValue number;
    NAPI_CALL(env, napi_create_double(env, value_, &number));

    return number;
}

static NAPIValue SetValue(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc == 1, "Wrong number of arguments");

    NAPI_CALL(env, napi_get_value_double(env, args[0], &value_));

    return nullptr;
}

static NAPIValue Echo(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc == 1, "Wrong number of arguments");

    return args[0];
}

static NAPIValue New(NAPIEnv env, NAPICallbackInfo info)
{
    NAPIValue _this;
    NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &_this, nullptr));

    return _this;
}

static NAPIValue GetStaticValue(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 0;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, nullptr));

    NAPI_ASSERT(env, argc == 0, "Wrong number of arguments");

    NAPIValue number;
    NAPI_CALL(env, napi_create_double(env, static_value_, &number));

    return number;
}

static NAPIValue NewExtra(NAPIEnv env, NAPICallbackInfo info)
{
    NAPIValue _this;
    NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &_this, nullptr));

    return _this;
}

EXTERN_C_END

TEST(TestConstructor, DefineClass)
{
    NAPIHandleScope handleScope;
    ASSERT_EQ(napi_open_handle_scope(globalEnv, &handleScope), NAPIOK);

    NAPIValue global;
    ASSERT_EQ(napi_get_global(globalEnv, &global), NAPIOK);

    NAPIValue number, cons;
    ASSERT_EQ(napi_create_double(globalEnv, value_, &number), NAPIOK);

    ASSERT_EQ(napi_define_class(globalEnv, "MyObject_Extra", -1, NewExtra, nullptr, 0, nullptr, &cons), NAPIOK);

    NAPIPropertyDescriptor properties[] = {
        {"echo", nullptr, Echo, nullptr, nullptr, nullptr, NAPIEnumerable, nullptr},
        {"readwriteValue", nullptr, nullptr, nullptr, nullptr, number,
         static_cast<NAPIPropertyAttributes>(NAPIEnumerable | NAPIWritable), nullptr},
        {"readonlyValue", nullptr, nullptr, nullptr, nullptr, number, NAPIEnumerable, nullptr},
        {"hiddenValue", nullptr, nullptr, nullptr, nullptr, number, NAPIDefault, nullptr},
        {"readwriteAccessor1", nullptr, nullptr, GetValue, SetValue, nullptr, NAPIDefault, nullptr},
        {"readwriteAccessor2", nullptr, nullptr, GetValue, SetValue, nullptr, NAPIWritable, nullptr},
        {"readonlyAccessor1", nullptr, nullptr, GetValue, nullptr, nullptr, NAPIDefault, nullptr},
        {"readonlyAccessor2", nullptr, nullptr, GetValue, nullptr, nullptr, NAPIWritable, nullptr},
        {"staticReadonlyAccessor1", nullptr, nullptr, GetStaticValue, nullptr, nullptr,
         static_cast<NAPIPropertyAttributes>(NAPIDefault | NAPIStatic), nullptr},
        {"constructorName", nullptr, nullptr, nullptr, nullptr, cons,
         static_cast<NAPIPropertyAttributes>(NAPIEnumerable | NAPIStatic), nullptr},
    };

    ASSERT_EQ(napi_define_class(globalEnv, "MyObject", NAPI_AUTO_LENGTH, New, nullptr,
                                sizeof(properties) / sizeof(*properties), properties, &cons),
              NAPIOK);

    ASSERT_EQ(napi_set_named_property(globalEnv, global, "addon", cons), NAPIOK);

    NAPIValue result;
    ASSERT_EQ(
        NAPIRunScript(
            globalEnv,
            "(()=>{var r={991:(r,t,e)=>{e(7690);var n=e(5703);r.exports=n(\"Array\").includes},8557:(r,t,e)=>{var "
            "n=e(991),o=e(1631),a=Array.prototype,i=String.prototype;r.exports=function(r){var t=r.includes;return "
            "r===a||r instanceof Array&&t===a.includes?n:\"string\"==typeof r||r===i||r instanceof "
            "String&&t===i.includes?o:t}},1631:(r,t,e)=>{e(1035);var "
            "n=e(5703);r.exports=n(\"String\").includes},3916:r=>{r.exports=function(r){if(\"function\"!=typeof "
            "r)throw TypeError(String(r)+\" is not a function\");return "
            "r}},8479:r=>{r.exports=function(){}},6059:(r,t,e)=>{var n=e(941);r.exports=function(r){if(!n(r))throw "
            "TypeError(String(r)+\" is not an object\");return r}},1692:(r,t,e)=>{var "
            "n=e(4529),o=e(3057),a=e(9413),i=function(r){return function(t,e,i){var "
            "u,s=n(t),c=o(s.length),l=a(i,c);if(r&&e!=e){for(;c>l;)if((u=s[l++])!=u)return!0}else "
            "for(;c>l;l++)if((r||l in s)&&s[l]===e)return "
            "r||l||0;return!r&&-1}};r.exports={includes:i(!0),indexOf:i(!1)}},2532:r=>{var "
            "t={}.toString;r.exports=function(r){return t.call(r).slice(8,-1)}},7772:(r,t,e)=>{var "
            "n=e(9813)(\"match\");r.exports=function(r){var t=/./;try{\"/./\"[r](t)}catch(e){try{return "
            "t[n]=!1,\"/./\"[r](t)}catch(r){}}return!1}},2029:(r,t,e)=>{var "
            "n=e(5746),o=e(5988),a=e(1887);r.exports=n?function(r,t,e){return o.f(r,t,a(1,e))}:function(r,t,e){return "
            "r[t]=e,r}},1887:r=>{r.exports=function(r,t){return{enumerable:!(1&r),configurable:!(2&r),writable:!(4&r),"
            "value:t}}},5746:(r,t,e)=>{var n=e(5981);r.exports=!n((function(){return "
            "7!=Object.defineProperty({},1,{get:function(){return 7}})[1]}))},1333:(r,t,e)=>{var "
            "n=e(1899),o=e(941),a=n.document,i=o(a)&&o(a.createElement);r.exports=function(r){return "
            "i?a.createElement(r):{}}},6049:(r,t,e)=>{var "
            "n=e(2532),o=e(1899);r.exports=\"process\"==n(o.process)},2861:(r,t,e)=>{var "
            "n=e(626);r.exports=n(\"navigator\",\"userAgent\")||\"\"},3385:(r,t,e)=>{var "
            "n,o,a=e(1899),i=e(2861),u=a.process,s=u&&u.versions,c=s&&s.v8;c?o=(n=c.split(\".\"))[0]+n[1]:i&&(!(n=i."
            "match(/Edge\\/(\\d+)/))||n[1]>=74)&&(n=i.match(/Chrome\\/(\\d+)/"
            "))&&(o=n[1]),r.exports=o&&+o},5703:(r,t,e)=>{var n=e(4058);r.exports=function(r){return "
            "n[r+\"Prototype\"]}},6887:(r,t,e)=>{\"use strict\";var "
            "n=e(1899),o=e(9677).f,a=e(7252),i=e(4058),u=e(6843),s=e(2029),c=e(7457),l=function(r){var "
            "t=function(t,e,n){if(this instanceof r){switch(arguments.length){case 0:return new r;case 1:return new "
            "r(t);case 2:return new r(t,e)}return new r(t,e,n)}return r.apply(this,arguments)};return "
            "t.prototype=r.prototype,t};r.exports=function(r,t){var "
            "e,f,p,v,h,y,d,g,b=r.target,x=r.global,w=r.stat,m=r.proto,T=x?n:w?n[b]:(n[b]||{}).prototype,j=x?i:i[b]||(i["
            "b]={}),S=j.prototype;for(p in "
            "t)e=!a(x?p:b+(w?\".\":\"#\")+p,r.forced)&&T&&c(T,p),h=j[p],e&&(y=r.noTargetGet?(g=o(T,p))&&g.value:T[p]),"
            "v=e&&y?y:t[p],e&&typeof h==typeof v||(d=r.bind&&e?u(v,n):r.wrap&&e?l(v):m&&\"function\"==typeof "
            "v?u(Function.call,v):v,(r.sham||v&&v.sham||h&&h.sham)&&s(d,\"sham\",!0),j[p]=d,m&&(c(i,f=b+\"Prototype\")|"
            "|s(i,f,{}),i[f][p]=v,r.real&&S&&!S[p]&&s(S,p,v)))}},5981:r=>{r.exports=function(r){try{return!!r()}catch("
            "r){return!0}}},6843:(r,t,e)=>{var n=e(3916);r.exports=function(r,t,e){if(n(r),void 0===t)return "
            "r;switch(e){case 0:return function(){return r.call(t)};case 1:return function(e){return r.call(t,e)};case "
            "2:return function(e,n){return r.call(t,e,n)};case 3:return function(e,n,o){return r.call(t,e,n,o)}}return "
            "function(){return r.apply(t,arguments)}}},626:(r,t,e)=>{var "
            "n=e(4058),o=e(1899),a=function(r){return\"function\"==typeof r?r:void 0};r.exports=function(r,t){return "
            "arguments.length<2?a(n[r])||a(o[r]):n[r]&&n[r][t]||o[r]&&o[r][t]}},1899:(r,t,e)=>{var "
            "n=function(r){return r&&r.Math==Math&&r};r.exports=n(\"object\"==typeof "
            "globalThis&&globalThis)||n(\"object\"==typeof window&&window)||n(\"object\"==typeof "
            "self&&self)||n(\"object\"==typeof e.g&&e.g)||function(){return this}()||Function(\"return "
            "this\")()},7457:(r,t,e)=>{var n=e(9678),o={}.hasOwnProperty;r.exports=function(r,t){return "
            "o.call(n(r),t)}},2840:(r,t,e)=>{var n=e(5746),o=e(5981),a=e(1333);r.exports=!n&&!o((function(){return "
            "7!=Object.defineProperty(a(\"div\"),\"a\",{get:function(){return 7}}).a}))},7026:(r,t,e)=>{var "
            "n=e(5981),o=e(2532),a=\"\".split;r.exports=n((function(){return!Object(\"z\").propertyIsEnumerable(0)}))?"
            "function(r){return\"String\"==o(r)?a.call(r,\"\"):Object(r)}:Object},7252:(r,t,e)=>{var "
            "n=e(5981),o=/#|\\.prototype\\./,a=function(r,t){var e=u[i(r)];return e==c||e!=s&&(\"function\"==typeof "
            "t?n(t):!!t)},i=a.normalize=function(r){return "
            "String(r).replace(o,\".\").toLowerCase()},u=a.data={},s=a.NATIVE=\"N\",c=a.POLYFILL=\"P\";r.exports=a},"
            "941:r=>{r.exports=function(r){return\"object\"==typeof r?null!==r:\"function\"==typeof "
            "r}},2529:r=>{r.exports=!0},685:(r,t,e)=>{var "
            "n=e(941),o=e(2532),a=e(9813)(\"match\");r.exports=function(r){var t;return n(r)&&(void "
            "0!==(t=r[a])?!!t:\"RegExp\"==o(r))}},2497:(r,t,e)=>{var "
            "n=e(6049),o=e(3385),a=e(5981);r.exports=!!Object.getOwnPropertySymbols&&!a((function(){return!Symbol.sham&"
            "&(n?38===o:o>37&&o<41)}))},344:(r,t,e)=>{var n=e(685);r.exports=function(r){if(n(r))throw TypeError(\"The "
            "method doesn't accept regular expressions\");return r}},5988:(r,t,e)=>{var "
            "n=e(5746),o=e(2840),a=e(6059),i=e(6935),u=Object.defineProperty;t.f=n?u:function(r,t,e){if(a(r),t=i(t,!0),"
            "a(e),o)try{return u(r,t,e)}catch(r){}if(\"get\"in e||\"set\"in e)throw TypeError(\"Accessors not "
            "supported\");return\"value\"in e&&(r[t]=e.value),r}},9677:(r,t,e)=>{var "
            "n=e(5746),o=e(6760),a=e(1887),i=e(4529),u=e(6935),s=e(7457),c=e(2840),l=Object.getOwnPropertyDescriptor;t."
            "f=n?l:function(r,t){if(r=i(r),t=u(t,!0),c)try{return l(r,t)}catch(r){}if(s(r,t))return "
            "a(!o.f.call(r,t),r[t])}},6760:(r,t)=>{\"use strict\";var "
            "e={}.propertyIsEnumerable,n=Object.getOwnPropertyDescriptor,o=n&&!e.call({1:2},1);t.f=o?function(r){var "
            "t=n(this,r);return!!t&&t.enumerable}:e},4058:r=>{r.exports={}},8219:r=>{r.exports=function(r){if(null==r)"
            "throw TypeError(\"Can't call method on \"+r);return r}},4911:(r,t,e)=>{var "
            "n=e(1899),o=e(2029);r.exports=function(r,t){try{o(n,r,t)}catch(e){n[r]=t}return t}},3030:(r,t,e)=>{var "
            "n=e(1899),o=e(4911),a=\"__core-js_shared__\",i=n[a]||o(a,{});r.exports=i},8726:(r,t,e)=>{var "
            "n=e(2529),o=e(3030);(r.exports=function(r,t){return o[r]||(o[r]=void "
            "0!==t?t:{})})(\"versions\",[]).push({version:\"3.11.1\",mode:n?\"pure\":\"global\",copyright:\"© 2021 "
            "Denis Pushkarev (zloirock.ru)\"})},9413:(r,t,e)=>{var "
            "n=e(8459),o=Math.max,a=Math.min;r.exports=function(r,t){var e=n(r);return "
            "e<0?o(e+t,0):a(e,t)}},4529:(r,t,e)=>{var n=e(7026),o=e(8219);r.exports=function(r){return "
            "n(o(r))}},8459:r=>{var t=Math.ceil,e=Math.floor;r.exports=function(r){return "
            "isNaN(r=+r)?0:(r>0?e:t)(r)}},3057:(r,t,e)=>{var n=e(8459),o=Math.min;r.exports=function(r){return "
            "r>0?o(n(r),9007199254740991):0}},9678:(r,t,e)=>{var n=e(8219);r.exports=function(r){return "
            "Object(n(r))}},6935:(r,t,e)=>{var n=e(941);r.exports=function(r,t){if(!n(r))return r;var "
            "e,o;if(t&&\"function\"==typeof(e=r.toString)&&!n(o=e.call(r)))return "
            "o;if(\"function\"==typeof(e=r.valueOf)&&!n(o=e.call(r)))return "
            "o;if(!t&&\"function\"==typeof(e=r.toString)&&!n(o=e.call(r)))return o;throw TypeError(\"Can't convert "
            "object to primitive value\")}},9418:r=>{var "
            "t=0,e=Math.random();r.exports=function(r){return\"Symbol(\"+String(void "
            "0===r?\"\":r)+\")_\"+(++t+e).toString(36)}},2302:(r,t,e)=>{var "
            "n=e(2497);r.exports=n&&!Symbol.sham&&\"symbol\"==typeof Symbol.iterator},9813:(r,t,e)=>{var "
            "n=e(1899),o=e(8726),a=e(7457),i=e(9418),u=e(2497),s=e(2302),c=o(\"wks\"),l=n.Symbol,f=s?l:l&&l."
            "withoutSetter||i;r.exports=function(r){return a(c,r)&&(u||\"string\"==typeof "
            "c[r])||(u&&a(l,r)?c[r]=l[r]:c[r]=f(\"Symbol.\"+r)),c[r]}},7690:(r,t,e)=>{\"use strict\";var "
            "n=e(6887),o=e(1692).includes,a=e(8479);n({target:\"Array\",proto:!0},{includes:function(r){return "
            "o(this,r,arguments.length>1?arguments[1]:void 0)}}),a(\"includes\")},1035:(r,t,e)=>{\"use strict\";var "
            "n=e(6887),o=e(344),a=e(8219);n({target:\"String\",proto:!0,forced:!e(7772)(\"includes\")},{includes:"
            "function(r){return!!~String(a(this)).indexOf(o(r),arguments.length>1?arguments[1]:void "
            "0)}})},3778:(r,t,e)=>{var n=e(8557);r.exports=n}},t={};function e(n){var o=t[n];if(void 0!==o)return "
            "o.exports;var a=t[n]={exports:{}};return r[n](a,a.exports,e),a.exports}e.n=r=>{var "
            "t=r&&r.__esModule?()=>r.default:()=>r;return e.d(t,{a:t}),t},e.d=(r,t)=>{for(var n in "
            "t)e.o(t,n)&&!e.o(r,n)&&Object.defineProperty(r,n,{enumerable:!0,get:t[n]})},e.g=function(){if(\"object\"=="
            "typeof globalThis)return globalThis;try{return this||new Function(\"return "
            "this\")()}catch(r){if(\"object\"==typeof window)return "
            "window}}(),e.o=(r,t)=>Object.prototype.hasOwnProperty.call(r,t),(()=>{\"use strict\";var "
            "r=e(3778),t=e.n(r),n=new "
            "globalThis.addon;globalThis.assert.strictEqual(n.echo(\"hello\"),\"hello\"),n.readwriteValue=1,globalThis."
            "assert.strictEqual(n.readwriteValue,1),n.readwriteValue=2,globalThis.assert.strictEqual(n.readwriteValue,"
            "2),globalThis.assert.ok(n.hiddenValue);var o=[];for(var a in "
            "n)o.push(a);console.log(o),globalThis.assert.ok(t()(o).call(o,\"echo\")),globalThis.assert.ok(t()(o).call("
            "o,\"readwriteValue\")),globalThis.assert.ok(t()(o).call(o,\"readonlyValue\")),globalThis.assert.ok(!t()(o)"
            ".call(o,\"hiddenValue\")),globalThis.assert.ok(!t()(o).call(o,\"readwriteAccessor1\")),globalThis.assert."
            "ok(!t()(o).call(o,\"readwriteAccessor2\")),globalThis.assert.ok(!t()(o).call(o,\"readonlyAccessor1\")),"
            "globalThis.assert.ok(!t()(o).call(o,\"readonlyAccessor2\"))})()})();",
            "https://www.napi.com/test_constructor_test.js", &result),
        NAPIOK);

    //    ASSERT_EQ(
    //        NAPIRunScript(globalEnv,
    //                      "(()=>{\"use
    //                      strict\";globalThis.assert.strictEqual(globalThis.addon.name,\"MyObject\")})();",
    //                      "https://www.napi.com/test_constructor_test2.js", &result),
    //        NAPIOK);
    ASSERT_EQ(napi_close_handle_scope(globalEnv, handleScope), NAPIOK);
}