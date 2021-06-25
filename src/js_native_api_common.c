#include <napi/js_native_api.h>

#define RETURN_STATUS_IF_FALSE(condition, status)                                                                      \
    {                                                                                                                  \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            return status;                                                                                             \
        }                                                                                                              \
    }

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

NAPIStatus napi_create_object(NAPIEnv env, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    NAPIValue globalValue;
    CHECK_NAPI(napi_get_global(env, &globalValue));
    NAPIValue objectConstructorValue;
    CHECK_NAPI(napi_get_named_property(env, globalValue, "Object", &objectConstructorValue));
    CHECK_NAPI(napi_new_instance(env, objectConstructorValue, 0, NULL, result));

    return NAPIOK;
}

NAPIStatus napi_create_array(NAPIEnv env, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    NAPIValue globalValue;
    CHECK_NAPI(napi_get_global(env, &globalValue));
    NAPIValue objectConstructorValue;
    CHECK_NAPI(napi_get_named_property(env, globalValue, "Array", &objectConstructorValue));
    CHECK_NAPI(napi_new_instance(env, objectConstructorValue, 0, NULL, result));

    return NAPIOK;
}

NAPIStatus napi_create_array_with_length(NAPIEnv env, size_t length, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    NAPIValue lengthValue;
    CHECK_NAPI(napi_create_uint32(env, length, &lengthValue));
    NAPIValue globalValue;
    CHECK_NAPI(napi_get_global(env, &globalValue));
    NAPIValue objectConstructorValue;
    CHECK_NAPI(napi_get_named_property(env, globalValue, "Array", &objectConstructorValue));
    NAPIValue arg[] = {lengthValue};
    CHECK_NAPI(napi_new_instance(env, objectConstructorValue, 1, arg, result));

    return NAPIOK;
}

// NAPIStringExpected
static NAPIStatus setErrorCode(NAPIEnv env, NAPIValue error, NAPIValue code)
{
    CHECK_ARG(env);
    CHECK_ARG(error);

    if (code)
    {
        NAPIValueType valueType;
        CHECK_NAPI(napi_typeof(env, code, &valueType));
        RETURN_STATUS_IF_FALSE(valueType == NAPIString, NAPIStringExpected);
        CHECK_NAPI(napi_set_named_property(env, error, "code", code));
    }

    return NAPIOK;
}

// NAPIStringExpected
NAPIStatus napi_create_error(NAPIEnv env, NAPIValue code, NAPIValue msg, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(msg);
    CHECK_ARG(result);

    NAPIValue global;
    CHECK_NAPI(napi_get_global(env, &global));
    NAPIValue errorConstructor;
    CHECK_NAPI(napi_get_named_property(env, global, "Error", &errorConstructor));
    CHECK_NAPI(napi_new_instance(env, errorConstructor, 1, &msg, result));
    CHECK_NAPI(setErrorCode(env, *result, code));

    return NAPIOK;
}

NAPIStatus napi_create_type_error(NAPIEnv env, NAPIValue code, NAPIValue msg, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(msg);
    CHECK_ARG(result);

    NAPIValue global;
    CHECK_NAPI(napi_get_global(env, &global));
    NAPIValue errorConstructor;
    CHECK_NAPI(napi_get_named_property(env, global, "TypeError", &errorConstructor));
    CHECK_NAPI(napi_new_instance(env, errorConstructor, 1, &msg, result));
    CHECK_NAPI(setErrorCode(env, *result, code));

    return NAPIOK;
}

NAPIStatus napi_create_range_error(NAPIEnv env, NAPIValue code, NAPIValue msg, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(msg);
    CHECK_ARG(result);

    NAPIValue global;
    CHECK_NAPI(napi_get_global(env, &global));
    NAPIValue errorConstructor;
    CHECK_NAPI(napi_get_named_property(env, global, "RangeError", &errorConstructor));
    CHECK_NAPI(napi_new_instance(env, errorConstructor, 1, &msg, result));
    CHECK_NAPI(setErrorCode(env, *result, code));

    return NAPIOK;
}

NAPIStatus napi_coerce_to_object(NAPIEnv env, NAPIValue value, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(value);
    CHECK_ARG(result);

    NAPIValue global, objectCtor;
    CHECK_NAPI(napi_get_global(env, &global));
    CHECK_NAPI(napi_get_named_property(env, global, "Object", &objectCtor));
    CHECK_NAPI(napi_new_instance(env, objectCtor, 1, &value, result));

    return NAPIOK;
}

// NAPIStatus napi_get_prototype(NAPIEnv env, NAPIValue object, NAPIValue *result)
//{
//     CHECK_ARG(env);
//     CHECK_ARG(object);
//     CHECK_ARG(result);
//
//     NAPIValue global, objectCtor, getPrototypeOf;
//     CHECK_NAPI(napi_get_global(env, &global));
//     CHECK_NAPI(napi_get_named_property(env, global, "Object", &objectCtor));
//     CHECK_NAPI(napi_get_named_property(env, objectCtor, "getPrototypeOf", &getPrototypeOf));
//     CHECK_NAPI(napi_call_function(env, objectCtor, getPrototypeOf, 1, &object, result));
//
//     return NAPIOK;
// }

NAPIStatus napi_set_named_property(NAPIEnv env, NAPIValue object, const char *utf8name, NAPIValue value)
{
    CHECK_ARG(env);
    CHECK_ARG(object);
    CHECK_ARG(value);

    NAPIValue keyValue;
    CHECK_NAPI(napi_create_string_utf8(env, utf8name, -1, &keyValue));
    CHECK_NAPI(napi_set_property(env, object, keyValue, value));

    return NAPIOK;
}

NAPIStatus napi_get_named_property(NAPIEnv env, NAPIValue object, const char *utf8name, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(object);
    CHECK_ARG(result);

    NAPIValue keyValue;
    CHECK_NAPI(napi_create_string_utf8(env, utf8name, -1, &keyValue));
    CHECK_NAPI(napi_get_property(env, object, keyValue, result));

    return NAPIOK;
}

NAPIStatus napi_set_element(NAPIEnv env, NAPIValue object, uint32_t index, NAPIValue value)
{
    CHECK_ARG(env);
    CHECK_ARG(object);
    CHECK_ARG(value);

    NAPIValue indexValue;
    CHECK_NAPI(napi_create_uint32(env, index, &indexValue));
    CHECK_NAPI(napi_set_property(env, object, indexValue, value));

    return NAPIOK;
}

NAPIStatus napi_has_element(NAPIEnv env, NAPIValue object, uint32_t index, bool *result)
{
    CHECK_ARG(env);
    CHECK_ARG(object);
    CHECK_ARG(result);

    NAPIValue indexValue;
    CHECK_NAPI(napi_create_uint32(env, index, &indexValue));
    CHECK_NAPI(napi_has_property(env, object, indexValue, result));

    return NAPIOK;
}

NAPIStatus napi_get_element(NAPIEnv env, NAPIValue object, uint32_t index, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(object);
    CHECK_ARG(result);

    NAPIValue indexValue;
    CHECK_NAPI(napi_create_uint32(env, index, &indexValue));
    CHECK_NAPI(napi_get_property(env, object, indexValue, result));

    return NAPIOK;
}

NAPIStatus napi_delete_element(NAPIEnv env, NAPIValue object, uint32_t index, bool *result)
{
    CHECK_ARG(env);
    CHECK_ARG(object);
    CHECK_ARG(result);

    NAPIValue indexValue;
    CHECK_NAPI(napi_create_uint32(env, index, &indexValue));
    CHECK_NAPI(napi_delete_property(env, object, indexValue, result));

    return NAPIOK;
}

NAPIStatus napi_define_properties(NAPIEnv env, NAPIValue object, size_t propertyCount,
                                  const NAPIPropertyDescriptor *properties)
{
    CHECK_ARG(env);
    CHECK_ARG(object);
    if (propertyCount > 0)
    {
        CHECK_ARG(properties);
    }
    for (size_t i = 0; i < propertyCount; i++)
    {
        const NAPIPropertyDescriptor *p = properties + i;
        NAPIValue descriptor;
        CHECK_NAPI(napi_create_object(env, &descriptor));

        NAPIValue configurable;
        CHECK_NAPI(napi_get_boolean(env, (p->attributes & NAPIConfigurable), &configurable));
        CHECK_NAPI(napi_set_named_property(env, descriptor, "configurable", configurable));

        NAPIValue enumerable;
        CHECK_NAPI(napi_get_boolean(env, (p->attributes & NAPIEnumerable), &enumerable));
        CHECK_NAPI(napi_set_named_property(env, descriptor, "enumerable", enumerable));

        if (p->getter || p->setter)
        {
            if (p->getter)
            {
                NAPIValue getter;
                CHECK_NAPI(napi_create_function(env, p->utf8name, -1, p->getter, p->data, &getter));
                CHECK_NAPI(napi_set_named_property(env, descriptor, "get", getter));
            }
            if (p->setter)
            {
                NAPIValue setter;
                CHECK_NAPI(napi_create_function(env, p->utf8name, -1, p->setter, p->data, &setter));
                CHECK_NAPI(napi_set_named_property(env, descriptor, "set", setter));
            }
        }
        else
        {
            NAPIValue value;
            if (p->method)
            {
                CHECK_NAPI(napi_create_function(env, p->utf8name, -1, p->method, p->data, &value));
            }
            else if (p->value)
            {
                value = p->value;
            }
            else
            {
                CHECK_NAPI(napi_get_undefined(env, &value));
            }
            NAPIValue writable;
            CHECK_NAPI(napi_get_boolean(env, (p->attributes & NAPIWritable), &writable));
            CHECK_NAPI(napi_set_named_property(env, descriptor, "writable", writable));
            CHECK_NAPI(napi_set_named_property(env, descriptor, "value", value));
        }
        NAPIValue propertyName;
        if (p->name)
        {
            propertyName = p->name;
        }
        else
        {
            CHECK_NAPI(napi_create_string_utf8(env, p->utf8name, -1, &propertyName));
        }
        NAPIValue global, objectCtor, function;
        CHECK_NAPI(napi_get_global(env, &global));
        CHECK_NAPI(napi_get_named_property(env, global, "Object", &objectCtor));
        CHECK_NAPI(napi_get_named_property(env, objectCtor, "defineProperty", &function));

        NAPIValue args[] = {object, propertyName, descriptor};
        CHECK_NAPI(napi_call_function(env, objectCtor, function, 3, args, NULL));
    }

    return NAPIOK;
}

// NAPIGenericFailure/NAPIArrayExpected
NAPIStatus napi_get_array_length(NAPIEnv env, NAPIValue value, uint32_t *result)
{
    CHECK_ARG(env);
    CHECK_ARG(value);
    CHECK_ARG(result);

    bool isArray;
    CHECK_NAPI(napi_is_array(env, value, &isArray));
    RETURN_STATUS_IF_FALSE(isArray, NAPIArrayExpected);

    NAPIValue lengthValue;
    CHECK_NAPI(napi_get_named_property(env, value, "length", &lengthValue));
    NAPIValueType valueType;
    CHECK_NAPI(napi_typeof(env, lengthValue, &valueType));
    RETURN_STATUS_IF_FALSE(valueType == NAPINumber, NAPIGenericFailure);
    CHECK_NAPI(napi_get_value_uint32(env, lengthValue, result));

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

NAPIStatus napi_define_class(NAPIEnv env, const char *utf8name, size_t length, NAPICallback constructor, void *data,
                             size_t propertyCount, const NAPIPropertyDescriptor *properties, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(constructor);
    CHECK_ARG(result);

    CHECK_NAPI(NAPIDefineClass(env, utf8name, length, constructor, data, result));
    if (propertyCount > 0)
    {
        CHECK_ARG(properties);
        // 大概率事件
        NAPIValue prototype;
        CHECK_NAPI(napi_get_named_property(env, *result, "prototype", &prototype));
        for (size_t i = 0; i < propertyCount; ++i)
        {
            CHECK_NAPI(napi_define_properties(env, properties[i].attributes & NAPIStatic ? *result : prototype, 1,
                                              &properties[i]));
        }
    }

    return NAPIOK;
}

NAPIStatus napi_throw_error(NAPIEnv env, const char *code, const char *msg)
{
    CHECK_ARG(env);
    CHECK_ARG(msg);

    NAPIValue error, codeValue = NULL, message;
    if (code)
    {
        CHECK_NAPI(napi_create_string_utf8(env, code, -1, &codeValue));
    }
    CHECK_NAPI(napi_create_string_utf8(env, msg, -1, &message));
    CHECK_NAPI(napi_create_error(env, codeValue, message, &error));
    CHECK_NAPI(napi_throw(env, error));

    return NAPIOK;
}

NAPIStatus napi_throw_type_error(NAPIEnv env, const char *code, const char *msg)
{
    CHECK_ARG(env);
    CHECK_ARG(msg);

    NAPIValue error, codeValue = NULL, message;
    if (code)
    {
        CHECK_NAPI(napi_create_string_utf8(env, code, -1, &codeValue));
    }
    CHECK_NAPI(napi_create_string_utf8(env, msg, -1, &message));
    CHECK_NAPI(napi_create_type_error(env, codeValue, message, &error));
    CHECK_NAPI(napi_throw(env, error));

    return NAPIOK;
}

NAPIStatus napi_throw_range_error(NAPIEnv env, const char *code, const char *msg)
{
    CHECK_ARG(env);
    CHECK_ARG(msg);

    NAPIValue error, codeValue = NULL, message;
    if (code)
    {
        CHECK_NAPI(napi_create_string_utf8(env, code, -1, &codeValue));
    }
    CHECK_NAPI(napi_create_string_utf8(env, msg, -1, &message));
    CHECK_NAPI(napi_create_range_error(env, codeValue, message, &error));
    CHECK_NAPI(napi_throw(env, error));

    return NAPIOK;
}

NAPIStatus napi_is_error(NAPIEnv env, NAPIValue value, bool *result)
{
    CHECK_ARG(env);
    CHECK_ARG(value);
    CHECK_ARG(result);

    NAPIValue global;
    CHECK_NAPI(napi_get_global(env, &global));
    NAPIValue error;
    CHECK_NAPI(napi_get_named_property(env, global, "Error", &error));
    CHECK_NAPI(napi_instanceof(env, value, error, result));

    return NAPIOK;
}

NAPIStatus NAPIParseUTF8JSONString(NAPIEnv env, const char *utf8String, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(utf8String);
    CHECK_ARG(result);

    NAPIValue stringValue;
    CHECK_NAPI(napi_create_string_utf8(env, utf8String, -1, &stringValue));
    NAPIValue global;
    CHECK_NAPI(napi_get_global(env, &global));
    NAPIValue json, parse;
    CHECK_NAPI(napi_get_named_property(env, global, "JSON", &json));
    CHECK_NAPI(napi_get_named_property(env, json, "parse", &parse));
    CHECK_NAPI(napi_call_function(env, json, parse, 1, &stringValue, result));

    return NAPIOK;
}
