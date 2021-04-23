#include <gtest/gtest.h>
#include <napi/js_native_api.h>
#include <common.h>

EXTERN_C_START

static NAPIValue add(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 2;
    NAPIValue args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 2, "Wrong number of arguments");

    NAPIValueType valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPIValueType valuetype1;
    NAPI_CALL(env, napi_typeof(env, args[1], &valuetype1));

    NAPI_ASSERT(env, valuetype0 == NAPINumber && valuetype1 == NAPINumber,
                "Wrong argument type. Numbers expected.");

    double value0;
    NAPI_CALL(env, napi_get_value_double(env, args[0], &value0));

    double value1;
    NAPI_CALL(env, napi_get_value_double(env, args[1], &value1));

    NAPIValue sum;
    NAPI_CALL(env, napi_create_double(env, value0 + value1, &sum));

    return sum;
}

EXTERN_C_END

TEST(FunctionArguments, add) {
    NAPIEnv env = nullptr;
    ASSERT_EQ(NAPICreateEnv(&env), NAPIOK);

    NAPIValue global = nullptr;
    ASSERT_EQ(napi_get_global(env, &global), NAPIOK);

    EXPECT_EQ(initAssert(env, global), NAPIOK);

    NAPIPropertyDescriptor desc = DECLARE_NAPI_PROPERTY("add", add);
    EXPECT_EQ(napi_define_properties(env, global, 1, &desc), NAPIOK);

    NAPIValue result = nullptr;
    EXPECT_EQ(NAPIRunScriptWithSourceUrl(env, "(function () { assert(add(3, 5) === 8); })();",
                                         "https://n-api.com/2_function_arguments.js",
                                         &result), NAPIOK);
    NAPIValueType valueType;
    EXPECT_EQ(napi_typeof(env, result, &valueType), NAPIOK);
    EXPECT_EQ(valueType, NAPIUndefined);

    ASSERT_EQ(NAPIFreeEnv(env), NAPIOK);
}