#include <cassert>
#include <test.h>

NAPIEnv globalEnv = nullptr;

bool finalizeIsCalled = false;

EXTERN_C_START

static NAPIValue jsAssert(NAPIEnv env, NAPICallbackInfo callbackInfo)
{
    size_t argc = 1;
    NAPIValue argv[1];
    assert(napi_get_cb_info(env, callbackInfo, &argc, argv, nullptr, nullptr) == NAPIOK);
    NAPIValueType valueType;
    assert(napi_typeof(env, argv[0], &valueType) == NAPIOK);
    assert(valueType == NAPIBoolean);
    bool assertValue;
    assert(napi_get_value_bool(env, argv[0], &assertValue) == NAPIOK);
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
        ASSERT_EQ(NAPICreateEnv(&globalEnv), NAPIOK);
        NAPIHandleScope handleScope;
        ASSERT_EQ(napi_open_handle_scope(globalEnv, &handleScope), NAPIOK);
        NAPIValue assertValue;
        ASSERT_EQ(napi_create_function(globalEnv, "assert", -1, jsAssert, nullptr, &assertValue), NAPIOK);
        NAPIValue globalValue;
        ASSERT_EQ(napi_get_global(globalEnv, &globalValue), NAPIOK);
        NAPIValue stringValue;
        ASSERT_EQ(napi_create_string_utf8(globalEnv, "assert", -1, &stringValue), NAPIOK);
        ASSERT_EQ(napi_set_property(globalEnv, globalValue, stringValue, assertValue), NAPIOK);
        ASSERT_EQ(napi_close_handle_scope(globalEnv, handleScope), NAPIOK);
    }

    // Override this to define how to tear down the environment.
    void TearDown() override
    {
        ASSERT_EQ(NAPIFreeEnv(globalEnv), NAPIOK);
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
    ASSERT_EQ(napi_open_handle_scope(globalEnv, &handleScope), NAPIOK);
    NAPIValue global;
    ASSERT_EQ(napi_get_global(globalEnv, &global), NAPIOK);
    ASSERT_EQ(napi_create_object(globalEnv, &addonValue), NAPIOK);
    NAPIValue stringValue;
    ASSERT_EQ(napi_create_string_utf8(globalEnv, "addon", -1, &stringValue), NAPIOK);
    ASSERT_EQ(napi_set_property(globalEnv, global, stringValue, addonValue), NAPIOK);
}

void ::Test::TearDown()
{
    ::testing::Test::TearDown();
    ASSERT_EQ(napi_close_handle_scope(globalEnv, handleScope), NAPIOK);
}
