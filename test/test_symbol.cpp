#include <common.h>
#include <js_native_api_test.h>

EXTERN_C_START

static NAPIValue New(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPIValue description;
    if (argc >= 1)
    {
        NAPIValueType valuetype;
        NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));

        NAPI_ASSERT(env, valuetype == NAPIString, "Wrong type of arguments. Expects a string.");

        description = args[0];
    }
    else
    {
        NAPI_CALL(env, napi_get_undefined(env, &description));
    }

    NAPIValue symbol;
    NAPI_CALL(env, napi_create_symbol(env, description, &symbol));

    return symbol;
}

EXTERN_C_END

TEST_F(Test, TestSymbol)
{
    NAPIPropertyDescriptor properties[] = {
        DECLARE_NAPI_PROPERTY("New", New),
    };

    ASSERT_EQ(napi_define_properties(globalEnv, exports, sizeof(properties) / sizeof(*properties), properties), NAPIOK);

    NAPIValue result;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(
                  globalEnv,
                  "(()=>{\"use strict\";var "
                  "a=globalThis.addon.New(\"test\");globalThis.assert.strictEqual(a.toString(),\"Symbol(test)\");var "
                  "s={},t=globalThis.addon.New(\"foo\"),l=globalThis.addon.New(\"bar\");s.foo=\"bar\",s[t]=\"baz\",s[l]"
                  "=\"bing\",globalThis.assert.strictEqual(s.foo,\"bar\"),globalThis.assert.strictEqual(s[t],\"baz\"),"
                  "globalThis.assert.strictEqual(s[l],\"bing\")})();",
                  "https://www.napi.com/test_symbol_test1.js", &result),
              NAPIOK);

    ASSERT_EQ(
        NAPIRunScriptWithSourceUrl(
            globalEnv,
            "(()=>{var t={498:(t,r,e)=>{e(5824);var "
            "n=e(4058);t.exports=n.Object.getOwnPropertySymbols},3916:t=>{t.exports=function(t){if(\"function\"!="
            "typeof t)throw TypeError(String(t)+\" is not a function\");return t}},6059:(t,r,e)=>{var "
            "n=e(941);t.exports=function(t){if(!n(t))throw TypeError(String(t)+\" is not an object\");return "
            "t}},1692:(t,r,e)=>{var n=e(4529),o=e(3057),i=e(9413),u=function(t){return function(r,e,u){var "
            "c,a=n(r),f=o(a.length),s=i(u,f);if(t&&e!=e){for(;f>s;)if((c=a[s++])!=c)return!0}else "
            "for(;f>s;s++)if((t||s in a)&&a[s]===e)return "
            "t||s||0;return!t&&-1}};t.exports={includes:u(!0),indexOf:u(!1)}},3610:(t,r,e)=>{var "
            "n=e(6843),o=e(7026),i=e(9678),u=e(3057),c=e(4692),a=[].push,f=function(t){var "
            "r=1==t,e=2==t,f=3==t,s=4==t,p=6==t,l=7==t,v=5==t||p;return function(y,h,b,g){for(var "
            "d,m,w=i(y),x=o(w),O=n(h,b,3),j=u(x.length),S=0,P=g||c,E=r?P(y,j):e||l?P(y,0):void 0;j>S;S++)if((v||S in "
            "x)&&(m=O(d=x[S],S,w),t))if(r)E[S]=m;else if(m)switch(t){case 3:return!0;case 5:return d;case 6:return "
            "S;case 2:a.call(E,d)}else switch(t){case 4:return!1;case 7:a.call(E,d)}return "
            "p?-1:f||s?s:E}};t.exports={forEach:f(0),map:f(1),filter:f(2),some:f(3),every:f(4),find:f(5),findIndex:f(6)"
            ",filterOut:f(7)}},4692:(t,r,e)=>{var "
            "n=e(941),o=e(1052),i=e(9813)(\"species\");t.exports=function(t,r){var e;return "
            "o(t)&&(\"function\"!=typeof(e=t.constructor)||e!==Array&&!o(e.prototype)?n(e)&&null===(e=e[i])&&(e=void "
            "0):e=void 0),new(void 0===e?Array:e)(0===r?0:r)}},2532:t=>{var r={}.toString;t.exports=function(t){return "
            "r.call(t).slice(8,-1)}},9697:(t,r,e)=>{var "
            "n=e(2885),o=e(2532),i=e(9813)(\"toStringTag\"),u=\"Arguments\"==o(function(){return "
            "arguments}());t.exports=n?o:function(t){var r,e,n;return void "
            "0===t?\"Undefined\":null===t?\"Null\":\"string\"==typeof(e=function(t,r){try{return "
            "t[r]}catch(t){}}(r=Object(t),i))?e:u?o(r):\"Object\"==(n=o(r))&&\"function\"==typeof "
            "r.callee?\"Arguments\":n}},2029:(t,r,e)=>{var "
            "n=e(5746),o=e(5988),i=e(1887);t.exports=n?function(t,r,e){return o.f(t,r,i(1,e))}:function(t,r,e){return "
            "t[r]=e,t}},1887:t=>{t.exports=function(t,r){return{enumerable:!(1&t),configurable:!(2&t),writable:!(4&t),"
            "value:r}}},6349:(t,r,e)=>{var n=e(4058),o=e(7457),i=e(1477),u=e(5988).f;t.exports=function(t){var "
            "r=n.Symbol||(n.Symbol={});o(r,t)||u(r,t,{value:i.f(t)})}},5746:(t,r,e)=>{var "
            "n=e(5981);t.exports=!n((function(){return 7!=Object.defineProperty({},1,{get:function(){return "
            "7}})[1]}))},1333:(t,r,e)=>{var "
            "n=e(1899),o=e(941),i=n.document,u=o(i)&&o(i.createElement);t.exports=function(t){return "
            "u?i.createElement(t):{}}},6049:(t,r,e)=>{var "
            "n=e(2532),o=e(1899);t.exports=\"process\"==n(o.process)},2861:(t,r,e)=>{var "
            "n=e(626);t.exports=n(\"navigator\",\"userAgent\")||\"\"},3385:(t,r,e)=>{var "
            "n,o,i=e(1899),u=e(2861),c=i.process,a=c&&c.versions,f=a&&a.v8;f?o=(n=f.split(\".\"))[0]+n[1]:u&&(!(n=u."
            "match(/Edge\\/(\\d+)/))||n[1]>=74)&&(n=u.match(/Chrome\\/(\\d+)/"
            "))&&(o=n[1]),t.exports=o&&+o},6759:t=>{t.exports=[\"constructor\",\"hasOwnProperty\",\"isPrototypeOf\","
            "\"propertyIsEnumerable\",\"toLocaleString\",\"toString\",\"valueOf\"]},6887:(t,r,e)=>{\"use strict\";var "
            "n=e(1899),o=e(9677).f,i=e(7252),u=e(4058),c=e(6843),a=e(2029),f=e(7457),s=function(t){var "
            "r=function(r,e,n){if(this instanceof t){switch(arguments.length){case 0:return new t;case 1:return new "
            "t(r);case 2:return new t(r,e)}return new t(r,e,n)}return t.apply(this,arguments)};return "
            "r.prototype=t.prototype,r};t.exports=function(t,r){var "
            "e,p,l,v,y,h,b,g,d=t.target,m=t.global,w=t.stat,x=t.proto,O=m?n:w?n[d]:(n[d]||{}).prototype,j=m?u:u[d]||(u["
            "d]={}),S=j.prototype;for(l in "
            "r)e=!i(m?l:d+(w?\".\":\"#\")+l,t.forced)&&O&&f(O,l),y=j[l],e&&(h=t.noTargetGet?(g=o(O,l))&&g.value:O[l]),"
            "v=e&&h?h:r[l],e&&typeof y==typeof v||(b=t.bind&&e?c(v,n):t.wrap&&e?s(v):x&&\"function\"==typeof "
            "v?c(Function.call,v):v,(t.sham||v&&v.sham||y&&y.sham)&&a(b,\"sham\",!0),j[l]=b,x&&(f(u,p=d+\"Prototype\")|"
            "|a(u,p,{}),u[p][l]=v,t.real&&S&&!S[l]&&a(S,l,v)))}},5981:t=>{t.exports=function(t){try{return!!t()}catch("
            "t){return!0}}},6843:(t,r,e)=>{var n=e(3916);t.exports=function(t,r,e){if(n(t),void 0===r)return "
            "t;switch(e){case 0:return function(){return t.call(r)};case 1:return function(e){return t.call(r,e)};case "
            "2:return function(e,n){return t.call(r,e,n)};case 3:return function(e,n,o){return t.call(r,e,n,o)}}return "
            "function(){return t.apply(r,arguments)}}},626:(t,r,e)=>{var "
            "n=e(4058),o=e(1899),i=function(t){return\"function\"==typeof t?t:void 0};t.exports=function(t,r){return "
            "arguments.length<2?i(n[t])||i(o[t]):n[t]&&n[t][r]||o[t]&&o[t][r]}},1899:(t,r,e)=>{var "
            "n=function(t){return t&&t.Math==Math&&t};t.exports=n(\"object\"==typeof "
            "globalThis&&globalThis)||n(\"object\"==typeof window&&window)||n(\"object\"==typeof "
            "self&&self)||n(\"object\"==typeof e.g&&e.g)||function(){return this}()||Function(\"return "
            "this\")()},7457:(t,r,e)=>{var n=e(9678),o={}.hasOwnProperty;t.exports=function(t,r){return "
            "o.call(n(t),r)}},7748:t=>{t.exports={}},5463:(t,r,e)=>{var "
            "n=e(626);t.exports=n(\"document\",\"documentElement\")},2840:(t,r,e)=>{var "
            "n=e(5746),o=e(5981),i=e(1333);t.exports=!n&&!o((function(){return "
            "7!=Object.defineProperty(i(\"div\"),\"a\",{get:function(){return 7}}).a}))},7026:(t,r,e)=>{var "
            "n=e(5981),o=e(2532),i=\"\".split;t.exports=n((function(){return!Object(\"z\").propertyIsEnumerable(0)}))?"
            "function(t){return\"String\"==o(t)?i.call(t,\"\"):Object(t)}:Object},1302:(t,r,e)=>{var "
            "n=e(3030),o=Function.toString;\"function\"!=typeof n.inspectSource&&(n.inspectSource=function(t){return "
            "o.call(t)}),t.exports=n.inspectSource},5402:(t,r,e)=>{var "
            "n,o,i,u=e(8019),c=e(1899),a=e(941),f=e(2029),s=e(7457),p=e(3030),l=e(4262),v=e(7748),y=\"Object already "
            "initialized\",h=c.WeakMap;if(u){var b=p.state||(p.state=new "
            "h),g=b.get,d=b.has,m=b.set;n=function(t,r){if(d.call(b,t))throw new TypeError(y);return "
            "r.facade=t,m.call(b,t,r),r},o=function(t){return g.call(b,t)||{}},i=function(t){return "
            "d.call(b,t)}}else{var w=l(\"state\");v[w]=!0,n=function(t,r){if(s(t,w))throw new TypeError(y);return "
            "r.facade=t,f(t,w,r),r},o=function(t){return s(t,w)?t[w]:{}},i=function(t){return "
            "s(t,w)}}t.exports={set:n,get:o,has:i,enforce:function(t){return "
            "i(t)?o(t):n(t,{})},getterFor:function(t){return function(r){var e;if(!a(r)||(e=o(r)).type!==t)throw "
            "TypeError(\"Incompatible receiver, \"+t+\" required\");return e}}}},1052:(t,r,e)=>{var "
            "n=e(2532);t.exports=Array.isArray||function(t){return\"Array\"==n(t)}},7252:(t,r,e)=>{var "
            "n=e(5981),o=/#|\\.prototype\\./,i=function(t,r){var e=c[u(t)];return e==f||e!=a&&(\"function\"==typeof "
            "r?n(r):!!r)},u=i.normalize=function(t){return "
            "String(t).replace(o,\".\").toLowerCase()},c=i.data={},a=i.NATIVE=\"N\",f=i.POLYFILL=\"P\";t.exports=i},"
            "941:t=>{t.exports=function(t){return\"object\"==typeof t?null!==t:\"function\"==typeof "
            "t}},2529:t=>{t.exports=!0},2497:(t,r,e)=>{var "
            "n=e(6049),o=e(3385),i=e(5981);t.exports=!!Object.getOwnPropertySymbols&&!i((function(){return!Symbol.sham&"
            "&(n?38===o:o>37&&o<41)}))},8019:(t,r,e)=>{var "
            "n=e(1899),o=e(1302),i=n.WeakMap;t.exports=\"function\"==typeof i&&/native "
            "code/.test(o(i))},9290:(t,r,e)=>{var "
            "n,o=e(6059),i=e(9938),u=e(6759),c=e(7748),a=e(5463),f=e(1333),s=e(4262)(\"IE_PROTO\"),p=function(){},l="
            "function(t){return\"<script>\"+t+\"<\\/script>\"},v=function(){try{n=document.domain&&new "
            "ActiveXObject(\"htmlfile\")}catch(t){}var t,r;v=n?function(t){t.write(l(\"\")),t.close();var "
            "r=t.parentWindow.Object;return "
            "t=null,r}(n):((r=f(\"iframe\")).style.display=\"none\",a.appendChild(r),r.src=String(\"javascript:\"),(t="
            "r.contentWindow.document).open(),t.write(l(\"document.F=Object\")),t.close(),t.F);for(var "
            "e=u.length;e--;)delete v.prototype[u[e]];return v()};c[s]=!0,t.exports=Object.create||function(t,r){var "
            "e;return null!==t?(p.prototype=o(t),e=new p,p.prototype=null,e[s]=t):e=v(),void "
            "0===r?e:i(e,r)}},9938:(t,r,e)=>{var "
            "n=e(5746),o=e(5988),i=e(6059),u=e(4771);t.exports=n?Object.defineProperties:function(t,r){i(t);for(var "
            "e,n=u(r),c=n.length,a=0;c>a;)o.f(t,e=n[a++],r[e]);return t}},5988:(t,r,e)=>{var "
            "n=e(5746),o=e(2840),i=e(6059),u=e(6935),c=Object.defineProperty;r.f=n?c:function(t,r,e){if(i(t),r=u(r,!0),"
            "i(e),o)try{return c(t,r,e)}catch(t){}if(\"get\"in e||\"set\"in e)throw TypeError(\"Accessors not "
            "supported\");return\"value\"in e&&(t[r]=e.value),t}},9677:(t,r,e)=>{var "
            "n=e(5746),o=e(6760),i=e(1887),u=e(4529),c=e(6935),a=e(7457),f=e(2840),s=Object.getOwnPropertyDescriptor;r."
            "f=n?s:function(t,r){if(t=u(t),r=c(r,!0),f)try{return s(t,r)}catch(t){}if(a(t,r))return "
            "i(!o.f.call(t,r),t[r])}},684:(t,r,e)=>{var n=e(4529),o=e(946).f,i={}.toString,u=\"object\"==typeof "
            "window&&window&&Object.getOwnPropertyNames?Object.getOwnPropertyNames(window):[];t.exports.f=function(t){"
            "return u&&\"[object Window]\"==i.call(t)?function(t){try{return o(t)}catch(t){return "
            "u.slice()}}(t):o(n(t))}},946:(t,r,e)=>{var "
            "n=e(5629),o=e(6759).concat(\"length\",\"prototype\");r.f=Object.getOwnPropertyNames||function(t){return "
            "n(t,o)}},7857:(t,r)=>{r.f=Object.getOwnPropertySymbols},5629:(t,r,e)=>{var "
            "n=e(7457),o=e(4529),i=e(1692).indexOf,u=e(7748);t.exports=function(t,r){var e,c=o(t),a=0,f=[];for(e in "
            "c)!n(u,e)&&n(c,e)&&f.push(e);for(;r.length>a;)n(c,e=r[a++])&&(~i(f,e)||f.push(e));return "
            "f}},4771:(t,r,e)=>{var n=e(5629),o=e(6759);t.exports=Object.keys||function(t){return "
            "n(t,o)}},6760:(t,r)=>{\"use strict\";var "
            "e={}.propertyIsEnumerable,n=Object.getOwnPropertyDescriptor,o=n&&!e.call({1:2},1);r.f=o?function(t){var "
            "r=n(this,t);return!!r&&r.enumerable}:e},5623:(t,r,e)=>{\"use strict\";var "
            "n=e(2885),o=e(9697);t.exports=n?{}.toString:function(){return\"[object "
            "\"+o(this)+\"]\"}},4058:t=>{t.exports={}},9754:(t,r,e)=>{var "
            "n=e(2029);t.exports=function(t,r,e,o){o&&o.enumerable?t[r]=e:n(t,r,e)}},8219:t=>{t.exports=function(t){if("
            "null==t)throw TypeError(\"Can't call method on \"+t);return t}},4911:(t,r,e)=>{var "
            "n=e(1899),o=e(2029);t.exports=function(t,r){try{o(n,t,r)}catch(e){n[t]=r}return r}},904:(t,r,e)=>{var "
            "n=e(2885),o=e(5988).f,i=e(2029),u=e(7457),c=e(5623),a=e(9813)(\"toStringTag\");t.exports=function(t,r,e,f)"
            "{if(t){var "
            "s=e?t:t.prototype;u(s,a)||o(s,a,{configurable:!0,value:r}),f&&!n&&i(s,\"toString\",c)}}},4262:(t,r,e)=>{"
            "var n=e(8726),o=e(9418),i=n(\"keys\");t.exports=function(t){return i[t]||(i[t]=o(t))}},3030:(t,r,e)=>{var "
            "n=e(1899),o=e(4911),i=\"__core-js_shared__\",u=n[i]||o(i,{});t.exports=u},8726:(t,r,e)=>{var "
            "n=e(2529),o=e(3030);(t.exports=function(t,r){return o[t]||(o[t]=void "
            "0!==r?r:{})})(\"versions\",[]).push({version:\"3.11.1\",mode:n?\"pure\":\"global\",copyright:\"© 2021 "
            "Denis Pushkarev (zloirock.ru)\"})},9413:(t,r,e)=>{var "
            "n=e(8459),o=Math.max,i=Math.min;t.exports=function(t,r){var e=n(t);return "
            "e<0?o(e+r,0):i(e,r)}},4529:(t,r,e)=>{var n=e(7026),o=e(8219);t.exports=function(t){return "
            "n(o(t))}},8459:t=>{var r=Math.ceil,e=Math.floor;t.exports=function(t){return "
            "isNaN(t=+t)?0:(t>0?e:r)(t)}},3057:(t,r,e)=>{var n=e(8459),o=Math.min;t.exports=function(t){return "
            "t>0?o(n(t),9007199254740991):0}},9678:(t,r,e)=>{var n=e(8219);t.exports=function(t){return "
            "Object(n(t))}},6935:(t,r,e)=>{var n=e(941);t.exports=function(t,r){if(!n(t))return t;var "
            "e,o;if(r&&\"function\"==typeof(e=t.toString)&&!n(o=e.call(t)))return "
            "o;if(\"function\"==typeof(e=t.valueOf)&&!n(o=e.call(t)))return "
            "o;if(!r&&\"function\"==typeof(e=t.toString)&&!n(o=e.call(t)))return o;throw TypeError(\"Can't convert "
            "object to primitive value\")}},2885:(t,r,e)=>{var "
            "n={};n[e(9813)(\"toStringTag\")]=\"z\",t.exports=\"[object z]\"===String(n)},9418:t=>{var "
            "r=0,e=Math.random();t.exports=function(t){return\"Symbol(\"+String(void "
            "0===t?\"\":t)+\")_\"+(++r+e).toString(36)}},2302:(t,r,e)=>{var "
            "n=e(2497);t.exports=n&&!Symbol.sham&&\"symbol\"==typeof Symbol.iterator},1477:(t,r,e)=>{var "
            "n=e(9813);r.f=n},9813:(t,r,e)=>{var "
            "n=e(1899),o=e(8726),i=e(7457),u=e(9418),c=e(2497),a=e(2302),f=o(\"wks\"),s=n.Symbol,p=a?s:s&&s."
            "withoutSetter||u;t.exports=function(t){return i(f,t)&&(c||\"string\"==typeof "
            "f[t])||(c&&i(s,t)?f[t]=s[t]:f[t]=p(\"Symbol.\"+t)),f[t]}},5824:(t,r,e)=>{\"use strict\";var "
            "n=e(6887),o=e(1899),i=e(626),u=e(2529),c=e(5746),a=e(2497),f=e(2302),s=e(5981),p=e(7457),l=e(1052),v=e("
            "941),y=e(6059),h=e(9678),b=e(4529),g=e(6935),d=e(1887),m=e(9290),w=e(4771),x=e(946),O=e(684),j=e(7857),S="
            "e(9677),P=e(5988),E=e(6760),T=e(2029),N=e(9754),A=e(8726),M=e(4262),k=e(7748),F=e(9418),I=e(9813),_=e("
            "1477),z=e(6349),C=e(904),L=e(5402),W=e(3610).forEach,D=M(\"hidden\"),q=\"Symbol\",J=I(\"toPrimitive\"),G="
            "L.set,Q=L.getterFor(q),R=Object.prototype,U=o.Symbol,V=i(\"JSON\",\"stringify\"),X=S.f,Y=P.f,B=O.f,H=E.f,"
            "K=A(\"symbols\"),Z=A(\"op-symbols\"),$=A(\"string-to-symbol-registry\"),tt=A(\"symbol-to-string-"
            "registry\"),rt=A(\"wks\"),et=o.QObject,nt=!et||!et.prototype||!et.prototype.findChild,ot=c&&s((function(){"
            "return 7!=m(Y({},\"a\",{get:function(){return Y(this,\"a\",{value:7}).a}})).a}))?function(t,r,e){var "
            "n=X(R,r);n&&delete R[r],Y(t,r,e),n&&t!==R&&Y(R,r,n)}:Y,it=function(t,r){var e=K[t]=m(U.prototype);return "
            "G(e,{type:q,tag:t,description:r}),c||(e.description=r),e},ut=f?function(t){return\"symbol\"==typeof "
            "t}:function(t){return Object(t)instanceof U},ct=function(t,r,e){t===R&&ct(Z,r,e),y(t);var "
            "n=g(r,!0);return "
            "y(e),p(K,n)?(e.enumerable?(p(t,D)&&t[D][n]&&(t[D][n]=!1),e=m(e,{enumerable:d(0,!1)})):(p(t,D)||Y(t,D,d(1,{"
            "})),t[D][n]=!0),ot(t,n,e)):Y(t,n,e)},at=function(t,r){y(t);var e=b(r),n=w(e).concat(lt(e));return "
            "W(n,(function(r){c&&!ft.call(e,r)||ct(t,r,e[r])})),t},ft=function(t){var "
            "r=g(t,!0),e=H.call(this,r);return!(this===R&&p(K,r)&&!p(Z,r))&&(!(e||!p(this,r)||!p(K,r)||p(this,D)&&this["
            "D][r])||e)},st=function(t,r){var e=b(t),n=g(r,!0);if(e!==R||!p(K,n)||p(Z,n)){var "
            "o=X(e,n);return!o||!p(K,n)||p(e,D)&&e[D][n]||(o.enumerable=!0),o}},pt=function(t){var "
            "r=B(b(t)),e=[];return W(r,(function(t){p(K,t)||p(k,t)||e.push(t)})),e},lt=function(t){var "
            "r=t===R,e=B(r?Z:b(t)),n=[];return "
            "W(e,(function(t){!p(K,t)||r&&!p(R,t)||n.push(K[t])})),n};a||(N((U=function(){if(this instanceof U)throw "
            "TypeError(\"Symbol is not a constructor\");var t=arguments.length&&void "
            "0!==arguments[0]?String(arguments[0]):void "
            "0,r=F(t),e=function(t){this===R&&e.call(Z,t),p(this,D)&&p(this[D],r)&&(this[D][r]=!1),ot(this,r,d(1,t))};"
            "return c&&nt&&ot(R,r,{configurable:!0,set:e}),it(r,t)}).prototype,\"toString\",(function(){return "
            "Q(this).tag})),N(U,\"withoutSetter\",(function(t){return "
            "it(F(t),t)})),E.f=ft,P.f=ct,S.f=st,x.f=O.f=pt,j.f=lt,_.f=function(t){return "
            "it(I(t),t)},c&&(Y(U.prototype,\"description\",{configurable:!0,get:function(){return "
            "Q(this).description}}),u||N(R,\"propertyIsEnumerable\",ft,{unsafe:!0}))),n({global:!0,wrap:!0,forced:!a,"
            "sham:!a},{Symbol:U}),W(w(rt),(function(t){z(t)})),n({target:q,stat:!0,forced:!a},{for:function(t){var "
            "r=String(t);if(p($,r))return $[r];var e=U(r);return $[r]=e,tt[e]=r,e},keyFor:function(t){if(!ut(t))throw "
            "TypeError(t+\" is not a symbol\");if(p(tt,t))return "
            "tt[t]},useSetter:function(){nt=!0},useSimple:function(){nt=!1}}),n({target:\"Object\",stat:!0,forced:!a,"
            "sham:!c},{create:function(t,r){return void "
            "0===r?m(t):at(m(t),r)},defineProperty:ct,defineProperties:at,getOwnPropertyDescriptor:st}),n({target:"
            "\"Object\",stat:!0,forced:!a},{getOwnPropertyNames:pt,getOwnPropertySymbols:lt}),n({target:\"Object\","
            "stat:!0,forced:s((function(){j.f(1)}))},{getOwnPropertySymbols:function(t){return "
            "j.f(h(t))}}),V&&n({target:\"JSON\",stat:!0,forced:!a||s((function(){var "
            "t=U();return\"[null]\"!=V([t])||\"{}\"!=V({a:t})||\"{}\"!=V(Object(t))}))},{stringify:function(t,r,e){for("
            "var n,o=[t],i=1;arguments.length>i;)o.push(arguments[i++]);if(n=r,(v(r)||void 0!==t)&&!ut(t))return "
            "l(r)||(r=function(t,r){if(\"function\"==typeof n&&(r=n.call(this,t,r)),!ut(r))return "
            "r}),o[1]=r,V.apply(null,o)}}),U.prototype[J]||T(U.prototype,J,U.prototype.valueOf),C(U,q),k[D]=!0},9534:("
            "t,r,e)=>{var n=e(498);t.exports=n}},r={};function e(n){var o=r[n];if(void 0!==o)return o.exports;var "
            "i=r[n]={exports:{}};return t[n](i,i.exports,e),i.exports}e.n=t=>{var "
            "r=t&&t.__esModule?()=>t.default:()=>t;return e.d(r,{a:r}),r},e.d=(t,r)=>{for(var n in "
            "r)e.o(r,n)&&!e.o(t,n)&&Object.defineProperty(t,n,{enumerable:!0,get:r[n]})},e.g=function(){if(\"object\"=="
            "typeof globalThis)return globalThis;try{return this||new Function(\"return "
            "this\")()}catch(t){if(\"object\"==typeof window)return "
            "window}}(),e.o=(t,r)=>Object.prototype.hasOwnProperty.call(t,r),(()=>{\"use strict\";var "
            "t=e(9534),r=e.n(t),n=globalThis.addon.New(\"foo\"),o={foo:\"bar\"};o[n]=\"baz\",Object.keys(o),Object."
            "getOwnPropertyNames(o),r()(o),globalThis.assert.strictEqual(r()(o)[0],n)})()})();",
            "https://www.napi.com/test_symbol_test2.js", &result),
        NAPIOK);

    ASSERT_EQ(NAPIRunScriptWithSourceUrl(
                  globalEnv,
                  "(()=>{\"use "
                  "strict\";globalThis.assert.notStrictEqual(globalThis.addon.New(),globalThis.addon.New()),globalThis."
                  "assert.notStrictEqual(globalThis.addon.New(\"foo\"),globalThis.addon.New(\"foo\")),globalThis."
                  "assert.notStrictEqual(globalThis.addon.New(\"foo\"),globalThis.addon.New(\"bar\"));var "
                  "o=globalThis.addon.New(\"foo\"),a=globalThis.addon.New(\"foo\"),l={[o]:1,[a]:2};globalThis.assert."
                  "strictEqual(l[o],1),globalThis.assert.strictEqual(l[a],2)})();",
                  "https://www.napi.com/test_symbol_test3.js", &result),
              NAPIOK);
}