#include <gtest/gtest.h>
#include <napi/js_native_api.h>
#include <napi_assert.h>

EXTERN_C_START

static NAPIValue Constructor(NAPIEnv env, NAPICallbackInfo info) {
    bool result;
    NAPIValue newTargetArg;
    NAPI_CALL(env, napi_get_new_target(env, info, &newTargetArg));
    size_t argc = 1;
    NAPIValue argv;
    NAPIValue thisArg;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &argv, &thisArg, NULL));
    NAPIValue undefined;
    NAPI_CALL(env, napi_get_undefined(env, &undefined));

    // new.target !== undefined because we should be called as a new expression
    NAPI_ASSERT(env, newTargetArg != NULL, "newTargetArg != NULL");
    NAPI_CALL(env, napi_strict_equals(env, newTargetArg, undefined, &result));
    NAPI_ASSERT(env, !result, "new.target !== undefined");

    // arguments[0] should be Constructor itself (test harness passed it)
    NAPI_CALL(env, napi_strict_equals(env, newTargetArg, argv, &result));
    NAPI_ASSERT(env, result, "new.target === Constructor");

    return thisArg;
}

static NAPIValue OrdinaryFunction(NAPIEnv env, NAPICallbackInfo info) {
    NAPIValue newTargetArg;
    NAPI_CALL(env, napi_get_new_target(env, info, &newTargetArg));

    NAPI_ASSERT(env, newTargetArg == NULL, "newTargetArg == NULL");

    NAPIValue _true;
    NAPI_CALL(env, napi_get_boolean(env, true, &_true));
    return _true;
}

EXTERN_C_END

TEST(TestNewTarget, Test) {
    NAPIEnv env = nullptr;
    ASSERT_EQ(NAPICreateEnv(&env), NAPIOK);

    NAPIValue global = nullptr;
    ASSERT_EQ(napi_get_global(env, &global), NAPIOK);

    NAPIValue exports;
    ASSERT_EQ(napi_create_object(env, &exports), NAPIOK);
    ASSERT_EQ(napi_set_named_property(env, global, "exports", exports), NAPIOK);

    NAPIValue constructor;
    ASSERT_EQ(napi_define_class(env, NULL, -1, Constructor, NULL, 0, NULL, &constructor), NAPIOK);

    // JavaScriptCore 无法 new method();
    const NAPIPropertyDescriptor desc[] = {
            DECLARE_NAPI_PROPERTY("OrdinaryFunction", OrdinaryFunction),
            {"Constructor", NULL, NULL, NULL, NULL, constructor, NAPIDefault, NULL}
    };
    ASSERT_EQ(napi_define_properties(env, exports, 2, desc), NAPIOK);

    EXPECT_EQ(initAssert(env, global), NAPIOK);

    NAPIValue result = nullptr;
    // class Class extends globalThis.exports.BaseClass 最终调用 BaseClass 构造函数的时候，是无法获得 Class 的构造函数的
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(env, "(function () {\n"
                                              "    assert(globalThis.exports.OrdinaryFunction());\n"
                                              "    assert(\n"
                                              "        new globalThis.exports.Constructor(globalThis.exports.Constructor) instanceof globalThis.exports.Constructor);\n"
                                              "})();",
                                         "https://n-api.com/test_new_target.js",
                                         &result), NAPIOK);
    NAPIValueType valueType;
    EXPECT_EQ(napi_typeof(env, result, &valueType), NAPIOK);
    EXPECT_EQ(valueType, NAPIUndefined);

    ASSERT_EQ(NAPIFreeEnv(env), NAPIOK);
}