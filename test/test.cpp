#include <cassert>
#include <napi/js_native_api_debugger.h>
#include <test.h>

NAPIEnv globalEnv = nullptr;

NAPIRuntime globalRuntime = nullptr;

bool finalizeIsCalled = false;

EXTERN_C_START

static NAPIValue jsAssert(NAPIEnv env, NAPICallbackInfo callbackInfo)
{
    size_t argc = 1;
    NAPIValue argv[1];
    assert(napi_get_cb_info(env, callbackInfo, &argc, argv, nullptr, nullptr) == NAPICommonOK);
    NAPIValueType valueType;
    assert(napi_typeof(env, argv[0], &valueType) == NAPICommonOK);
    assert(valueType == NAPIBoolean);
    bool assertValue;
    assert(napi_get_value_bool(env, argv[0], &assertValue) == NAPIErrorOK);
    assert(assertValue);

    return nullptr;
}

EXTERN_C_END

namespace
{
class NAPIEnvironment : public ::testing::Environment
{
  public:
    // Override this to define how to set up the environment.
    void SetUp() override
    {
        ASSERT_EQ(NAPICreateRuntime(&globalRuntime), NAPIErrorOK);
        ASSERT_EQ(NAPICreateEnv(&globalEnv, globalRuntime), NAPIErrorOK);
        NAPIHandleScope handleScope;

        NAPIEnableDebugger(globalEnv, "test", true);

        ASSERT_EQ(napi_open_handle_scope(globalEnv, &handleScope), NAPIErrorOK);
        NAPIValue assertValue;
        ASSERT_EQ(napi_create_function(globalEnv, "assert", jsAssert, nullptr, &assertValue), NAPIExceptionOK);
        NAPIValue globalValue;
        ASSERT_EQ(napi_get_global(globalEnv, &globalValue), NAPIErrorOK);
        NAPIValue stringValue;
        ASSERT_EQ(napi_create_string_utf8(globalEnv, "assert", &stringValue), NAPIExceptionOK);
        ASSERT_EQ(napi_set_property(globalEnv, globalValue, stringValue, assertValue), NAPIExceptionOK);
        napi_close_handle_scope(globalEnv, handleScope);
    }

    // Override this to define how to tear down the environment.
    void TearDown() override
    {
        NAPIFreeEnv(globalEnv);
        NAPIFreeRuntime(globalRuntime);
        ASSERT_TRUE(finalizeIsCalled);
    }
};
} // namespace

int main(int argc, char **argv)
{
    ::testing::AddGlobalTestEnvironment(new NAPIEnvironment());
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}

void ::Test::SetUp()
{
    ::testing::Test::SetUp();
    ASSERT_EQ(napi_open_handle_scope(globalEnv, &handleScope), NAPIErrorOK);
    NAPIValue global;
    ASSERT_EQ(napi_get_global(globalEnv, &global), NAPIErrorOK);
    NAPIValue objectCtor;
    ASSERT_EQ(napi_get_named_property(globalEnv, global, "Object", &objectCtor), NAPIExceptionOK);
    ASSERT_EQ(napi_new_instance(globalEnv, objectCtor, 0, nullptr, &addonValue), NAPIExceptionOK);
    NAPIValue stringValue;
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "addon", &stringValue), NAPIExceptionOK);
    ASSERT_EQ(napi_set_property(globalEnv, global, stringValue, addonValue), NAPIExceptionOK);
}

void ::Test::TearDown()
{
    ::testing::Test::TearDown();
    napi_close_handle_scope(globalEnv, handleScope);
}
