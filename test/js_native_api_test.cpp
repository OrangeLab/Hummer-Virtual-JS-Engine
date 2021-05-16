#include <napi/js_native_api_types.h>
#include <gtest/gtest.h>
#include <napi/js_native_api.h>
#include <js_native_api_test.h>

NAPIEnv globalEnv = nullptr;

namespace {
    class NAPIEnvironment : public ::testing::Environment {
    public:
        // Override this to define how to set up the environment.
        void SetUp() override {
            ASSERT_EQ(NAPICreateEnv(&globalEnv), NAPIOK);
            NAPIValue result;
            ASSERT_EQ(NAPIRunScriptWithSourceUrl(globalEnv,
                                                 "(()=>{\"use strict\";function t(n,r){return(t=Object.setPrototypeOf||function(t,n){return t.__proto__=n,t})(n,r)}function n(t){return(n=Object.setPrototypeOf?Object.getPrototypeOf:function(t){return t.__proto__||Object.getPrototypeOf(t)})(t)}function r(){if(\"undefined\"==typeof Reflect||!Reflect.construct)return!1;if(Reflect.construct.sham)return!1;if(\"function\"==typeof Proxy)return!0;try{return Boolean.prototype.valueOf.call(Reflect.construct(Boolean,[],(function(){}))),!0}catch(t){return!1}}function e(t){return(e=\"function\"==typeof Symbol&&\"symbol\"==typeof Symbol.iterator?function(t){return typeof t}:function(t){return t&&\"function\"==typeof Symbol&&t.constructor===Symbol&&t!==Symbol.prototype?\"symbol\":typeof t})(t)}function o(t,n){return!n||\"object\"!==e(n)&&\"function\"!=typeof n?function(t){if(void 0===t)throw new ReferenceError(\"this hasn't been initialised - super() hasn't been called\");return t}(t):n}function u(n,e,o){return(u=r()?Reflect.construct:function(n,r,e){var o=[null];o.push.apply(o,r);var u=new(Function.bind.apply(n,o));return e&&t(u,e.prototype),u}).apply(null,arguments)}function c(r){var e=\"function\"==typeof Map?new Map:void 0;return(c=function(r){if(null===r||(o=r,-1===Function.toString.call(o).indexOf(\"[native code]\")))return r;var o;if(\"function\"!=typeof r)throw new TypeError(\"Super expression must either be null or a function\");if(void 0!==e){if(e.has(r))return e.get(r);e.set(r,c)}function c(){return u(r,arguments,n(this).constructor)}return c.prototype=Object.create(r.prototype,{constructor:{value:c,enumerable:!1,writable:!0,configurable:!0}}),t(c,r)})(r)}var i=function(e){!function(n,r){if(\"function\"!=typeof r&&null!==r)throw new TypeError(\"Super expression must either be null or a function\");n.prototype=Object.create(r&&r.prototype,{constructor:{value:n,writable:!0,configurable:!0}}),r&&t(n,r)}(f,e);var u,c,i=(u=f,c=r(),function(){var t,r=n(u);if(c){var e=n(this).constructor;t=Reflect.construct(r,arguments,e)}else t=r.apply(this,arguments);return o(this,t)});function f(t){var n;return function(t,n){if(!(t instanceof n))throw new TypeError(\"Cannot call a class as a function\")}(this,f),(n=i.call(this,t)).name=\"AssertionError\",n}return f}(c(Error));function f(t){return`${String(t)}`}function l(t){var n=arguments.length>1&&void 0!==arguments[1]?arguments[1]:\"\";if(!t)throw new i(n)}l.ok=l,l.strictEqual=function(t,n,r){if(!Object.is(t,n)){var e=r;throw e||(e=`${f(t)} !== ${f(n)}`),new i(e)}},l.notStrictEqual=function(t,n,r){if(Object.is(t,n))throw new i(null!=r?r:`Expected \"actual\" to be strictly unequal to: ${f(t)}\\n`)},l.throws=function(t,n){var r=!1,e=null;try{t()}catch(t){r=!0,e=t}if(!r)throw new i(\"Expected function to throw\"+(n?`: ${n}`:\".\"));return e},globalThis.assert=l})();",
                                                 "https://www.didi.com/assert.js", &result), NAPIOK);
        }

        // Override this to define how to tear down the environment.
        void TearDown() override {
            ASSERT_EQ(NAPIFreeEnv(globalEnv), NAPIOK);
        }
    };
}

int main(int argc, char **argv) {
    ::testing::AddGlobalTestEnvironment(new NAPIEnvironment());
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}

void ::Test::SetUp() {
    NAPIValue global;
    ASSERT_EQ(napi_get_global(globalEnv, &global), NAPIOK);

    ASSERT_EQ(napi_create_object(globalEnv, &exports), NAPIOK);
    ASSERT_EQ(napi_set_named_property(globalEnv, global, "addon", exports), NAPIOK);
}

void ::Test::TearDown() {
    NAPIValue result;
    ASSERT_EQ(napi_get_and_clear_last_exception(globalEnv, &result), NAPIOK);
}