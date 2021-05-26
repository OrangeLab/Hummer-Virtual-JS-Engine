#include <common.h>
#include <js_native_api_test.h>

EXTERN_C_START

static bool exceptionWasPending = false;

static NAPIValue returnException(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPIValue global;
    NAPI_CALL(env, napi_get_global(env, &global));

    NAPIValue result;
    NAPIStatus status = napi_call_function(env, global, args[0], 0, 0, &result);
    if (status == NAPIPendingException)
    {
        NAPIValue ex;
        NAPI_CALL(env, napi_get_and_clear_last_exception(env, &ex));
        return ex;
    }

    return nullptr;
}

static NAPIValue allowException(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPIValue global;
    NAPI_CALL(env, napi_get_global(env, &global));

    NAPIValue result;
    napi_call_function(env, global, args[0], 0, 0, &result);
    // Ignore status and check napi_is_exception_pending() instead.

    NAPI_CALL(env, napi_is_exception_pending(env, &exceptionWasPending));
    return nullptr;
}

static NAPIValue wasPending(NAPIEnv env, NAPICallbackInfo /*info*/)
{
    NAPIValue result;
    NAPI_CALL(env, napi_get_boolean(env, exceptionWasPending, &result));

    return result;
}

static void finalizer(NAPIEnv env, void * /*data*/, void * /*hint*/)
{
    NAPI_CALL_RETURN_VOID(env, napi_throw_error(env, nullptr, "Error during Finalize"));
}

static NAPIValue createExternal(NAPIEnv env, NAPICallbackInfo /*info*/)
{
    NAPIValue external;

    NAPI_CALL(env, napi_create_external(env, nullptr, finalizer, nullptr, &external));

    return external;
}

static NAPIValue requireFunction(NAPIEnv env, NAPICallbackInfo /*info*/)
{
    NAPIValue exports;
    NAPI_CALL(env, napi_create_object(env, &exports));

    NAPIPropertyDescriptor descriptors[] = {
        DECLARE_NAPI_PROPERTY("returnException", returnException),
        DECLARE_NAPI_PROPERTY("allowException", allowException),
        DECLARE_NAPI_PROPERTY("wasPending", wasPending),
        DECLARE_NAPI_PROPERTY("createExternal", createExternal),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors));

    NAPIValue error, code, message;
    NAPI_CALL(env, napi_create_string_utf8(env, "Error during Init", NAPI_AUTO_LENGTH, &message));
    NAPI_CALL(env, napi_create_string_utf8(env, "", NAPI_AUTO_LENGTH, &code));
    NAPI_CALL(env, napi_create_error(env, code, message, &error));
    NAPI_CALL(env, napi_set_named_property(env, error, "binding", exports));
    NAPI_CALL(env, napi_throw(env, error));

    return exports;
}

EXTERN_C_END

TEST(TestException, RequireThrowException)
{
    NAPIValue global;
    ASSERT_EQ(napi_get_global(globalEnv, &global), NAPIOK);

    NAPIValue requireValue;
    ASSERT_EQ(napi_create_function(globalEnv, nullptr, -1, requireFunction, nullptr, &requireValue), NAPIOK);
    ASSERT_EQ(napi_set_named_property(globalEnv, global, "require", requireValue), NAPIOK);

    NAPIValue result;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(
                  globalEnv,
                  "(()=>{\"use strict\";var r=new Error(\"Some error\"),t=function(){var "
                  "r;try{globalThis.require()}catch(t){r=t}if(r)return globalThis.assert.strictEqual(r.message,\"Error "
                  "during Init\"),r.binding}(),n=function(){throw "
                  "r},e=t.returnException(n);globalThis.assert.strictEqual(e,r),globalThis.assert.throws((function(){t."
                  "allowException(n)}),(function(t){return t===r}));var "
                  "i=t.wasPending();globalThis.assert.strictEqual(i,!0,`Exception not pending as expected, "
                  ".wasPending() returned ${i}`)})();",
                  "https://www.didi.com/test_exception_test.js", &result),
              NAPIOK);
}