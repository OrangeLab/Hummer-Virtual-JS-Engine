#include <test.h>

TEST_F(Test, Reference)
{
    NAPIValue nullValue, trueValue, falseValue, numberValue, stringValue;
    ASSERT_EQ(napi_get_null(globalEnv, &nullValue), NAPIOK);
    ASSERT_EQ(napi_get_boolean(globalEnv, true, &trueValue), NAPIOK);
    ASSERT_EQ(napi_get_boolean(globalEnv, false, &falseValue), NAPIOK);
    ASSERT_EQ(napi_create_double(globalEnv, 100, &numberValue), NAPIOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, nullptr, -1, &stringValue), NAPIOK);
    NAPIRef ref;
    ASSERT_EQ(napi_create_reference(globalEnv, nullValue, 0, &ref), NAPIOK);
    NAPIValue value;
    ASSERT_EQ(napi_get_reference_value(globalEnv, ref, &value), NAPIOK);
    ASSERT_EQ(value, nullptr);
    ASSERT_EQ(napi_delete_reference(globalEnv, ref), NAPIOK);
    ASSERT_EQ(napi_create_reference(globalEnv, trueValue, 0, &ref), NAPIOK);
    ASSERT_EQ(napi_get_reference_value(globalEnv, ref, &value), NAPIOK);
    ASSERT_EQ(value, nullptr);
    ASSERT_EQ(napi_delete_reference(globalEnv, ref), NAPIOK);
    ASSERT_EQ(napi_create_reference(globalEnv, falseValue, 0, &ref), NAPIOK);
    ASSERT_EQ(napi_get_reference_value(globalEnv, ref, &value), NAPIOK);
    ASSERT_EQ(value, nullptr);
    ASSERT_EQ(napi_delete_reference(globalEnv, ref), NAPIOK);
    ASSERT_EQ(napi_create_reference(globalEnv, numberValue, 0, &ref), NAPIOK);
    ASSERT_EQ(napi_get_reference_value(globalEnv, ref, &value), NAPIOK);
    ASSERT_EQ(value, nullptr);
    ASSERT_EQ(napi_delete_reference(globalEnv, ref), NAPIOK);
    ASSERT_EQ(napi_create_reference(globalEnv, stringValue, 0, &ref), NAPIOK);
    ASSERT_EQ(napi_get_reference_value(globalEnv, ref, &value), NAPIOK);
    ASSERT_EQ(value, nullptr);
    NAPIStatus status = napi_reference_unref(globalEnv, ref, nullptr);
    ASSERT_NE(status, NAPIOK);
    if (status == NAPIPendingException)
    {
        NAPIValue exception;
        ASSERT_EQ(napi_get_and_clear_last_exception(globalEnv, &exception), NAPIOK);
    }
    uint32_t referenceCount;
    ASSERT_EQ(napi_reference_ref(globalEnv, ref, &referenceCount), NAPIOK);
    ASSERT_EQ(referenceCount, static_cast<unsigned int>(1));
    ASSERT_EQ(napi_delete_reference(globalEnv, ref), NAPIOK);
}

TEST_F(Test, EscapableHandleScope)
{
    NAPIEscapableHandleScope escapableHandleScope;
    ASSERT_EQ(napi_open_escapable_handle_scope(globalEnv, &escapableHandleScope), NAPIOK);

    NAPIValue stringValue, nullValue;
    ASSERT_EQ(napi_get_null(globalEnv, &nullValue), NAPIOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, nullptr, -1, &stringValue), NAPIOK);
    NAPIValue otherValue;
    ASSERT_EQ(napi_escape_handle(globalEnv, escapableHandleScope, stringValue, &otherValue), NAPIOK);
    NAPIValue otherNullValue;
    ASSERT_EQ(napi_escape_handle(globalEnv, escapableHandleScope, nullValue, &otherNullValue), NAPIEscapeCalledTwice);

    ASSERT_EQ(napi_close_escapable_handle_scope(globalEnv, escapableHandleScope), NAPIOK);

    const char *string;
    ASSERT_EQ(NAPIGetValueStringUTF8(globalEnv, otherValue, &string), NAPIOK);
    ASSERT_STREQ(string, "");
    ASSERT_EQ(NAPIFreeUTF8String(globalEnv, string), NAPIOK);
}