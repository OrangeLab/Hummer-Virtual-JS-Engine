#include <common.h>
#include <js_native_api_test.h>

EXTERN_C_START

// these tests validate the handle scope functions in the normal
// flow.  Forcing gc behavior to fully validate they are doing
// the right right thing would be quite hard so we keep it
// simple for now.

static NAPIValue NewScope(NAPIEnv env, NAPICallbackInfo /*info*/)
{
    NAPIHandleScope scope;
    NAPIValue output;

    NAPI_CALL(env, napi_open_handle_scope(env, &scope));
    NAPI_CALL(env, napi_create_object(env, &output));
    NAPI_CALL(env, napi_close_handle_scope(env, scope));
    return nullptr;
}

static NAPIValue NewScopeEscape(NAPIEnv env, NAPICallbackInfo /*info*/)
{
    NAPIEscapableHandleScope scope;
    NAPIValue output;
    NAPIValue escapee;

    NAPI_CALL(env, napi_open_escapable_handle_scope(env, &scope));
    NAPI_CALL(env, napi_create_object(env, &output));
    NAPI_CALL(env, napi_escape_handle(env, scope, output, &escapee));
    NAPI_CALL(env, napi_close_escapable_handle_scope(env, scope));
    return escapee;
}

static NAPIValue NewScopeEscapeTwice(NAPIEnv env, NAPICallbackInfo /*info*/)
{
    NAPIEscapableHandleScope scope;
    NAPIValue output;
    NAPIValue escapee;
    NAPIStatus status;

    NAPI_CALL(env, napi_open_escapable_handle_scope(env, &scope));
    NAPI_CALL(env, napi_create_object(env, &output));
    NAPI_CALL(env, napi_escape_handle(env, scope, output, &escapee));
    status = napi_escape_handle(env, scope, output, &escapee);
    NAPI_ASSERT(env, status == NAPIEscapeCalledTwice, "Escaping twice fails");
    NAPI_CALL(env, napi_close_escapable_handle_scope(env, scope));
    return nullptr;
}

static NAPIValue NewScopeWithException(NAPIEnv env, NAPICallbackInfo info)
{
    NAPIHandleScope scope;
    size_t argc;
    NAPIValue exception_function;
    NAPIStatus status;
    NAPIValue output;

    NAPI_CALL(env, napi_open_handle_scope(env, &scope));
    NAPI_CALL(env, napi_create_object(env, &output));

    argc = 1;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &exception_function, nullptr, nullptr));

    status = napi_call_function(env, output, exception_function, 0, nullptr, nullptr);
    NAPI_ASSERT(env, status == NAPIPendingException, "Function should have thrown.");

    // Closing a handle scope should still work while an exception is pending.
    NAPI_CALL(env, napi_close_handle_scope(env, scope));
    return nullptr;
}

EXTERN_C_END

TEST_F(Test, TestHandleScope)
{
    NAPIPropertyDescriptor properties[] = {
        DECLARE_NAPI_PROPERTY("NewScope", NewScope),
        DECLARE_NAPI_PROPERTY("NewScopeEscape", NewScopeEscape),
        DECLARE_NAPI_PROPERTY("NewScopeEscapeTwice", NewScopeEscapeTwice),
        DECLARE_NAPI_PROPERTY("NewScopeWithException", NewScopeWithException),
    };

    ASSERT_EQ(napi_define_properties(globalEnv, exports, sizeof(properties) / sizeof(*properties), properties), NAPIOK);

    NAPIValue result;
    ASSERT_EQ(
        NAPIRunScript(
            globalEnv,
            "(()=>{\"use "
            "strict\";globalThis.addon.NewScope(),globalThis.assert.ok(globalThis.addon.NewScopeEscape()instanceof "
            "Object),globalThis.addon.NewScopeEscapeTwice(),globalThis.assert.throws((function(){globalThis.addon."
            "NewScopeWithException((function(){throw new RangeError}))}))})();",
            "https://www.napi.com/test_handle_scope.js", &result),
        NAPIOK);
}