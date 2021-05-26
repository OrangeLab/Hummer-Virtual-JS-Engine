#include <common.h>
#include <js_native_api_test.h>

EXTERN_C_START

static NAPIDeferred deferred = nullptr;

static NAPIValue createPromise(NAPIEnv env, NAPICallbackInfo /*info*/)
{
    NAPIValue promise;

    // We do not overwrite an existing deferred.
    if (deferred != nullptr)
    {
        return nullptr;
    }

    NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));

    return promise;
}

static NAPIValue concludeCurrentPromise(NAPIEnv env, NAPICallbackInfo info)
{
    NAPIValue argv[2];
    size_t argc = 2;
    bool resolution;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_CALL(env, napi_get_value_bool(env, argv[1], &resolution));
    if (resolution)
    {
        NAPI_CALL(env, napi_resolve_deferred(env, deferred, argv[0]));
    }
    else
    {
        NAPI_CALL(env, napi_reject_deferred(env, deferred, argv[0]));
    }

    deferred = nullptr;

    return nullptr;
}

static NAPIValue isPromise(NAPIEnv env, NAPICallbackInfo info)
{
    NAPIValue promise, result;
    size_t argc = 1;
    bool is_promise;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &promise, nullptr, nullptr));
    NAPI_CALL(env, napi_is_promise(env, promise, &is_promise));
    NAPI_CALL(env, napi_get_boolean(env, is_promise, &result));

    return result;
}

EXTERN_C_END

TEST_F(Test, TestPromise)
{
    NAPIPropertyDescriptor descriptors[] = {
        DECLARE_NAPI_PROPERTY("createPromise", createPromise),
        DECLARE_NAPI_PROPERTY("concludeCurrentPromise", concludeCurrentPromise),
        DECLARE_NAPI_PROPERTY("isPromise", isPromise),
    };

    ASSERT_EQ(napi_define_properties(globalEnv, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors),
              NAPIOK);

    NAPIValue result;
    ASSERT_EQ(
        NAPIRunScriptWithSourceUrl(
            globalEnv,
            "(()=>{var t={2956:(t,r,e)=>{e(7627),e(5967),e(8881),e(4560),e(7206),e(4349),e(7971),e(7634);var "
            "n=e(4058);t.exports=n.Promise},3916:t=>{t.exports=function(t){if(\"function\"!=typeof t)throw "
            "TypeError(String(t)+\" is not a function\");return t}},1851:(t,r,e)=>{var "
            "n=e(941);t.exports=function(t){if(!n(t)&&null!==t)throw TypeError(\"Can't set \"+String(t)+\" as a "
            "prototype\");return t}},8479:t=>{t.exports=function(){}},5743:t=>{t.exports=function(t,r,e){if(!(t "
            "instanceof r))throw TypeError(\"Incorrect \"+(e?e+\" \":\"\")+\"invocation\");return "
            "t}},6059:(t,r,e)=>{var n=e(941);t.exports=function(t){if(!n(t))throw TypeError(String(t)+\" is not an "
            "object\");return t}},1692:(t,r,e)=>{var n=e(4529),o=e(3057),i=e(9413),a=function(t){return "
            "function(r,e,a){var c,u=n(r),s=o(u.length),f=i(a,s);if(t&&e!=e){for(;s>f;)if((c=u[f++])!=c)return!0}else "
            "for(;s>f;f++)if((t||f in u)&&u[f]===e)return "
            "t||f||0;return!t&&-1}};t.exports={includes:a(!0),indexOf:a(!1)}},1385:(t,r,e)=>{var "
            "n=e(9813)(\"iterator\"),o=!1;try{var "
            "i=0,a={next:function(){return{done:!!i++}},return:function(){o=!0}};a[n]=function(){return "
            "this},Array.from(a,(function(){throw 2}))}catch(t){}t.exports=function(t,r){if(!r&&!o)return!1;var "
            "e=!1;try{var i={};i[n]=function(){return{next:function(){return{done:e=!0}}}},t(i)}catch(t){}return "
            "e}},2532:t=>{var r={}.toString;t.exports=function(t){return r.call(t).slice(8,-1)}},9697:(t,r,e)=>{var "
            "n=e(2885),o=e(2532),i=e(9813)(\"toStringTag\"),a=\"Arguments\"==o(function(){return "
            "arguments}());t.exports=n?o:function(t){var r,e,n;return void "
            "0===t?\"Undefined\":null===t?\"Null\":\"string\"==typeof(e=function(t,r){try{return "
            "t[r]}catch(t){}}(r=Object(t),i))?e:a?o(r):\"Object\"==(n=o(r))&&\"function\"==typeof "
            "r.callee?\"Arguments\":n}},4160:(t,r,e)=>{var n=e(5981);t.exports=!n((function(){function t(){}return "
            "t.prototype.constructor=null,Object.getPrototypeOf(new t)!==t.prototype}))},1046:(t,r,e)=>{\"use "
            "strict\";var n=e(5143).IteratorPrototype,o=e(9290),i=e(1887),a=e(904),c=e(2077),u=function(){return "
            "this};t.exports=function(t,r,e){var s=r+\" Iterator\";return "
            "t.prototype=o(n,{next:i(1,e)}),a(t,s,!1,!0),c[s]=u,t}},2029:(t,r,e)=>{var "
            "n=e(5746),o=e(5988),i=e(1887);t.exports=n?function(t,r,e){return o.f(t,r,i(1,e))}:function(t,r,e){return "
            "t[r]=e,t}},1887:t=>{t.exports=function(t,r){return{enumerable:!(1&t),configurable:!(2&t),writable:!(4&t),"
            "value:r}}},7771:(t,r,e)=>{\"use strict\";var "
            "n=e(6887),o=e(1046),i=e(249),a=e(8929),c=e(904),u=e(2029),s=e(9754),f=e(9813),l=e(2529),p=e(2077),v=e("
            "5143),h=v.IteratorPrototype,d=v.BUGGY_SAFARI_ITERATORS,y=f(\"iterator\"),g=\"keys\",m=\"values\",x="
            "\"entries\",b=function(){return this};t.exports=function(t,r,e,f,v,w,j){o(e,r,f);var "
            "S,T,O,E=function(t){if(t===v&&M)return M;if(!d&&t in L)return L[t];switch(t){case g:case m:case x:return "
            "function(){return new e(this,t)}}return function(){return new e(this)}},P=r+\" "
            "Iterator\",A=!1,L=t.prototype,I=L[y]||L[\"@@iterator\"]||v&&L[v],M=!d&&I||E(v),_=\"Array\"==r&&L.entries||"
            "I;if(_&&(S=i(_.call(new t)),h!==Object.prototype&&S.next&&(l||i(S)===h||(a?a(S,h):\"function\"!=typeof "
            "S[y]&&u(S,y,b)),c(S,P,!0,!0),l&&(p[P]=b))),v==m&&I&&I.name!==m&&(A=!0,M=function(){return "
            "I.call(this)}),l&&!j||L[y]===M||u(L,y,M),p[r]=M,v)if(T={values:E(m),keys:w?M:E(g),entries:E(x)},j)for(O "
            "in T)(d||A||!(O in L))&&s(L,O,T[O]);else n({target:r,proto:!0,forced:d||A},T);return "
            "T}},5746:(t,r,e)=>{var n=e(5981);t.exports=!n((function(){return "
            "7!=Object.defineProperty({},1,{get:function(){return 7}})[1]}))},1333:(t,r,e)=>{var "
            "n=e(1899),o=e(941),i=n.document,a=o(i)&&o(i.createElement);t.exports=function(t){return "
            "a?i.createElement(t):{}}},3281:t=>{t.exports={CSSRuleList:0,CSSStyleDeclaration:0,CSSValueList:0,"
            "ClientRectList:0,DOMRectList:0,DOMStringList:0,DOMTokenList:1,DataTransferItemList:0,FileList:0,"
            "HTMLAllCollection:0,HTMLCollection:0,HTMLFormElement:0,HTMLSelectElement:0,MediaList:0,MimeTypeArray:0,"
            "NamedNodeMap:0,NodeList:1,PaintRequestList:0,Plugin:0,PluginArray:0,SVGLengthList:0,SVGNumberList:0,"
            "SVGPathSegList:0,SVGPointList:0,SVGStringList:0,SVGTransformList:0,SourceBufferList:0,StyleSheetList:0,"
            "TextTrackCueList:0,TextTrackList:0,TouchList:0}},2749:(t,r,e)=>{var "
            "n=e(2861);t.exports=/(?:iphone|ipod|ipad).*applewebkit/i.test(n)},6049:(t,r,e)=>{var "
            "n=e(2532),o=e(1899);t.exports=\"process\"==n(o.process)},8045:(t,r,e)=>{var "
            "n=e(2861);t.exports=/web0s(?!.*chrome)/i.test(n)},2861:(t,r,e)=>{var "
            "n=e(626);t.exports=n(\"navigator\",\"userAgent\")||\"\"},3385:(t,r,e)=>{var "
            "n,o,i=e(1899),a=e(2861),c=i.process,u=c&&c.versions,s=u&&u.v8;s?o=(n=s.split(\".\"))[0]+n[1]:a&&(!(n=a."
            "match(/Edge\\/(\\d+)/))||n[1]>=74)&&(n=a.match(/Chrome\\/(\\d+)/"
            "))&&(o=n[1]),t.exports=o&&+o},6759:t=>{t.exports=[\"constructor\",\"hasOwnProperty\",\"isPrototypeOf\","
            "\"propertyIsEnumerable\",\"toLocaleString\",\"toString\",\"valueOf\"]},6887:(t,r,e)=>{\"use strict\";var "
            "n=e(1899),o=e(9677).f,i=e(7252),a=e(4058),c=e(6843),u=e(2029),s=e(7457),f=function(t){var "
            "r=function(r,e,n){if(this instanceof t){switch(arguments.length){case 0:return new t;case 1:return new "
            "t(r);case 2:return new t(r,e)}return new t(r,e,n)}return t.apply(this,arguments)};return "
            "r.prototype=t.prototype,r};t.exports=function(t,r){var "
            "e,l,p,v,h,d,y,g,m=t.target,x=t.global,b=t.stat,w=t.proto,j=x?n:b?n[m]:(n[m]||{}).prototype,S=x?a:a[m]||(a["
            "m]={}),T=S.prototype;for(p in "
            "r)e=!i(x?p:m+(b?\".\":\"#\")+p,t.forced)&&j&&s(j,p),h=S[p],e&&(d=t.noTargetGet?(g=o(j,p))&&g.value:j[p]),"
            "v=e&&d?d:r[p],e&&typeof h==typeof v||(y=t.bind&&e?c(v,n):t.wrap&&e?f(v):w&&\"function\"==typeof "
            "v?c(Function.call,v):v,(t.sham||v&&v.sham||h&&h.sham)&&u(y,\"sham\",!0),S[p]=y,w&&(s(a,l=m+\"Prototype\")|"
            "|u(a,l,{}),a[l][p]=v,t.real&&T&&!T[p]&&u(T,p,v)))}},5981:t=>{t.exports=function(t){try{return!!t()}catch("
            "t){return!0}}},6843:(t,r,e)=>{var n=e(3916);t.exports=function(t,r,e){if(n(t),void 0===r)return "
            "t;switch(e){case 0:return function(){return t.call(r)};case 1:return function(e){return t.call(r,e)};case "
            "2:return function(e,n){return t.call(r,e,n)};case 3:return function(e,n,o){return t.call(r,e,n,o)}}return "
            "function(){return t.apply(r,arguments)}}},626:(t,r,e)=>{var "
            "n=e(4058),o=e(1899),i=function(t){return\"function\"==typeof t?t:void 0};t.exports=function(t,r){return "
            "arguments.length<2?i(n[t])||i(o[t]):n[t]&&n[t][r]||o[t]&&o[t][r]}},2902:(t,r,e)=>{var "
            "n=e(9697),o=e(2077),i=e(9813)(\"iterator\");t.exports=function(t){if(null!=t)return "
            "t[i]||t[\"@@iterator\"]||o[n(t)]}},1899:(t,r,e)=>{var n=function(t){return "
            "t&&t.Math==Math&&t};t.exports=n(\"object\"==typeof globalThis&&globalThis)||n(\"object\"==typeof "
            "window&&window)||n(\"object\"==typeof self&&self)||n(\"object\"==typeof e.g&&e.g)||function(){return "
            "this}()||Function(\"return this\")()},7457:(t,r,e)=>{var "
            "n=e(9678),o={}.hasOwnProperty;t.exports=function(t,r){return "
            "o.call(n(t),r)}},7748:t=>{t.exports={}},4845:(t,r,e)=>{var n=e(1899);t.exports=function(t,r){var "
            "e=n.console;e&&e.error&&(1===arguments.length?e.error(t):e.error(t,r))}},5463:(t,r,e)=>{var "
            "n=e(626);t.exports=n(\"document\",\"documentElement\")},2840:(t,r,e)=>{var "
            "n=e(5746),o=e(5981),i=e(1333);t.exports=!n&&!o((function(){return "
            "7!=Object.defineProperty(i(\"div\"),\"a\",{get:function(){return 7}}).a}))},7026:(t,r,e)=>{var "
            "n=e(5981),o=e(2532),i=\"\".split;t.exports=n((function(){return!Object(\"z\").propertyIsEnumerable(0)}))?"
            "function(t){return\"String\"==o(t)?i.call(t,\"\"):Object(t)}:Object},1302:(t,r,e)=>{var "
            "n=e(3030),o=Function.toString;\"function\"!=typeof n.inspectSource&&(n.inspectSource=function(t){return "
            "o.call(t)}),t.exports=n.inspectSource},5402:(t,r,e)=>{var "
            "n,o,i,a=e(8019),c=e(1899),u=e(941),s=e(2029),f=e(7457),l=e(3030),p=e(4262),v=e(7748),h=\"Object already "
            "initialized\",d=c.WeakMap;if(a){var y=l.state||(l.state=new "
            "d),g=y.get,m=y.has,x=y.set;n=function(t,r){if(m.call(y,t))throw new TypeError(h);return "
            "r.facade=t,x.call(y,t,r),r},o=function(t){return g.call(y,t)||{}},i=function(t){return "
            "m.call(y,t)}}else{var b=p(\"state\");v[b]=!0,n=function(t,r){if(f(t,b))throw new TypeError(h);return "
            "r.facade=t,s(t,b,r),r},o=function(t){return f(t,b)?t[b]:{}},i=function(t){return "
            "f(t,b)}}t.exports={set:n,get:o,has:i,enforce:function(t){return "
            "i(t)?o(t):n(t,{})},getterFor:function(t){return function(r){var e;if(!u(r)||(e=o(r)).type!==t)throw "
            "TypeError(\"Incompatible receiver, \"+t+\" required\");return e}}}},6782:(t,r,e)=>{var "
            "n=e(9813),o=e(2077),i=n(\"iterator\"),a=Array.prototype;t.exports=function(t){return void "
            "0!==t&&(o.Array===t||a[i]===t)}},7252:(t,r,e)=>{var n=e(5981),o=/#|\\.prototype\\./,i=function(t,r){var "
            "e=c[a(t)];return e==s||e!=u&&(\"function\"==typeof r?n(r):!!r)},a=i.normalize=function(t){return "
            "String(t).replace(o,\".\").toLowerCase()},c=i.data={},u=i.NATIVE=\"N\",s=i.POLYFILL=\"P\";t.exports=i},"
            "941:t=>{t.exports=function(t){return\"object\"==typeof t?null!==t:\"function\"==typeof "
            "t}},2529:t=>{t.exports=!0},3091:(t,r,e)=>{var "
            "n=e(6059),o=e(6782),i=e(3057),a=e(6843),c=e(2902),u=e(7609),s=function(t,r){this.stopped=t,this.result=r};"
            "t.exports=function(t,r,e){var "
            "f,l,p,v,h,d,y,g=e&&e.that,m=!(!e||!e.AS_ENTRIES),x=!(!e||!e.IS_ITERATOR),b=!(!e||!e.INTERRUPTED),w=a(r,g,"
            "1+m+b),j=function(t){return f&&u(f),new s(!0,t)},S=function(t){return "
            "m?(n(t),b?w(t[0],t[1],j):w(t[0],t[1])):b?w(t,j):w(t)};if(x)f=t;else{if(\"function\"!=typeof(l=c(t)))throw "
            "TypeError(\"Target is not iterable\");if(o(l)){for(p=0,v=i(t.length);v>p;p++)if((h=S(t[p]))&&h instanceof "
            "s)return h;return new "
            "s(!1)}f=l.call(t)}for(d=f.next;!(y=d.call(f)).done;){try{h=S(y.value)}catch(t){throw "
            "u(f),t}if(\"object\"==typeof h&&h&&h instanceof s)return h}return new s(!1)}},7609:(t,r,e)=>{var "
            "n=e(6059);t.exports=function(t){var r=t.return;if(void 0!==r)return "
            "n(r.call(t)).value}},5143:(t,r,e)=>{\"use strict\";var "
            "n,o,i,a=e(5981),c=e(249),u=e(2029),s=e(7457),f=e(9813),l=e(2529),p=f(\"iterator\"),v=!1;[].keys&&("
            "\"next\"in(i=[].keys())?(o=c(c(i)))!==Object.prototype&&(n=o):v=!0);var h=null==n||a((function(){var "
            "t={};return n[p].call(t)!==t}));h&&(n={}),l&&!h||s(n,p)||u(n,p,(function(){return "
            "this})),t.exports={IteratorPrototype:n,BUGGY_SAFARI_ITERATORS:v}},2077:t=>{t.exports={}},6132:(t,r,e)=>{"
            "var "
            "n,o,i,a,c,u,s,f,l=e(1899),p=e(9677).f,v=e(2941).set,h=e(2749),d=e(8045),y=e(6049),g=l.MutationObserver||l."
            "WebKitMutationObserver,m=l.document,x=l.process,b=l.Promise,w=p(l,\"queueMicrotask\"),j=w&&w.value;j||(n="
            "function(){var t,r;for(y&&(t=x.domain)&&t.exit();o;){r=o.fn,o=o.next;try{r()}catch(t){throw o?a():i=void "
            "0,t}}i=void 0,t&&t.enter()},h||y||d||!g||!m?b&&b.resolve?(s=b.resolve(void "
            "0),f=s.then,a=function(){f.call(s,n)}):a=y?function(){x.nextTick(n)}:function(){v.call(l,n)}:(c=!0,u=m."
            "createTextNode(\"\"),new "
            "g(n).observe(u,{characterData:!0}),a=function(){u.data=c=!c})),t.exports=j||function(t){var "
            "r={fn:t,next:void 0};i&&(i.next=r),o||(o=r,a()),i=r}},9297:(t,r,e)=>{var "
            "n=e(1899);t.exports=n.Promise},2497:(t,r,e)=>{var "
            "n=e(6049),o=e(3385),i=e(5981);t.exports=!!Object.getOwnPropertySymbols&&!i((function(){return!Symbol.sham&"
            "&(n?38===o:o>37&&o<41)}))},8019:(t,r,e)=>{var "
            "n=e(1899),o=e(1302),i=n.WeakMap;t.exports=\"function\"==typeof i&&/native "
            "code/.test(o(i))},9520:(t,r,e)=>{\"use strict\";var n=e(3916),o=function(t){var r,e;this.promise=new "
            "t((function(t,n){if(void 0!==r||void 0!==e)throw TypeError(\"Bad Promise "
            "constructor\");r=t,e=n})),this.resolve=n(r),this.reject=n(e)};t.exports.f=function(t){return new "
            "o(t)}},9290:(t,r,e)=>{var "
            "n,o=e(6059),i=e(9938),a=e(6759),c=e(7748),u=e(5463),s=e(1333),f=e(4262)(\"IE_PROTO\"),l=function(){},p="
            "function(t){return\"<script>\"+t+\"<\\/script>\"},v=function(){try{n=document.domain&&new "
            "ActiveXObject(\"htmlfile\")}catch(t){}var t,r;v=n?function(t){t.write(p(\"\")),t.close();var "
            "r=t.parentWindow.Object;return "
            "t=null,r}(n):((r=s(\"iframe\")).style.display=\"none\",u.appendChild(r),r.src=String(\"javascript:\"),(t="
            "r.contentWindow.document).open(),t.write(p(\"document.F=Object\")),t.close(),t.F);for(var "
            "e=a.length;e--;)delete v.prototype[a[e]];return v()};c[f]=!0,t.exports=Object.create||function(t,r){var "
            "e;return null!==t?(l.prototype=o(t),e=new l,l.prototype=null,e[f]=t):e=v(),void "
            "0===r?e:i(e,r)}},9938:(t,r,e)=>{var "
            "n=e(5746),o=e(5988),i=e(6059),a=e(4771);t.exports=n?Object.defineProperties:function(t,r){i(t);for(var "
            "e,n=a(r),c=n.length,u=0;c>u;)o.f(t,e=n[u++],r[e]);return t}},5988:(t,r,e)=>{var "
            "n=e(5746),o=e(2840),i=e(6059),a=e(6935),c=Object.defineProperty;r.f=n?c:function(t,r,e){if(i(t),r=a(r,!0),"
            "i(e),o)try{return c(t,r,e)}catch(t){}if(\"get\"in e||\"set\"in e)throw TypeError(\"Accessors not "
            "supported\");return\"value\"in e&&(t[r]=e.value),t}},9677:(t,r,e)=>{var "
            "n=e(5746),o=e(6760),i=e(1887),a=e(4529),c=e(6935),u=e(7457),s=e(2840),f=Object.getOwnPropertyDescriptor;r."
            "f=n?f:function(t,r){if(t=a(t),r=c(r,!0),s)try{return f(t,r)}catch(t){}if(u(t,r))return "
            "i(!o.f.call(t,r),t[r])}},249:(t,r,e)=>{var "
            "n=e(7457),o=e(9678),i=e(4262),a=e(4160),c=i(\"IE_PROTO\"),u=Object.prototype;t.exports=a?Object."
            "getPrototypeOf:function(t){return t=o(t),n(t,c)?t[c]:\"function\"==typeof t.constructor&&t instanceof "
            "t.constructor?t.constructor.prototype:t instanceof Object?u:null}},5629:(t,r,e)=>{var "
            "n=e(7457),o=e(4529),i=e(1692).indexOf,a=e(7748);t.exports=function(t,r){var e,c=o(t),u=0,s=[];for(e in "
            "c)!n(a,e)&&n(c,e)&&s.push(e);for(;r.length>u;)n(c,e=r[u++])&&(~i(s,e)||s.push(e));return "
            "s}},4771:(t,r,e)=>{var n=e(5629),o=e(6759);t.exports=Object.keys||function(t){return "
            "n(t,o)}},6760:(t,r)=>{\"use strict\";var "
            "e={}.propertyIsEnumerable,n=Object.getOwnPropertyDescriptor,o=n&&!e.call({1:2},1);r.f=o?function(t){var "
            "r=n(this,t);return!!r&&r.enumerable}:e},8929:(t,r,e)=>{var "
            "n=e(6059),o=e(1851);t.exports=Object.setPrototypeOf||(\"__proto__\"in{}?function(){var "
            "t,r=!1,e={};try{(t=Object.getOwnPropertyDescriptor(Object.prototype,\"__proto__\").set).call(e,[]),r=e "
            "instanceof Array}catch(t){}return function(e,i){return n(e),o(i),r?t.call(e,i):e.__proto__=i,e}}():void "
            "0)},5623:(t,r,e)=>{\"use strict\";var "
            "n=e(2885),o=e(9697);t.exports=n?{}.toString:function(){return\"[object "
            "\"+o(this)+\"]\"}},4058:t=>{t.exports={}},2:t=>{t.exports=function(t){try{return{error:!1,value:t()}}"
            "catch(t){return{error:!0,value:t}}}},6584:(t,r,e)=>{var "
            "n=e(6059),o=e(941),i=e(9520);t.exports=function(t,r){if(n(t),o(r)&&r.constructor===t)return r;var "
            "e=i.f(t);return(0,e.resolve)(r),e.promise}},7524:(t,r,e)=>{var "
            "n=e(9754);t.exports=function(t,r,e){for(var o in r)e&&e.unsafe&&t[o]?t[o]=r[o]:n(t,o,r[o],e);return "
            "t}},9754:(t,r,e)=>{var "
            "n=e(2029);t.exports=function(t,r,e,o){o&&o.enumerable?t[r]=e:n(t,r,e)}},8219:t=>{t.exports=function(t){if("
            "null==t)throw TypeError(\"Can't call method on \"+t);return t}},4911:(t,r,e)=>{var "
            "n=e(1899),o=e(2029);t.exports=function(t,r){try{o(n,t,r)}catch(e){n[t]=r}return r}},4431:(t,r,e)=>{\"use "
            "strict\";var n=e(626),o=e(5988),i=e(9813),a=e(5746),c=i(\"species\");t.exports=function(t){var "
            "r=n(t),e=o.f;a&&r&&!r[c]&&e(r,c,{configurable:!0,get:function(){return this}})}},904:(t,r,e)=>{var "
            "n=e(2885),o=e(5988).f,i=e(2029),a=e(7457),c=e(5623),u=e(9813)(\"toStringTag\");t.exports=function(t,r,e,s)"
            "{if(t){var "
            "f=e?t:t.prototype;a(f,u)||o(f,u,{configurable:!0,value:r}),s&&!n&&i(f,\"toString\",c)}}},4262:(t,r,e)=>{"
            "var n=e(8726),o=e(9418),i=n(\"keys\");t.exports=function(t){return i[t]||(i[t]=o(t))}},3030:(t,r,e)=>{var "
            "n=e(1899),o=e(4911),i=\"__core-js_shared__\",a=n[i]||o(i,{});t.exports=a},8726:(t,r,e)=>{var "
            "n=e(2529),o=e(3030);(t.exports=function(t,r){return o[t]||(o[t]=void "
            "0!==r?r:{})})(\"versions\",[]).push({version:\"3.11.1\",mode:n?\"pure\":\"global\",copyright:\"© 2021 "
            "Denis Pushkarev (zloirock.ru)\"})},487:(t,r,e)=>{var "
            "n=e(6059),o=e(3916),i=e(9813)(\"species\");t.exports=function(t,r){var e,a=n(t).constructor;return void "
            "0===a||null==(e=n(a)[i])?r:o(e)}},4620:(t,r,e)=>{var n=e(8459),o=e(8219),i=function(t){return "
            "function(r,e){var i,a,c=String(o(r)),u=n(e),s=c.length;return u<0||u>=s?t?\"\":void "
            "0:(i=c.charCodeAt(u))<55296||i>56319||u+1===s||(a=c.charCodeAt(u+1))<56320||a>57343?t?c.charAt(u):i:t?c."
            "slice(u,u+2):a-56320+(i-55296<<10)+65536}};t.exports={codeAt:i(!1),charAt:i(!0)}},2941:(t,r,e)=>{var "
            "n,o,i,a=e(1899),c=e(5981),u=e(6843),s=e(5463),f=e(1333),l=e(2749),p=e(6049),v=a.location,h=a.setImmediate,"
            "d=a.clearImmediate,y=a.process,g=a.MessageChannel,m=a.Dispatch,x=0,b={},w=function(t){if(b.hasOwnProperty("
            "t)){var r=b[t];delete b[t],r()}},j=function(t){return "
            "function(){w(t)}},S=function(t){w(t.data)},T=function(t){a.postMessage(t+\"\",v.protocol+\"//"
            "\"+v.host)};h&&d||(h=function(t){for(var r=[],e=1;arguments.length>e;)r.push(arguments[e++]);return "
            "b[++x]=function(){(\"function\"==typeof t?t:Function(t)).apply(void 0,r)},n(x),x},d=function(t){delete "
            "b[t]},p?n=function(t){y.nextTick(j(t))}:m&&m.now?n=function(t){m.now(j(t))}:g&&!l?(i=(o=new "
            "g).port2,o.port1.onmessage=S,n=u(i.postMessage,i,1)):a.addEventListener&&\"function\"==typeof "
            "postMessage&&!a.importScripts&&v&&\"file:\"!==v.protocol&&!c(T)?(n=T,a.addEventListener(\"message\",S,!1))"
            ":n=\"onreadystatechange\"in "
            "f(\"script\")?function(t){s.appendChild(f(\"script\")).onreadystatechange=function(){s.removeChild(this),"
            "w(t)}}:function(t){setTimeout(j(t),0)}),t.exports={set:h,clear:d}},9413:(t,r,e)=>{var "
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
            "n=e(2497);t.exports=n&&!Symbol.sham&&\"symbol\"==typeof Symbol.iterator},9813:(t,r,e)=>{var "
            "n=e(1899),o=e(8726),i=e(7457),a=e(9418),c=e(2497),u=e(2302),s=o(\"wks\"),f=n.Symbol,l=u?f:f&&f."
            "withoutSetter||a;t.exports=function(t){return i(s,t)&&(c||\"string\"==typeof "
            "s[t])||(c&&i(f,t)?s[t]=f[t]:s[t]=l(\"Symbol.\"+t)),s[t]}},7627:(t,r,e)=>{\"use strict\";var "
            "n=e(6887),o=e(249),i=e(8929),a=e(9290),c=e(2029),u=e(1887),s=e(3091),f=function(t,r){var e=this;if(!(e "
            "instanceof f))return new f(t,r);i&&(e=i(new Error(void 0),o(e))),void "
            "0!==r&&c(e,\"message\",String(r));var n=[];return "
            "s(t,n.push,{that:n}),c(e,\"errors\",n),e};f.prototype=a(Error.prototype,{constructor:u(5,f),message:u(5,"
            "\"\"),name:u(5,\"AggregateError\")}),n({global:!0},{AggregateError:f})},6274:(t,r,e)=>{\"use strict\";var "
            "n=e(4529),o=e(8479),i=e(2077),a=e(5402),c=e(7771),u=\"Array "
            "Iterator\",s=a.set,f=a.getterFor(u);t.exports=c(Array,\"Array\",(function(t,r){s(this,{type:u,target:n(t),"
            "index:0,kind:r})}),(function(){var "
            "t=f(this),r=t.target,e=t.kind,n=t.index++;return!r||n>=r.length?(t.target=void 0,{value:void "
            "0,done:!0}):\"keys\"==e?{value:n,done:!1}:\"values\"==e?{value:r[n],done:!1}:{value:[n,r[n]],done:!1}}),"
            "\"values\"),i.Arguments=i.Array,o(\"keys\"),o(\"values\"),o(\"entries\")},5967:()=>{},4560:(t,r,e)=>{"
            "\"use strict\";var "
            "n=e(6887),o=e(3916),i=e(9520),a=e(2),c=e(3091);n({target:\"Promise\",stat:!0},{allSettled:function(t){var "
            "r=this,e=i.f(r),n=e.resolve,u=e.reject,s=a((function(){var "
            "e=o(r.resolve),i=[],a=0,u=1;c(t,(function(t){var o=a++,c=!1;i.push(void "
            "0),u++,e.call(r,t).then((function(t){c||(c=!0,i[o]={status:\"fulfilled\",value:t},--u||n(i))}),(function("
            "t){c||(c=!0,i[o]={status:\"rejected\",reason:t},--u||n(i))}))})),--u||n(i)}));return "
            "s.error&&u(s.value),e.promise}})},7206:(t,r,e)=>{\"use strict\";var "
            "n=e(6887),o=e(3916),i=e(626),a=e(9520),c=e(2),u=e(3091),s=\"No one promise "
            "resolved\";n({target:\"Promise\",stat:!0},{any:function(t){var "
            "r=this,e=a.f(r),n=e.resolve,f=e.reject,l=c((function(){var "
            "e=o(r.resolve),a=[],c=0,l=1,p=!1;u(t,(function(t){var o=c++,u=!1;a.push(void "
            "0),l++,e.call(r,t).then((function(t){u||p||(p=!0,n(t))}),(function(t){u||p||(u=!0,a[o]=t,--l||f(new(i("
            "\"AggregateError\"))(a,s)))}))})),--l||f(new(i(\"AggregateError\"))(a,s))}));return "
            "l.error&&f(l.value),e.promise}})},4349:(t,r,e)=>{\"use strict\";var "
            "n=e(6887),o=e(2529),i=e(9297),a=e(5981),c=e(626),u=e(487),s=e(6584),f=e(9754);n({target:\"Promise\",proto:"
            "!0,real:!0,forced:!!i&&a((function(){i.prototype.finally.call({then:function(){}},(function(){}))}))},{"
            "finally:function(t){var r=u(this,c(\"Promise\")),e=\"function\"==typeof t;return "
            "this.then(e?function(e){return s(r,t()).then((function(){return e}))}:t,e?function(e){return "
            "s(r,t()).then((function(){throw e}))}:t)}}),o||\"function\"!=typeof "
            "i||i.prototype.finally||f(i.prototype,\"finally\",c(\"Promise\").prototype.finally)},8881:(t,r,e)=>{\"use "
            "strict\";var "
            "n,o,i,a,c=e(6887),u=e(2529),s=e(1899),f=e(626),l=e(9297),p=e(9754),v=e(7524),h=e(8929),d=e(904),y=e(4431),"
            "g=e(941),m=e(3916),x=e(5743),b=e(1302),w=e(3091),j=e(1385),S=e(487),T=e(2941).set,O=e(6132),E=e(6584),P=e("
            "4845),A=e(9520),L=e(2),I=e(5402),M=e(7252),_=e(9813),k=e(6049),C=e(3385),R=_(\"species\"),F=\"Promise\",N="
            "I.get,D=I.set,G=I.getterFor(F),q=l&&l.prototype,V=l,z=s.TypeError,H=s.document,U=s.process,W=A.f,B=W,Y=!!("
            "H&&H.createEvent&&s.dispatchEvent),K=\"function\"==typeof "
            "PromiseRejectionEvent,X=\"unhandledrejection\",J=M(F,(function(){if(b(V)===String(V)){if(66===C)return!0;"
            "if(!k&&!K)return!0}if(u&&!V.prototype.finally)return!0;if(C>=51&&/native code/.test(V))return!1;var "
            "t=V.resolve(1),r=function(t){t((function(){}),(function(){}))};return(t.constructor={})[R]=r,!(t.then(("
            "function(){}))instanceof r)})),Q=J||!j((function(t){V.all(t).catch((function(){}))})),Z=function(t){var "
            "r;return!(!g(t)||\"function\"!=typeof(r=t.then))&&r},$=function(t,r){if(!t.notified){t.notified=!0;var "
            "e=t.reactions;O((function(){for(var n=t.value,o=1==t.state,i=0;e.length>i;){var "
            "a,c,u,s=e[i++],f=o?s.ok:s.fail,l=s.resolve,p=s.reject,v=s.domain;try{f?(o||(2===t.rejection&&nt(t),t."
            "rejection=1),!0===f?a=n:(v&&v.enter(),a=f(n),v&&(v.exit(),u=!0)),a===s.promise?p(z(\"Promise-chain "
            "cycle\")):(c=Z(a))?c.call(a,l,p):l(a)):p(n)}catch(t){v&&!u&&v.exit(),p(t)}}t.reactions=[],t.notified=!1,r&"
            "&!t.rejection&&rt(t)}))}},tt=function(t,r,e){var "
            "n,o;Y?((n=H.createEvent(\"Event\")).promise=r,n.reason=e,n.initEvent(t,!1,!0),s.dispatchEvent(n)):n={"
            "promise:r,reason:e},!K&&(o=s[\"on\"+t])?o(n):t===X&&P(\"Unhandled promise "
            "rejection\",e)},rt=function(t){T.call(s,(function(){var "
            "r,e=t.facade,n=t.value;if(et(t)&&(r=L((function(){k?U.emit(\"unhandledRejection\",n,e):tt(X,e,n)})),t."
            "rejection=k||et(t)?2:1,r.error))throw r.value}))},et=function(t){return "
            "1!==t.rejection&&!t.parent},nt=function(t){T.call(s,(function(){var "
            "r=t.facade;k?U.emit(\"rejectionHandled\",r):tt(\"rejectionhandled\",r,t.value)}))},ot=function(t,r,e){"
            "return "
            "function(n){t(r,n,e)}},it=function(t,r,e){t.done||(t.done=!0,e&&(t=e),t.value=r,t.state=2,$(t,!0))},at="
            "function(t,r,e){if(!t.done){t.done=!0,e&&(t=e);try{if(t.facade===r)throw z(\"Promise can't be resolved "
            "itself\");var n=Z(r);n?O((function(){var "
            "e={done:!1};try{n.call(r,ot(at,e,t),ot(it,e,t))}catch(r){it(e,r,t)}})):(t.value=r,t.state=1,$(t,!1))}"
            "catch(r){it({done:!1},r,t)}}};if(J&&(V=function(t){x(this,V,F),m(t),n.call(this);var "
            "r=N(this);try{t(ot(at,r),ot(it,r))}catch(t){it(r,t)}},(n=function(t){D(this,{type:F,done:!1,notified:!1,"
            "parent:!1,reactions:[],rejection:!1,state:0,value:void "
            "0})}).prototype=v(V.prototype,{then:function(t,r){var e=G(this),n=W(S(this,V));return "
            "n.ok=\"function\"!=typeof t||t,n.fail=\"function\"==typeof r&&r,n.domain=k?U.domain:void "
            "0,e.parent=!0,e.reactions.push(n),0!=e.state&&$(e,!1),n.promise},catch:function(t){return this.then(void "
            "0,t)}}),o=function(){var t=new "
            "n,r=N(t);this.promise=t,this.resolve=ot(at,r),this.reject=ot(it,r)},A.f=W=function(t){return "
            "t===V||t===i?new o(t):B(t)},!u&&\"function\"==typeof "
            "l&&q!==Object.prototype)){a=q.then,p(q,\"then\",(function(t,r){var e=this;return new "
            "V((function(t,r){a.call(e,t,r)})).then(t,r)}),{unsafe:!0});try{delete "
            "q.constructor}catch(t){}h&&h(q,V.prototype)}c({global:!0,wrap:!0,forced:J},{Promise:V}),d(V,F,!1,!0),y(F),"
            "i=f(F),c({target:F,stat:!0,forced:J},{reject:function(t){var r=W(this);return r.reject.call(void "
            "0,t),r.promise}}),c({target:F,stat:!0,forced:u||J},{resolve:function(t){return "
            "E(u&&this===i?V:this,t)}}),c({target:F,stat:!0,forced:Q},{all:function(t){var "
            "r=this,e=W(r),n=e.resolve,o=e.reject,i=L((function(){var e=m(r.resolve),i=[],a=0,c=1;w(t,(function(t){var "
            "u=a++,s=!1;i.push(void "
            "0),c++,e.call(r,t).then((function(t){s||(s=!0,i[u]=t,--c||n(i))}),o)})),--c||n(i)}));return "
            "i.error&&o(i.value),e.promise},race:function(t){var r=this,e=W(r),n=e.reject,o=L((function(){var "
            "o=m(r.resolve);w(t,(function(t){o.call(r,t).then(e.resolve,n)}))}));return "
            "o.error&&n(o.value),e.promise}})},7971:(t,r,e)=>{\"use strict\";var "
            "n=e(4620).charAt,o=e(5402),i=e(7771),a=\"String "
            "Iterator\",c=o.set,u=o.getterFor(a);i(String,\"String\",(function(t){c(this,{type:a,string:String(t),"
            "index:0})}),(function(){var t,r=u(this),e=r.string,o=r.index;return o>=e.length?{value:void "
            "0,done:!0}:(t=n(e,o),r.index+=t.length,{value:t,done:!1})}))},7634:(t,r,e)=>{e(6274);var "
            "n=e(3281),o=e(1899),i=e(9697),a=e(2029),c=e(2077),u=e(9813)(\"toStringTag\");for(var s in n){var "
            "f=o[s],l=f&&f.prototype;l&&i(l)!==u&&a(l,u,s),c[s]=c.Array}},7460:(t,r,e)=>{var "
            "n=e(2956);t.exports=n}},r={};function e(n){var o=r[n];if(void 0!==o)return o.exports;var "
            "i=r[n]={exports:{}};return t[n](i,i.exports,e),i.exports}e.n=t=>{var "
            "r=t&&t.__esModule?()=>t.default:()=>t;return e.d(r,{a:r}),r},e.d=(t,r)=>{for(var n in "
            "r)e.o(r,n)&&!e.o(t,n)&&Object.defineProperty(t,n,{enumerable:!0,get:r[n]})},e.g=function(){if(\"object\"=="
            "typeof globalThis)return globalThis;try{return this||new Function(\"return "
            "this\")()}catch(t){if(\"object\"==typeof window)return "
            "window}}(),e.o=(t,r)=>Object.prototype.hasOwnProperty.call(t,r),(()=>{\"use strict\";var "
            "t=e(7460),r=e.n(t),n=globalThis.addon.createPromise();globalThis.assert.strictEqual(globalThis.addon."
            "isPromise(n),!0),globalThis.addon.concludeCurrentPromise(void "
            "0,!0),r().reject(-1).catch((function(t){globalThis.assert.strictEqual(t,-1)})),globalThis.assert."
            "strictEqual(globalThis.addon.isPromise(2.4),!1),globalThis.assert.strictEqual(globalThis.addon.isPromise("
            "\"I promise!\"),!1),globalThis.assert.strictEqual(globalThis.addon.isPromise(void "
            "0),!1),globalThis.assert.strictEqual(globalThis.addon.isPromise(null),!1),globalThis.assert.strictEqual("
            "globalThis.addon.isPromise({}),!1)})()})();",
            "https://www.didi.com/test_promise.js", &result),
        NAPIOK);
}