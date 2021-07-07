#include <napi/js_native_api.h>

#define RETURN_STATUS_IF_FALSE(condition, status)                                                                      \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            return status;                                                                                             \
        }                                                                                                              \
    } while (0)

#define CHECK_ARG(arg) RETURN_STATUS_IF_FALSE(arg, NAPIInvalidArg)

#define CHECK_NAPI(expr)                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        NAPIStatus status = expr;                                                                                      \
        if (status != NAPIOK)                                                                                          \
        {                                                                                                              \
            return status;                                                                                             \
        }                                                                                                              \
    } while (0)

NAPIStatus napi_set_named_property(NAPIEnv env, NAPIValue object, const char *utf8name, NAPIValue value)
{
    CHECK_ARG(env);
    CHECK_ARG(object);
    CHECK_ARG(value);

    NAPIValue keyValue;
    CHECK_NAPI(napi_create_string_utf8(env, utf8name, &keyValue));
    CHECK_NAPI(napi_set_property(env, object, keyValue, value));

    return NAPIOK;
}

NAPIStatus napi_get_named_property(NAPIEnv env, NAPIValue object, const char *utf8name, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(object);
    CHECK_ARG(result);

    NAPIValue keyValue;
    CHECK_NAPI(napi_create_string_utf8(env, utf8name, &keyValue));
    CHECK_NAPI(napi_get_property(env, object, keyValue, result));

    return NAPIOK;
}

// NAPIBooleanExpected
NAPIStatus napi_strict_equals(NAPIEnv env, NAPIValue lhs, NAPIValue rhs, bool *result)
{
    CHECK_ARG(env);
    CHECK_ARG(lhs);
    CHECK_ARG(rhs);
    CHECK_ARG(result);

    NAPIValue global, objectCtor, is;
    CHECK_NAPI(napi_get_global(env, &global));
    CHECK_NAPI(napi_get_named_property(env, global, "Object", &objectCtor));
    CHECK_NAPI(napi_get_named_property(env, objectCtor, "is", &is));
    NAPIValue argv[] = {lhs, rhs};
    NAPIValue resultValue;
    CHECK_NAPI(napi_call_function(env, objectCtor, is, 2, argv, &resultValue));
    NAPIValueType valueType;
    CHECK_NAPI(napi_typeof(env, resultValue, &valueType));
    RETURN_STATUS_IF_FALSE(valueType == NAPIBoolean, NAPIBooleanExpected);
    CHECK_NAPI(napi_get_value_bool(env, resultValue, result));

    return NAPIOK;
}

NAPIStatus NAPIParseUTF8JSONString(NAPIEnv env, const char *utf8String, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(utf8String);
    CHECK_ARG(result);

    NAPIValue stringValue;
    CHECK_NAPI(napi_create_string_utf8(env, utf8String, &stringValue));
    NAPIValue global;
    CHECK_NAPI(napi_get_global(env, &global));
    NAPIValue json, parse;
    CHECK_NAPI(napi_get_named_property(env, global, "JSON", &json));
    CHECK_NAPI(napi_get_named_property(env, json, "parse", &parse));
    CHECK_NAPI(napi_call_function(env, json, parse, 1, &stringValue, result));

    return NAPIOK;
}
