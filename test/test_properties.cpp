#include <common.h>
#include <js_native_api_test.h>

EXTERN_C_START

static double value_ = 1;

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

    return getUndefined(env);
}

static NAPIValue Echo(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc == 1, "Wrong number of arguments");

    return args[0];
}

static NAPIValue HasNamedProperty(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 2;
    NAPIValue args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc == 2, "Wrong number of arguments");

    // Extract the name of the property to check
    char buffer[128];
    size_t copied;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], buffer, sizeof(buffer), &copied));

    // do the check and create the boolean return value
    bool value;
    NAPIValue result;
    NAPI_CALL(env, napi_has_named_property(env, args[0], buffer, &value));
    NAPI_CALL(env, napi_get_boolean(env, value, &result));

    return result;
}

EXTERN_C_END

TEST_F(Test, TestProperties)
{
    NAPIValue number;
    ASSERT_EQ(napi_create_double(globalEnv, value_, &number), NAPIOK);

    NAPIValue name_value;
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "NameKeyValue", NAPI_AUTO_LENGTH, &name_value), NAPIOK);

    NAPIValue symbol_description;
    NAPIValue name_symbol;
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "NameKeySymbol", NAPI_AUTO_LENGTH, &symbol_description), NAPIOK);
    ASSERT_EQ(napi_create_symbol(globalEnv, symbol_description, &name_symbol), NAPIOK);

    NAPIPropertyDescriptor properties[] = {
        {"echo", getUndefined(globalEnv), Echo, 0, 0, getUndefined(globalEnv), NAPIEnumerable, 0},
        {"readwriteValue", getUndefined(globalEnv), 0, 0, 0, number,
         static_cast<NAPIPropertyAttributes>(NAPIEnumerable | NAPIWritable), 0},
        {"readonlyValue", getUndefined(globalEnv), 0, 0, 0, number, NAPIEnumerable, 0},
        {"hiddenValue", getUndefined(globalEnv), 0, 0, 0, number, NAPIDefault, 0},
        {nullptr, name_value, 0, 0, 0, number, NAPIEnumerable, 0},
        {nullptr, name_symbol, 0, 0, 0, number, NAPIEnumerable, 0},
        {"readwriteAccessor1", getUndefined(globalEnv), 0, GetValue, SetValue, getUndefined(globalEnv), NAPIDefault, 0},
        {"readwriteAccessor2", getUndefined(globalEnv), 0, GetValue, SetValue, getUndefined(globalEnv), NAPIWritable,
         0},
        {"readonlyAccessor1", getUndefined(globalEnv), 0, GetValue, nullptr, getUndefined(globalEnv), NAPIDefault, 0},
        {"readonlyAccessor2", getUndefined(globalEnv), 0, GetValue, nullptr, getUndefined(globalEnv), NAPIWritable, 0},
        {"hasNamedProperty", getUndefined(globalEnv), HasNamedProperty, 0, 0, getUndefined(globalEnv), NAPIDefault, 0},
    };

    ASSERT_EQ(napi_define_properties(globalEnv, exports, sizeof(properties) / sizeof(*properties), properties), NAPIOK);

    NAPIValue result;
    ASSERT_EQ(
        NAPIRunScriptWithSourceUrl(
            globalEnv,
            "(()=>{var t={991:(t,r,e)=>{e(7690);var "
            "o=e(5703);t.exports=o(\"Array\").includes},4900:(t,r,e)=>{e(186);var "
            "o=e(5703);t.exports=o(\"Array\").slice},8557:(t,r,e)=>{var "
            "o=e(991),n=e(1631),a=Array.prototype,i=String.prototype;t.exports=function(t){var r=t.includes;return "
            "t===a||t instanceof Array&&r===a.includes?o:\"string\"==typeof t||t===i||t instanceof "
            "String&&r===i.includes?n:r}},9601:(t,r,e)=>{var o=e(4900),n=Array.prototype;t.exports=function(t){var "
            "r=t.slice;return t===n||t instanceof Array&&r===n.slice?o:r}},498:(t,r,e)=>{e(5824);var "
            "o=e(4058);t.exports=o.Object.getOwnPropertySymbols},1631:(t,r,e)=>{e(1035);var "
            "o=e(5703);t.exports=o(\"String\").includes},3916:t=>{t.exports=function(t){if(\"function\"!=typeof "
            "t)throw TypeError(String(t)+\" is not a function\");return "
            "t}},8479:t=>{t.exports=function(){}},6059:(t,r,e)=>{var o=e(941);t.exports=function(t){if(!o(t))throw "
            "TypeError(String(t)+\" is not an object\");return t}},1692:(t,r,e)=>{var "
            "o=e(4529),n=e(3057),a=e(9413),i=function(t){return function(r,e,i){var "
            "s,c=o(r),u=n(c.length),l=a(i,u);if(t&&e!=e){for(;u>l;)if((s=c[l++])!=s)return!0}else "
            "for(;u>l;l++)if((t||l in c)&&c[l]===e)return "
            "t||l||0;return!t&&-1}};t.exports={includes:i(!0),indexOf:i(!1)}},3610:(t,r,e)=>{var "
            "o=e(6843),n=e(7026),a=e(9678),i=e(3057),s=e(4692),c=[].push,u=function(t){var "
            "r=1==t,e=2==t,u=3==t,l=4==t,f=6==t,p=7==t,d=5==t||f;return function(h,v,y,g){for(var "
            "b,m,x=a(h),w=n(x),T=o(v,y,3),O=i(w.length),S=0,j=g||s,E=r?j(h,O):e||p?j(h,0):void 0;O>S;S++)if((d||S in "
            "w)&&(m=T(b=w[S],S,x),t))if(r)E[S]=m;else if(m)switch(t){case 3:return!0;case 5:return b;case 6:return "
            "S;case 2:c.call(E,b)}else switch(t){case 4:return!1;case 7:c.call(E,b)}return "
            "f?-1:u||l?l:E}};t.exports={forEach:u(0),map:u(1),filter:u(2),some:u(3),every:u(4),find:u(5),findIndex:u(6)"
            ",filterOut:u(7)}},568:(t,r,e)=>{var "
            "o=e(5981),n=e(9813),a=e(3385),i=n(\"species\");t.exports=function(t){return a>=51||!o((function(){var "
            "r=[];return(r.constructor={})[i]=function(){return{foo:1}},1!==r[t](Boolean).foo}))}},4692:(t,r,e)=>{var "
            "o=e(941),n=e(1052),a=e(9813)(\"species\");t.exports=function(t,r){var e;return "
            "n(t)&&(\"function\"!=typeof(e=t.constructor)||e!==Array&&!n(e.prototype)?o(e)&&null===(e=e[a])&&(e=void "
            "0):e=void 0),new(void 0===e?Array:e)(0===r?0:r)}},2532:t=>{var r={}.toString;t.exports=function(t){return "
            "r.call(t).slice(8,-1)}},9697:(t,r,e)=>{var "
            "o=e(2885),n=e(2532),a=e(9813)(\"toStringTag\"),i=\"Arguments\"==n(function(){return "
            "arguments}());t.exports=o?n:function(t){var r,e,o;return void "
            "0===t?\"Undefined\":null===t?\"Null\":\"string\"==typeof(e=function(t,r){try{return "
            "t[r]}catch(t){}}(r=Object(t),a))?e:i?n(r):\"Object\"==(o=n(r))&&\"function\"==typeof "
            "r.callee?\"Arguments\":o}},7772:(t,r,e)=>{var o=e(9813)(\"match\");t.exports=function(t){var "
            "r=/./;try{\"/./\"[t](r)}catch(e){try{return "
            "r[o]=!1,\"/./\"[t](r)}catch(t){}}return!1}},2029:(t,r,e)=>{var "
            "o=e(5746),n=e(5988),a=e(1887);t.exports=o?function(t,r,e){return n.f(t,r,a(1,e))}:function(t,r,e){return "
            "t[r]=e,t}},1887:t=>{t.exports=function(t,r){return{enumerable:!(1&t),configurable:!(2&t),writable:!(4&t),"
            "value:r}}},5449:(t,r,e)=>{\"use strict\";var o=e(6935),n=e(5988),a=e(1887);t.exports=function(t,r,e){var "
            "i=o(r);i in t?n.f(t,i,a(0,e)):t[i]=e}},6349:(t,r,e)=>{var "
            "o=e(4058),n=e(7457),a=e(1477),i=e(5988).f;t.exports=function(t){var "
            "r=o.Symbol||(o.Symbol={});n(r,t)||i(r,t,{value:a.f(t)})}},5746:(t,r,e)=>{var "
            "o=e(5981);t.exports=!o((function(){return 7!=Object.defineProperty({},1,{get:function(){return "
            "7}})[1]}))},1333:(t,r,e)=>{var "
            "o=e(1899),n=e(941),a=o.document,i=n(a)&&n(a.createElement);t.exports=function(t){return "
            "i?a.createElement(t):{}}},6049:(t,r,e)=>{var "
            "o=e(2532),n=e(1899);t.exports=\"process\"==o(n.process)},2861:(t,r,e)=>{var "
            "o=e(626);t.exports=o(\"navigator\",\"userAgent\")||\"\"},3385:(t,r,e)=>{var "
            "o,n,a=e(1899),i=e(2861),s=a.process,c=s&&s.versions,u=c&&c.v8;u?n=(o=u.split(\".\"))[0]+o[1]:i&&(!(o=i."
            "match(/Edge\\/(\\d+)/))||o[1]>=74)&&(o=i.match(/Chrome\\/(\\d+)/"
            "))&&(n=o[1]),t.exports=n&&+n},5703:(t,r,e)=>{var o=e(4058);t.exports=function(t){return "
            "o[t+\"Prototype\"]}},6759:t=>{t.exports=[\"constructor\",\"hasOwnProperty\",\"isPrototypeOf\","
            "\"propertyIsEnumerable\",\"toLocaleString\",\"toString\",\"valueOf\"]},6887:(t,r,e)=>{\"use strict\";var "
            "o=e(1899),n=e(9677).f,a=e(7252),i=e(4058),s=e(6843),c=e(2029),u=e(7457),l=function(t){var "
            "r=function(r,e,o){if(this instanceof t){switch(arguments.length){case 0:return new t;case 1:return new "
            "t(r);case 2:return new t(r,e)}return new t(r,e,o)}return t.apply(this,arguments)};return "
            "r.prototype=t.prototype,r};t.exports=function(t,r){var "
            "e,f,p,d,h,v,y,g,b=t.target,m=t.global,x=t.stat,w=t.proto,T=m?o:x?o[b]:(o[b]||{}).prototype,O=m?i:i[b]||(i["
            "b]={}),S=O.prototype;for(p in "
            "r)e=!a(m?p:b+(x?\".\":\"#\")+p,t.forced)&&T&&u(T,p),h=O[p],e&&(v=t.noTargetGet?(g=n(T,p))&&g.value:T[p]),"
            "d=e&&v?v:r[p],e&&typeof h==typeof d||(y=t.bind&&e?s(d,o):t.wrap&&e?l(d):w&&\"function\"==typeof "
            "d?s(Function.call,d):d,(t.sham||d&&d.sham||h&&h.sham)&&c(y,\"sham\",!0),O[p]=y,w&&(u(i,f=b+\"Prototype\")|"
            "|c(i,f,{}),i[f][p]=d,t.real&&S&&!S[p]&&c(S,p,d)))}},5981:t=>{t.exports=function(t){try{return!!t()}catch("
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
            "n.call(o(t),r)}},7748:t=>{t.exports={}},5463:(t,r,e)=>{var "
            "o=e(626);t.exports=o(\"document\",\"documentElement\")},2840:(t,r,e)=>{var "
            "o=e(5746),n=e(5981),a=e(1333);t.exports=!o&&!n((function(){return "
            "7!=Object.defineProperty(a(\"div\"),\"a\",{get:function(){return 7}}).a}))},7026:(t,r,e)=>{var "
            "o=e(5981),n=e(2532),a=\"\".split;t.exports=o((function(){return!Object(\"z\").propertyIsEnumerable(0)}))?"
            "function(t){return\"String\"==n(t)?a.call(t,\"\"):Object(t)}:Object},1302:(t,r,e)=>{var "
            "o=e(3030),n=Function.toString;\"function\"!=typeof o.inspectSource&&(o.inspectSource=function(t){return "
            "n.call(t)}),t.exports=o.inspectSource},5402:(t,r,e)=>{var "
            "o,n,a,i=e(8019),s=e(1899),c=e(941),u=e(2029),l=e(7457),f=e(3030),p=e(4262),d=e(7748),h=\"Object already "
            "initialized\",v=s.WeakMap;if(i){var y=f.state||(f.state=new "
            "v),g=y.get,b=y.has,m=y.set;o=function(t,r){if(b.call(y,t))throw new TypeError(h);return "
            "r.facade=t,m.call(y,t,r),r},n=function(t){return g.call(y,t)||{}},a=function(t){return "
            "b.call(y,t)}}else{var x=p(\"state\");d[x]=!0,o=function(t,r){if(l(t,x))throw new TypeError(h);return "
            "r.facade=t,u(t,x,r),r},n=function(t){return l(t,x)?t[x]:{}},a=function(t){return "
            "l(t,x)}}t.exports={set:o,get:n,has:a,enforce:function(t){return "
            "a(t)?n(t):o(t,{})},getterFor:function(t){return function(r){var e;if(!c(r)||(e=n(r)).type!==t)throw "
            "TypeError(\"Incompatible receiver, \"+t+\" required\");return e}}}},1052:(t,r,e)=>{var "
            "o=e(2532);t.exports=Array.isArray||function(t){return\"Array\"==o(t)}},7252:(t,r,e)=>{var "
            "o=e(5981),n=/#|\\.prototype\\./,a=function(t,r){var e=s[i(t)];return e==u||e!=c&&(\"function\"==typeof "
            "r?o(r):!!r)},i=a.normalize=function(t){return "
            "String(t).replace(n,\".\").toLowerCase()},s=a.data={},c=a.NATIVE=\"N\",u=a.POLYFILL=\"P\";t.exports=a},"
            "941:t=>{t.exports=function(t){return\"object\"==typeof t?null!==t:\"function\"==typeof "
            "t}},2529:t=>{t.exports=!0},685:(t,r,e)=>{var "
            "o=e(941),n=e(2532),a=e(9813)(\"match\");t.exports=function(t){var r;return o(t)&&(void "
            "0!==(r=t[a])?!!r:\"RegExp\"==n(t))}},2497:(t,r,e)=>{var "
            "o=e(6049),n=e(3385),a=e(5981);t.exports=!!Object.getOwnPropertySymbols&&!a((function(){return!Symbol.sham&"
            "&(o?38===n:n>37&&n<41)}))},8019:(t,r,e)=>{var "
            "o=e(1899),n=e(1302),a=o.WeakMap;t.exports=\"function\"==typeof a&&/native "
            "code/.test(n(a))},344:(t,r,e)=>{var o=e(685);t.exports=function(t){if(o(t))throw TypeError(\"The method "
            "doesn't accept regular expressions\");return t}},9290:(t,r,e)=>{var "
            "o,n=e(6059),a=e(9938),i=e(6759),s=e(7748),c=e(5463),u=e(1333),l=e(4262)(\"IE_PROTO\"),f=function(){},p="
            "function(t){return\"<script>\"+t+\"<\\/script>\"},d=function(){try{o=document.domain&&new "
            "ActiveXObject(\"htmlfile\")}catch(t){}var t,r;d=o?function(t){t.write(p(\"\")),t.close();var "
            "r=t.parentWindow.Object;return "
            "t=null,r}(o):((r=u(\"iframe\")).style.display=\"none\",c.appendChild(r),r.src=String(\"javascript:\"),(t="
            "r.contentWindow.document).open(),t.write(p(\"document.F=Object\")),t.close(),t.F);for(var "
            "e=i.length;e--;)delete d.prototype[i[e]];return d()};s[l]=!0,t.exports=Object.create||function(t,r){var "
            "e;return null!==t?(f.prototype=n(t),e=new f,f.prototype=null,e[l]=t):e=d(),void "
            "0===r?e:a(e,r)}},9938:(t,r,e)=>{var "
            "o=e(5746),n=e(5988),a=e(6059),i=e(4771);t.exports=o?Object.defineProperties:function(t,r){a(t);for(var "
            "e,o=i(r),s=o.length,c=0;s>c;)n.f(t,e=o[c++],r[e]);return t}},5988:(t,r,e)=>{var "
            "o=e(5746),n=e(2840),a=e(6059),i=e(6935),s=Object.defineProperty;r.f=o?s:function(t,r,e){if(a(t),r=i(r,!0),"
            "a(e),n)try{return s(t,r,e)}catch(t){}if(\"get\"in e||\"set\"in e)throw TypeError(\"Accessors not "
            "supported\");return\"value\"in e&&(t[r]=e.value),t}},9677:(t,r,e)=>{var "
            "o=e(5746),n=e(6760),a=e(1887),i=e(4529),s=e(6935),c=e(7457),u=e(2840),l=Object.getOwnPropertyDescriptor;r."
            "f=o?l:function(t,r){if(t=i(t),r=s(r,!0),u)try{return l(t,r)}catch(t){}if(c(t,r))return "
            "a(!n.f.call(t,r),t[r])}},684:(t,r,e)=>{var o=e(4529),n=e(946).f,a={}.toString,i=\"object\"==typeof "
            "window&&window&&Object.getOwnPropertyNames?Object.getOwnPropertyNames(window):[];t.exports.f=function(t){"
            "return i&&\"[object Window]\"==a.call(t)?function(t){try{return n(t)}catch(t){return "
            "i.slice()}}(t):n(o(t))}},946:(t,r,e)=>{var "
            "o=e(5629),n=e(6759).concat(\"length\",\"prototype\");r.f=Object.getOwnPropertyNames||function(t){return "
            "o(t,n)}},7857:(t,r)=>{r.f=Object.getOwnPropertySymbols},5629:(t,r,e)=>{var "
            "o=e(7457),n=e(4529),a=e(1692).indexOf,i=e(7748);t.exports=function(t,r){var e,s=n(t),c=0,u=[];for(e in "
            "s)!o(i,e)&&o(s,e)&&u.push(e);for(;r.length>c;)o(s,e=r[c++])&&(~a(u,e)||u.push(e));return "
            "u}},4771:(t,r,e)=>{var o=e(5629),n=e(6759);t.exports=Object.keys||function(t){return "
            "o(t,n)}},6760:(t,r)=>{\"use strict\";var "
            "e={}.propertyIsEnumerable,o=Object.getOwnPropertyDescriptor,n=o&&!e.call({1:2},1);r.f=n?function(t){var "
            "r=o(this,t);return!!r&&r.enumerable}:e},5623:(t,r,e)=>{\"use strict\";var "
            "o=e(2885),n=e(9697);t.exports=o?{}.toString:function(){return\"[object "
            "\"+n(this)+\"]\"}},4058:t=>{t.exports={}},9754:(t,r,e)=>{var "
            "o=e(2029);t.exports=function(t,r,e,n){n&&n.enumerable?t[r]=e:o(t,r,e)}},8219:t=>{t.exports=function(t){if("
            "null==t)throw TypeError(\"Can't call method on \"+t);return t}},4911:(t,r,e)=>{var "
            "o=e(1899),n=e(2029);t.exports=function(t,r){try{n(o,t,r)}catch(e){o[t]=r}return r}},904:(t,r,e)=>{var "
            "o=e(2885),n=e(5988).f,a=e(2029),i=e(7457),s=e(5623),c=e(9813)(\"toStringTag\");t.exports=function(t,r,e,u)"
            "{if(t){var "
            "l=e?t:t.prototype;i(l,c)||n(l,c,{configurable:!0,value:r}),u&&!o&&a(l,\"toString\",s)}}},4262:(t,r,e)=>{"
            "var o=e(8726),n=e(9418),a=o(\"keys\");t.exports=function(t){return a[t]||(a[t]=n(t))}},3030:(t,r,e)=>{var "
            "o=e(1899),n=e(4911),a=\"__core-js_shared__\",i=o[a]||n(a,{});t.exports=i},8726:(t,r,e)=>{var "
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
            "object to primitive value\")}},2885:(t,r,e)=>{var "
            "o={};o[e(9813)(\"toStringTag\")]=\"z\",t.exports=\"[object z]\"===String(o)},9418:t=>{var "
            "r=0,e=Math.random();t.exports=function(t){return\"Symbol(\"+String(void "
            "0===t?\"\":t)+\")_\"+(++r+e).toString(36)}},2302:(t,r,e)=>{var "
            "o=e(2497);t.exports=o&&!Symbol.sham&&\"symbol\"==typeof Symbol.iterator},1477:(t,r,e)=>{var "
            "o=e(9813);r.f=o},9813:(t,r,e)=>{var "
            "o=e(1899),n=e(8726),a=e(7457),i=e(9418),s=e(2497),c=e(2302),u=n(\"wks\"),l=o.Symbol,f=c?l:l&&l."
            "withoutSetter||i;t.exports=function(t){return a(u,t)&&(s||\"string\"==typeof "
            "u[t])||(s&&a(l,t)?u[t]=l[t]:u[t]=f(\"Symbol.\"+t)),u[t]}},7690:(t,r,e)=>{\"use strict\";var "
            "o=e(6887),n=e(1692).includes,a=e(8479);o({target:\"Array\",proto:!0},{includes:function(t){return "
            "n(this,t,arguments.length>1?arguments[1]:void 0)}}),a(\"includes\")},186:(t,r,e)=>{\"use strict\";var "
            "o=e(6887),n=e(941),a=e(1052),i=e(9413),s=e(3057),c=e(4529),u=e(5449),l=e(9813),f=e(568)(\"slice\"),p=l("
            "\"species\"),d=[].slice,h=Math.max;o({target:\"Array\",proto:!0,forced:!f},{slice:function(t,r){var "
            "e,o,l,f=c(this),v=s(f.length),y=i(t,v),g=i(void "
            "0===r?v:r,v);if(a(f)&&(\"function\"!=typeof(e=f.constructor)||e!==Array&&!a(e.prototype)?n(e)&&null===(e="
            "e[p])&&(e=void 0):e=void 0,e===Array||void 0===e))return d.call(f,y,g);for(o=new(void "
            "0===e?Array:e)(h(g-y,0)),l=0;y<g;y++,l++)y in f&&u(o,l,f[y]);return o.length=l,o}})},1035:(t,r,e)=>{\"use "
            "strict\";var "
            "o=e(6887),n=e(344),a=e(8219);o({target:\"String\",proto:!0,forced:!e(7772)(\"includes\")},{includes:"
            "function(t){return!!~String(a(this)).indexOf(n(t),arguments.length>1?arguments[1]:void "
            "0)}})},5824:(t,r,e)=>{\"use strict\";var "
            "o=e(6887),n=e(1899),a=e(626),i=e(2529),s=e(5746),c=e(2497),u=e(2302),l=e(5981),f=e(7457),p=e(1052),d=e("
            "941),h=e(6059),v=e(9678),y=e(4529),g=e(6935),b=e(1887),m=e(9290),x=e(4771),w=e(946),T=e(684),O=e(7857),S="
            "e(9677),j=e(5988),E=e(6760),P=e(2029),A=e(9754),k=e(8726),N=e(4262),q=e(7748),M=e(9418),V=e(9813),F=e("
            "1477),I=e(6349),_=e(904),z=e(5402),C=e(3610).forEach,D=N(\"hidden\"),L=\"Symbol\",W=V(\"toPrimitive\"),K="
            "z.set,J=z.getterFor(L),R=Object.prototype,B=n.Symbol,G=a(\"JSON\",\"stringify\"),Q=S.f,U=j.f,X=T.f,Y=E.f,"
            "H=k(\"symbols\"),Z=k(\"op-symbols\"),$=k(\"string-to-symbol-registry\"),tt=k(\"symbol-to-string-"
            "registry\"),rt=k(\"wks\"),et=n.QObject,ot=!et||!et.prototype||!et.prototype.findChild,nt=s&&l((function(){"
            "return 7!=m(U({},\"a\",{get:function(){return U(this,\"a\",{value:7}).a}})).a}))?function(t,r,e){var "
            "o=Q(R,r);o&&delete R[r],U(t,r,e),o&&t!==R&&U(R,r,o)}:U,at=function(t,r){var e=H[t]=m(B.prototype);return "
            "K(e,{type:L,tag:t,description:r}),s||(e.description=r),e},it=u?function(t){return\"symbol\"==typeof "
            "t}:function(t){return Object(t)instanceof B},st=function(t,r,e){t===R&&st(Z,r,e),h(t);var "
            "o=g(r,!0);return "
            "h(e),f(H,o)?(e.enumerable?(f(t,D)&&t[D][o]&&(t[D][o]=!1),e=m(e,{enumerable:b(0,!1)})):(f(t,D)||U(t,D,b(1,{"
            "})),t[D][o]=!0),nt(t,o,e)):U(t,o,e)},ct=function(t,r){h(t);var e=y(r),o=x(e).concat(pt(e));return "
            "C(o,(function(r){s&&!ut.call(e,r)||st(t,r,e[r])})),t},ut=function(t){var "
            "r=g(t,!0),e=Y.call(this,r);return!(this===R&&f(H,r)&&!f(Z,r))&&(!(e||!f(this,r)||!f(H,r)||f(this,D)&&this["
            "D][r])||e)},lt=function(t,r){var e=y(t),o=g(r,!0);if(e!==R||!f(H,o)||f(Z,o)){var "
            "n=Q(e,o);return!n||!f(H,o)||f(e,D)&&e[D][o]||(n.enumerable=!0),n}},ft=function(t){var "
            "r=X(y(t)),e=[];return C(r,(function(t){f(H,t)||f(q,t)||e.push(t)})),e},pt=function(t){var "
            "r=t===R,e=X(r?Z:y(t)),o=[];return "
            "C(e,(function(t){!f(H,t)||r&&!f(R,t)||o.push(H[t])})),o};c||(A((B=function(){if(this instanceof B)throw "
            "TypeError(\"Symbol is not a constructor\");var t=arguments.length&&void "
            "0!==arguments[0]?String(arguments[0]):void "
            "0,r=M(t),e=function(t){this===R&&e.call(Z,t),f(this,D)&&f(this[D],r)&&(this[D][r]=!1),nt(this,r,b(1,t))};"
            "return s&&ot&&nt(R,r,{configurable:!0,set:e}),at(r,t)}).prototype,\"toString\",(function(){return "
            "J(this).tag})),A(B,\"withoutSetter\",(function(t){return "
            "at(M(t),t)})),E.f=ut,j.f=st,S.f=lt,w.f=T.f=ft,O.f=pt,F.f=function(t){return "
            "at(V(t),t)},s&&(U(B.prototype,\"description\",{configurable:!0,get:function(){return "
            "J(this).description}}),i||A(R,\"propertyIsEnumerable\",ut,{unsafe:!0}))),o({global:!0,wrap:!0,forced:!c,"
            "sham:!c},{Symbol:B}),C(x(rt),(function(t){I(t)})),o({target:L,stat:!0,forced:!c},{for:function(t){var "
            "r=String(t);if(f($,r))return $[r];var e=B(r);return $[r]=e,tt[e]=r,e},keyFor:function(t){if(!it(t))throw "
            "TypeError(t+\" is not a symbol\");if(f(tt,t))return "
            "tt[t]},useSetter:function(){ot=!0},useSimple:function(){ot=!1}}),o({target:\"Object\",stat:!0,forced:!c,"
            "sham:!s},{create:function(t,r){return void "
            "0===r?m(t):ct(m(t),r)},defineProperty:st,defineProperties:ct,getOwnPropertyDescriptor:lt}),o({target:"
            "\"Object\",stat:!0,forced:!c},{getOwnPropertyNames:ft,getOwnPropertySymbols:pt}),o({target:\"Object\","
            "stat:!0,forced:l((function(){O.f(1)}))},{getOwnPropertySymbols:function(t){return "
            "O.f(v(t))}}),G&&o({target:\"JSON\",stat:!0,forced:!c||l((function(){var "
            "t=B();return\"[null]\"!=G([t])||\"{}\"!=G({a:t})||\"{}\"!=G(Object(t))}))},{stringify:function(t,r,e){for("
            "var o,n=[t],a=1;arguments.length>a;)n.push(arguments[a++]);if(o=r,(d(r)||void 0!==t)&&!it(t))return "
            "p(r)||(r=function(t,r){if(\"function\"==typeof o&&(r=o.call(this,t,r)),!it(r))return "
            "r}),n[1]=r,G.apply(null,n)}}),B.prototype[W]||P(B.prototype,W,B.prototype.valueOf),_(B,L),q[D]=!0},3778:("
            "t,r,e)=>{var o=e(8557);t.exports=o},2073:(t,r,e)=>{var o=e(9601);t.exports=o},9534:(t,r,e)=>{var "
            "o=e(498);t.exports=o}},r={};function e(o){var n=r[o];if(void 0!==n)return n.exports;var "
            "a=r[o]={exports:{}};return t[o](a,a.exports,e),a.exports}e.n=t=>{var "
            "r=t&&t.__esModule?()=>t.default:()=>t;return e.d(r,{a:r}),r},e.d=(t,r)=>{for(var o in "
            "r)e.o(r,o)&&!e.o(t,o)&&Object.defineProperty(t,o,{enumerable:!0,get:r[o]})},e.g=function(){if(\"object\"=="
            "typeof globalThis)return globalThis;try{return this||new Function(\"return "
            "this\")()}catch(t){if(\"object\"==typeof window)return "
            "window}}(),e.o=(t,r)=>Object.prototype.hasOwnProperty.call(t,r),(()=>{\"use strict\";var "
            "t,r=e(3778),o=e.n(r),n=e(2073),a=e.n(n),i=e(9534),s=e.n(i);globalThis.assert.strictEqual(globalThis.addon."
            "echo(\"hello\"),\"hello\"),globalThis.addon.readwriteValue=1,globalThis.assert.strictEqual(globalThis."
            "addon.readwriteValue,1),globalThis.addon.readwriteValue=2,globalThis.assert.strictEqual(globalThis.addon."
            "readwriteValue,2),globalThis.assert.ok(globalThis.addon.hiddenValue);var c=[];for(var u in "
            "globalThis.addon)c.push(u);globalThis.assert.ok(o()(c).call(c,\"echo\")),globalThis.assert.ok(o()(c).call("
            "c,\"readwriteValue\")),globalThis.assert.ok(o()(c).call(c,\"readonlyValue\")),globalThis.assert.ok(!o()(c)"
            ".call(c,\"hiddenValue\")),globalThis.assert.ok(o()(c).call(c,\"NameKeyValue\")),globalThis.assert.ok(!o()("
            "c).call(c,\"readwriteAccessor1\")),globalThis.assert.ok(!o()(c).call(c,\"readwriteAccessor2\")),"
            "globalThis.assert.ok(!o()(c).call(c,\"readonlyAccessor1\")),globalThis.assert.ok(!o()(c).call(c,"
            "\"readonlyAccessor2\"));var "
            "l=\"Symbol(\".length,f=l+\"NameKeySymbol\".length,p=a()(t=String(s()(globalThis.addon)[0])).call(t,l,f);"
            "globalThis.assert.strictEqual(p,\"NameKeySymbol\");var "
            "d=Object.getOwnPropertyDescriptor(globalThis.addon,\"readwriteAccessor1\"),h=Object."
            "getOwnPropertyDescriptor(globalThis.addon,\"readonlyAccessor1\");d?(globalThis.assert.ok(null!=d.get),"
            "globalThis.assert.ok(null!=d.set),globalThis.assert.ok(void "
            "0===d.value)):globalThis.assert(!1),h?(globalThis.assert.ok(null!=h.get),globalThis.assert.ok(void "
            "0===h.set),globalThis.assert.ok(void "
            "0===h.value)):globalThis.assert(!1),globalThis.addon.readwriteAccessor1=1,globalThis.assert.strictEqual("
            "globalThis.addon.readwriteAccessor1,1),globalThis.assert.strictEqual(globalThis.addon.readonlyAccessor1,1)"
            ",globalThis.addon.readwriteAccessor2=2,globalThis.assert.strictEqual(globalThis.addon.readwriteAccessor2,"
            "2),globalThis.assert.strictEqual(globalThis.addon.readonlyAccessor2,2),globalThis.assert.strictEqual("
            "globalThis.addon.hasNamedProperty(globalThis.addon,\"echo\"),!0),globalThis.assert.strictEqual(globalThis."
            "addon.hasNamedProperty(globalThis.addon,\"hiddenValue\"),!0),globalThis.assert.strictEqual(globalThis."
            "addon.hasNamedProperty(globalThis.addon,\"doesnotexist\"),!1)})()})();",
            "https://www.didi.com/test_properties.js", &result),
        NAPIOK);
}