#include <napi/js_native_api.h>

#define RETURN_STATUS_IF_FALSE(condition, status)                                                                      \
    if (!(condition))                                                                                                  \
    {                                                                                                                  \
        return status;                                                                                                 \
    }

#define CHECK_ARG(arg) RETURN_STATUS_IF_FALSE(arg, NAPIExceptionInvalidArg)

#define CHECK_NAPI(expr, exprStatus)                                                                                   \
    {                                                                                                                  \
        NAPI##exprStatus##Status status = expr;                                                                        \
        if (status != NAPI##exprStatus##OK)                                                                            \
        {                                                                                                              \
            return (NAPIExceptionStatus)status;                                                                        \
        }                                                                                                              \
    }

NAPIExceptionStatus napi_set_named_property(NAPIEnv env, NAPIValue object, const char *utf8name, NAPIValue value)
{
    CHECK_ARG(env);
    CHECK_ARG(object);
    CHECK_ARG(value);

    NAPIValue keyValue;
    CHECK_NAPI(napi_create_string_utf8(env, utf8name, &keyValue), Exception)
    CHECK_NAPI(napi_set_property(env, object, keyValue, value), Exception)

    return NAPIExceptionOK;
}

NAPIExceptionStatus napi_get_named_property(NAPIEnv env, NAPIValue object, const char *utf8name, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(object);
    CHECK_ARG(result);

    NAPIValue keyValue;
    CHECK_NAPI(napi_create_string_utf8(env, utf8name, &keyValue), Exception);
    CHECK_NAPI(napi_get_property(env, object, keyValue, result), Exception);

    return NAPIExceptionOK;
}

// NAPIBooleanExpected
NAPIExceptionStatus napi_strict_equals(NAPIEnv env, NAPIValue lhs, NAPIValue rhs, bool *result)
{
    CHECK_ARG(env);
    CHECK_ARG(lhs);
    CHECK_ARG(rhs);
    CHECK_ARG(result);

    NAPIValue global, objectCtor, is;
    CHECK_NAPI(napi_get_global(env, &global), Error);
    CHECK_NAPI(napi_get_named_property(env, global, "Object", &objectCtor), Exception);
    CHECK_NAPI(napi_get_named_property(env, objectCtor, "is", &is), Exception);
    NAPIValue argv[] = {lhs, rhs};
    NAPIValue resultValue;
    CHECK_NAPI(napi_call_function(env, objectCtor, is, 2, argv, &resultValue), Exception);
    NAPIValueType valueType;
    CHECK_NAPI(napi_typeof(env, resultValue, &valueType), Common);
    RETURN_STATUS_IF_FALSE(valueType == NAPIBoolean, NAPIExceptionBooleanExpected);
    CHECK_NAPI(napi_get_value_bool(env, resultValue, result), Error);

    return NAPIExceptionOK;
}

NAPIExceptionStatus NAPIParseUTF8JSONString(NAPIEnv env, const char *utf8String, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(utf8String);
    CHECK_ARG(result);

    NAPIValue stringValue;
    CHECK_NAPI(napi_create_string_utf8(env, utf8String, &stringValue), Exception);
    NAPIValue global;
    CHECK_NAPI(napi_get_global(env, &global), Error);
    NAPIValue json, parse;
    CHECK_NAPI(napi_get_named_property(env, global, "JSON", &json), Exception);
    CHECK_NAPI(napi_get_named_property(env, json, "parse", &parse), Exception);
    CHECK_NAPI(napi_call_function(env, json, parse, 1, &stringValue, result), Exception);

    return NAPIExceptionOK;
}
