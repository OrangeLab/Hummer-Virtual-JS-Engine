#include <gtest/gtest.h>
#include <napi/js_native_api.h>
#include <napi_assert.h>

EXTERN_C_START

// these tests validate the handle scope functions in the normal
// flow.  Forcing gc behavior to fully validate they are doing
// the right right thing would be quite hard so we keep it
// simple for now.

static NAPIValue NewScope(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIHandleScope scope;
    NAPIValue output = NULL;

    NAPI_CALL(env, napi_open_handle_scope(env, &scope));
    NAPI_CALL(env, napi_create_object(env, &output));
    NAPI_CALL(env, napi_close_handle_scope(env, scope));
    return NULL;
}

static NAPIValue NewScopeEscape(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIEscapableHandleScope scope;
    NAPIValue output = NULL;
    NAPIValue escapee = NULL;

    NAPI_CALL(env, napi_open_escapable_handle_scope(env, &scope));
    NAPI_CALL(env, napi_create_object(env, &output));
    NAPI_CALL(env, napi_escape_handle(env, scope, output, &escapee));
    NAPI_CALL(env, napi_close_escapable_handle_scope(env, scope));
    return escapee;
}

static NAPIValue NewScopeEscapeTwice(NAPIEnv env, NAPICallbackInfo /*info*/) {
    NAPIEscapableHandleScope scope;
    NAPIValue output = NULL;
    NAPIValue escapee = NULL;
    NAPIStatus status;

    NAPI_CALL(env, napi_open_escapable_handle_scope(env, &scope));
    NAPI_CALL(env, napi_create_object(env, &output));
    NAPI_CALL(env, napi_escape_handle(env, scope, output, &escapee));
    status = napi_escape_handle(env, scope, output, &escapee);
    NAPI_ASSERT(env, status == NAPIEscapeCalledTwice, "Escaping twice fails");
    NAPI_CALL(env, napi_close_escapable_handle_scope(env, scope));
    return NULL;
}

static NAPIValue NewScopeWithException(NAPIEnv env, NAPICallbackInfo info) {
    NAPIHandleScope scope;
    size_t argc;
    NAPIValue exception_function;
    NAPIStatus status;
    NAPIValue output = NULL;

    NAPI_CALL(env, napi_open_handle_scope(env, &scope));
    NAPI_CALL(env, napi_create_object(env, &output));

    argc = 1;
    NAPI_CALL(env, napi_get_cb_info(
            env, info, &argc, &exception_function, NULL, NULL));

    status = napi_call_function(
            env, output, exception_function, 0, NULL, NULL);
    NAPI_ASSERT(env, status == NAPIPendingException,
                "Function should have thrown.");

    // Closing a handle scope should still work while an exception is pending.
    NAPI_CALL(env, napi_close_handle_scope(env, scope));
    return NULL;
}

EXTERN_C_END

TEST(TestHandleScope, Test) {
    NAPIEnv env = nullptr;
    ASSERT_EQ(NAPICreateEnv(&env), NAPIOK);

    NAPIValue global = nullptr;
    ASSERT_EQ(napi_get_global(env, &global), NAPIOK);

    NAPIValue exports;
    ASSERT_EQ(napi_create_object(env, &exports), NAPIOK);
    ASSERT_EQ(napi_set_named_property(env, global, "exports", exports), NAPIOK);

    NAPIPropertyDescriptor properties[] = {
            DECLARE_NAPI_PROPERTY("NewScope", NewScope),
            DECLARE_NAPI_PROPERTY("NewScopeEscape", NewScopeEscape),
            DECLARE_NAPI_PROPERTY("NewScopeEscapeTwice", NewScopeEscapeTwice),
            DECLARE_NAPI_PROPERTY("NewScopeWithException", NewScopeWithException),
    };

    ASSERT_EQ(napi_define_properties(
            env, exports, sizeof(properties) / sizeof(*properties), properties), NAPIOK);

    EXPECT_EQ(initAssert(env, global), NAPIOK);

    NAPIValue result = nullptr;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(env, "(function () {\n"
                                              "    globalThis.exports.NewScope();\n"
                                              "\n"
                                              "    assert(globalThis.exports.NewScopeEscape() instanceof Object);\n"
                                              "\n"
                                              "    globalThis.exports.NewScopeEscapeTwice();\n"
                                              "\n"
                                              "    assert.throws(\n"
                                              "        () => {\n"
                                              "            globalThis.exports.NewScopeWithException(() => { throw new RangeError(); });\n"
                                              "        });\n"
                                              "})();",
                                         "https://n-api.com/test_handle_scope.js",
                                         &result), NAPIOK);
    NAPIValueType valueType;
    EXPECT_EQ(napi_typeof(env, result, &valueType), NAPIOK);
    EXPECT_EQ(valueType, NAPIUndefined);

    ASSERT_EQ(NAPIFreeEnv(env), NAPIOK);
}