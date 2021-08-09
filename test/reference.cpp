#include <test.h>

TEST_F(Test, Reference)
{
    NAPIValue nullValue, trueValue, falseValue, numberValue, stringValue;
    ASSERT_EQ(napi_get_null(globalEnv, &nullValue), NAPICommonOK);
    ASSERT_EQ(napi_get_boolean(globalEnv, true, &trueValue), NAPIErrorOK);
    ASSERT_EQ(napi_get_boolean(globalEnv, false, &falseValue), NAPIErrorOK);
    ASSERT_EQ(napi_create_double(globalEnv, 100, &numberValue), NAPIErrorOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, nullptr, &stringValue), NAPIExceptionOK);
    NAPIRef ref;
    ASSERT_EQ(napi_create_reference(globalEnv, nullValue, 0, &ref), NAPIExceptionOK);
    NAPIValue value;
    ASSERT_EQ(napi_get_reference_value(globalEnv, ref, &value), NAPIErrorOK);
    ASSERT_EQ(value, nullptr);
    ASSERT_EQ(napi_delete_reference(globalEnv, ref), NAPIExceptionOK);
    ASSERT_EQ(napi_create_reference(globalEnv, trueValue, 0, &ref), NAPIExceptionOK);
    ASSERT_EQ(napi_get_reference_value(globalEnv, ref, &value), NAPIErrorOK);
    ASSERT_EQ(value, nullptr);
    ASSERT_EQ(napi_delete_reference(globalEnv, ref), NAPIExceptionOK);
    ASSERT_EQ(napi_create_reference(globalEnv, falseValue, 0, &ref), NAPIExceptionOK);
    ASSERT_EQ(napi_get_reference_value(globalEnv, ref, &value), NAPIErrorOK);
    ASSERT_EQ(value, nullptr);
    ASSERT_EQ(napi_delete_reference(globalEnv, ref), NAPIExceptionOK);
    ASSERT_EQ(napi_create_reference(globalEnv, numberValue, 0, &ref), NAPIExceptionOK);
    ASSERT_EQ(napi_get_reference_value(globalEnv, ref, &value), NAPIErrorOK);
    ASSERT_EQ(value, nullptr);
    ASSERT_EQ(napi_delete_reference(globalEnv, ref), NAPIExceptionOK);
    ASSERT_EQ(napi_create_reference(globalEnv, stringValue, 0, &ref), NAPIExceptionOK);
    ASSERT_EQ(napi_get_reference_value(globalEnv, ref, &value), NAPIErrorOK);
    ASSERT_EQ(value, nullptr);
    NAPIExceptionStatus status = napi_reference_unref(globalEnv, ref, nullptr);
    ASSERT_NE(status, NAPIExceptionOK);
    if (status == NAPIExceptionPendingException)
    {
        NAPIValue exception;
        ASSERT_EQ(napi_get_and_clear_last_exception(globalEnv, &exception), NAPIErrorOK);
    }
    uint32_t referenceCount;
    ASSERT_EQ(napi_reference_ref(globalEnv, ref, &referenceCount), NAPIExceptionOK);
    ASSERT_EQ(referenceCount, static_cast<unsigned int>(1));
    ASSERT_EQ(napi_delete_reference(globalEnv, ref), NAPIExceptionOK);
}

TEST_F(Test, EscapableHandleScope)
{
    NAPIEscapableHandleScope escapableHandleScope;
    ASSERT_EQ(napi_open_escapable_handle_scope(globalEnv, &escapableHandleScope), NAPIErrorOK);

    NAPIValue stringValue, nullValue;
    ASSERT_EQ(napi_get_null(globalEnv, &nullValue), NAPICommonOK);
    ASSERT_EQ(napi_create_string_utf8(globalEnv, nullptr, &stringValue), NAPIExceptionOK);
    NAPIValue otherValue;
    ASSERT_EQ(napi_escape_handle(globalEnv, escapableHandleScope, stringValue, &otherValue), NAPIErrorOK);
    NAPIValue otherNullValue;
    ASSERT_EQ(napi_escape_handle(globalEnv, escapableHandleScope, nullValue, &otherNullValue),
              NAPIErrorEscapeCalledTwice);

    napi_close_escapable_handle_scope(globalEnv, escapableHandleScope);

    const char *string;
    ASSERT_EQ(NAPIGetValueStringUTF8(globalEnv, otherValue, &string), NAPIErrorOK);
    ASSERT_STREQ(string, "");
    ASSERT_EQ(NAPIFreeUTF8String(globalEnv, string), NAPICommonOK);
}