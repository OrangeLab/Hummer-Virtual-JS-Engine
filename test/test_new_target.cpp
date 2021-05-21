#include <common.h>
#include <js_native_api_test.h>

EXTERN_C_START

static NAPIValue Constructor(NAPIEnv env, NAPICallbackInfo info)
{
    bool result;
    NAPIValue newTargetArg;
    NAPI_CALL(env, napi_get_new_target(env, info, &newTargetArg));
    size_t argc = 1;
    NAPIValue argv;
    NAPIValue thisArg;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &argv, &thisArg, nullptr));
    NAPIValue undefined;
    NAPI_CALL(env, napi_get_undefined(env, &undefined));

    // new.target !== undefined because we should be called as a new expression
    NAPIValueType valueType;
    NAPI_CALL(env, napi_typeof(env, newTargetArg, &valueType));
    NAPI_ASSERT(env, valueType != NAPIUndefined, "newTargetArg != nullptr");
    NAPI_CALL(env, napi_strict_equals(env, newTargetArg, undefined, &result));
    NAPI_ASSERT(env, !result, "new.target !== undefined");

    // arguments[0] should be Constructor itself (test harness passed it)
    NAPI_CALL(env, napi_strict_equals(env, newTargetArg, argv, &result));
    NAPI_ASSERT(env, result, "new.target === Constructor");

    return thisArg;
}

static NAPIValue OrdinaryFunction(NAPIEnv env, NAPICallbackInfo info)
{
    NAPIValue newTargetArg;
    NAPI_CALL(env, napi_get_new_target(env, info, &newTargetArg));

    NAPIValueType valueType;
    NAPI_CALL(env, napi_typeof(env, newTargetArg, &valueType));
    NAPI_ASSERT(env, valueType == NAPIUndefined, "newTargetArg == nullptr");

    NAPIValue _true;
    NAPI_CALL(env, napi_get_boolean(env, true, &_true));
    return _true;
}

EXTERN_C_END

TEST_F(Test, TestNewTarget)
{
    NAPIValue constructor;
    ASSERT_EQ(napi_define_class(globalEnv, nullptr, -1, Constructor, nullptr, 0, nullptr, &constructor), NAPIOK);

    // JavaScriptCore 无法 new method();
    const NAPIPropertyDescriptor desc[] = {
        DECLARE_NAPI_PROPERTY("OrdinaryFunction", OrdinaryFunction),
        {"Constructor", getUndefined(globalEnv), nullptr, nullptr, nullptr, constructor, NAPIDefault, nullptr}};
    ASSERT_EQ(napi_define_properties(globalEnv, exports, 2, desc), NAPIOK);

    NAPIValue result;

    ASSERT_EQ(
        NAPIRunScriptWithSourceUrl(
            globalEnv,
            "(()=>{\"use strict\";globalThis.assert.ok(globalThis.addon.OrdinaryFunction()),globalThis.assert.ok(new "
            "globalThis.addon.Constructor(globalThis.addon.Constructor)instanceof globalThis.addon.Constructor)})();",
            "https://www.didi.com/test_new_target.js", &result),
        NAPIOK);
}