//
//  js_native_api_jsc.c
//  NAPI
//
//  Created by 唐佳诚 on 2021/2/23.
//


// 如果一个函数返回的 napi_status 为 napi_ok，则没有异常被挂起，并且不需要做任何处理
// 如果 napi_status 是除了 napi_ok 或者 napi_pending_exception，为了尝试恢复并继续执行，而不是直接返回，必须调用 napi_is_exception_pending 来确定是否存在挂起的异常

// 大多数情况下，调用 N-API 函数的时候，如果已经有异常被挂起，函数会立即返回 napi_pending_exception，但是，N-API 允许一些函数被调用用来在返回 JavaScript 环境前做最小化的清理
// 在这种情况下，napi_status 会反映函数的执行结果状态，而不是当前存在异常，为了避免混淆，在每次函数执行后请检查结果

// 当异常被挂起的时候，有两种方法
// 1. 进行适当的清理并返回，那么执行环境会返回到 JavaScript，作为返回到 JavaScript 的过渡的一部分，异常将在 JavaScript 调用原生方法的语句处抛出，大多数 N-API 函数在出现异常被挂起的时候，都只是简单的返回 napi_pending_exception，，所以在这种情况下，建议尽量少做事情，等待返回到 JavaScript 中处理异常
// 2. 尝试处理异常，在原生代码能捕获到异常的地方采取适当的操作，然后继续执行。只有当异常能被安全的处理的少数特定情况下推荐这样做，在这些情况下，napi_get_and_clear_last_exception 可以被用来获取并清除异常，在此之后，如果发现异常还是无法处理，可以通过 napi_throw 重新抛出错误

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <JavaScriptCore/JavaScriptCore.h>

#define NAPI_EXPERIMENTAL

#include "js_native_api.h"

struct napi_env__ {
    JSContextRef const contextRef;
    // undefined 和 null 实际上也可以当做 exception 抛出，所以异常检查只需要检查是否为 NULL
    JSValueRef lastException;
    napi_extended_error_info lastError;
};

static inline napi_status napi_set_last_error_code(napi_env env, napi_status error_code) {
    if (env) {
        env->lastError.error_code = error_code;
    }

    return error_code;
}

#define RETURN_STATUS_IF_FALSE(env, condition, status)                  \
  do {                                                                  \
    if (!(condition)) {                                                 \
      return napi_set_last_error_code((env), (status));                      \
    }                                                                   \
  } while (0)

#define CHECK_ARG(env, arg) \
  RETURN_STATUS_IF_FALSE((env), ((arg) != NULL), napi_invalid_arg)

#define CHECK_ENV(env)          \
  do {                          \
    if ((env) == NULL) {        \
      return napi_invalid_arg;  \
    }                           \
  } while (0)

#define NAPI_PREAMBLE(env) \
  do {                     \
    CHECK_ENV((env));                                                 \
    RETURN_STATUS_IF_FALSE((env), (((env)->lastException) != NULL), napi_pending_exception);                                      \
    napi_clear_last_error((env));                                     \
  } while (0)

// Warning: Keep in-sync with napi_status enum

static const char *error_messages[] = {
        NULL,
        "Invalid argument",
        "An object was expected",
        "A string was expected",
        "A string or symbol was expected",
        "A function was expected",
        "A number was expected",
        "A boolean was expected",
        "An array was expected",
        "Unknown failure",
        "An exception is pending",
        "The async work item was cancelled",
        "napi_escape_handle already called on scope",
        "Invalid handle scope usage",
        "Invalid callback scope usage",
        "Thread-safe function queue is full",
        "Thread-safe function handle is closing",
        "A bigint was expected",
        "A date was expected",
        "An arraybuffer was expected",
        "A detachable arraybuffer was expected",
        "Main thread would deadlock"
};

static inline napi_status napi_clear_last_error(napi_env env) {
    if (env) {
        env->lastError.error_code = napi_ok;

        // TODO(boingoing): Should this be a callback?
        env->lastError.engine_error_code = 0;
        env->lastError.engine_reserved = NULL;
    }

    return napi_ok;
}

napi_status napi_get_last_error_info(napi_env env, const napi_extended_error_info **result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    // The value of the constant below must be updated to reference the last
    // message in the `napi_status` enum each time a new error message is added.
    // We don't have a napi_status_last as this would result in an ABI
    // change each time a message was added.
//    const int last_status = napi_would_deadlock;

    static_assert(sizeof(error_messages) / sizeof(*error_messages) == napi_status_last, "Count of error messages must match count of error values");
    if (env->lastError.error_code < napi_status_last) {
        // Wait until someone requests the last error information to fetch the error
        // message string
        env->lastError.error_message = error_messages[env->lastError.error_code];
    } else {
        assert(false);
    }

    *result = &(env->lastError);

    return napi_clear_last_error(env);
}

napi_status napi_get_undefined(napi_env env, napi_value *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    // JSValueMakeUndefined 不能传入 NULL，否则触发 release_assert
    CHECK_ARG(env, env->contextRef);

    *result = (napi_value) JSValueMakeUndefined(env->contextRef);

    return napi_clear_last_error(env);
}

napi_status napi_get_null(napi_env env, napi_value *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    // JSValueMakeNull 不能传入 NULL，否则触发 release_assert
    CHECK_ARG(env, env->contextRef);

    *result = (napi_value) JSValueMakeNull(env->contextRef);

    return napi_clear_last_error(env);
}

napi_status napi_get_global(napi_env env, napi_value *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    // JSContextGetGlobalObject 不能传入 NULL，否则触发 release_assert
    CHECK_ARG(env, env->contextRef);

    *result = (napi_value) JSContextGetGlobalObject(env->contextRef);

    return napi_clear_last_error(env);
}

napi_status napi_get_boolean(napi_env env, bool value, napi_value *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    // JSValueMakeBoolean 不能传入 NULL，否则触发 release_assert
    CHECK_ARG(env, env->contextRef);

    *result = (napi_value) JSValueMakeBoolean(env->contextRef, value);

    return napi_clear_last_error(env);
}

napi_status napi_create_object(napi_env env, napi_value *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    // JSObjectMake 不能传入 NULL，否则触发 release_assert
    CHECK_ARG(env, env->contextRef);

    *result = (napi_value) JSObjectMake(env->contextRef, NULL, NULL);

    return napi_clear_last_error(env);
}

napi_status napi_create_array(napi_env env, napi_value *result) {
    return napi_create_array_with_length(env, 0, result);
}

napi_status napi_create_array_with_length(napi_env env, size_t length, napi_value *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    // JSObjectMakeArray JSValueMakeNumber JSObjectSetProperty 不能传入 NULL，否则触发 release_assert
    CHECK_ARG(env, env->contextRef);

    *result = (napi_value) JSObjectMakeArray(env->contextRef, 0, NULL, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, env->lastException, napi_pending_exception);
//    assert(*result);
    if (length != 0) {
        // JSObjectSetProperty 不能传入 NULL object
        // 实际上不会存在未抛出异常，但是返回 NULL 的情况
//        RETURN_STATUS_IF_FALSE(env, *result, napi_generic_failure);

        // 实际上不存在返回 NULL 的情况
//        RETURN_STATUS_IF_FALSE(env, lengthValueRef, napi_generic_failure);
        JSStringRef lengthStringRef = JSStringCreateWithUTF8CString("length");
        // { value: 0, writable: true, enumerable: false, configurable: false }
        // 实际上 attributes 不会生效
        // JSValueMakeNumber 必定有返回
        // 忽略异常
        JSObjectSetProperty(env->contextRef, (JSObjectRef) *result, lengthStringRef, JSValueMakeNumber(env->contextRef, length), kJSPropertyAttributeDontEnum | kJSPropertyAttributeDontDelete, &env->lastException);
        JSStringRelease(lengthStringRef);
        RETURN_STATUS_IF_FALSE(env, env->lastException, napi_pending_exception);
    }

    return napi_clear_last_error(env);
}

napi_status napi_create_double(napi_env env, double value, napi_value *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->contextRef);

    *result = (napi_value) JSValueMakeNumber(env->contextRef, value);

    return napi_clear_last_error(env);
}

napi_status napi_create_int32(napi_env env, int32_t value, napi_value *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->contextRef);

    *result = (napi_value) JSValueMakeNumber(env->contextRef, value);

    return napi_clear_last_error(env);
}

napi_status napi_create_uint32(napi_env env, uint32_t value, napi_value *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->contextRef);

    *result = (napi_value) JSValueMakeNumber(env->contextRef, value);

    return napi_clear_last_error(env);
}

napi_status napi_create_int64(napi_env env, int64_t value, napi_value *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->contextRef);

    *result = (napi_value) JSValueMakeNumber(env->contextRef, value);

    return napi_clear_last_error(env);
}

// 等同于 napi_create_string_utf8
napi_status napi_create_string_latin1(napi_env env, const char *str, size_t length, napi_value *result) {
    // TODO(ChasonTang): 未实现
    return napi_generic_failure;
}

// JavaScriptCore 只能接受 \0 结尾的字符串
napi_status napi_create_string_utf8(napi_env env,
        const char *str,
        size_t length,
        napi_value *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    // JSValueMakeString 不能传入 NULL，否则触发 release_assert
    CHECK_ARG(env, env->contextRef);
    RETURN_STATUS_IF_FALSE(env, length == NAPI_AUTO_LENGTH, napi_invalid_arg);

    JSStringRef stringRef = JSStringCreateWithUTF8CString(str);
    *result = (napi_value) JSValueMakeString(env->contextRef, stringRef);
    JSStringRelease(stringRef);

    return napi_clear_last_error(env);
}

napi_status napi_create_string_utf16(napi_env env, const uint_least16_t *str, size_t length, napi_value *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->contextRef);
    RETURN_STATUS_IF_FALSE(env, length <= INT_MAX && length != NAPI_AUTO_LENGTH, napi_invalid_arg);

    JSStringRef stringRef = JSStringCreateWithCharacters(str, length);
    *result = (napi_value) JSValueMakeString(env->contextRef, stringRef);
    JSStringRelease(stringRef);

    return napi_clear_last_error(env);
}

napi_status napi_create_symbol(napi_env env, napi_value description, napi_value *result) {
    // TODO(ChasonTang): 未实现，需要考虑 iOS 9 的情况，iOS 13 才可以创建 Symbol
    return napi_generic_failure;
}

// 所有参数都会存在
static JSValueRef wrapperFunction(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception) {
    napi_callback callback = JSObjectGetPrivate(function);
    if (!callback) {
        assert(false);

        return NULL;
    }

    // TODO(ChasonTang): 实现
    return NULL;
}

napi_status napi_create_function(napi_env env, const char *utf8name, size_t length, napi_callback cb, void *data, napi_value *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);
    CHECK_ARG(env, cb);

    // JSObjectMakeFunctionWithCallback 不能传入 NULL，否则触发 release_assert
    CHECK_ARG(env, env->contextRef);

    JSStringRef stringRef = utf8name ? JSStringCreateWithUTF8CString(utf8name) : NULL;
    *result = (napi_value) JSObjectMakeFunctionWithCallback(env->contextRef, stringRef, wrapperFunction);
    if (stringRef) {
        JSStringRelease(stringRef);
    }
    RETURN_STATUS_IF_FALSE(env, JSObjectSetPrivate((JSObjectRef) *result, cb), napi_generic_failure);

    return napi_clear_last_error(env);
}

napi_status napi_create_error(napi_env env, napi_value code, napi_value msg, napi_value *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, msg);
    CHECK_ARG(env, result);

    // JSValueIsString JSObjectMakeError 不能传入 NULL，否则触发 release_assert
    CHECK_ARG(env, env->contextRef);
    RETURN_STATUS_IF_FALSE(env, JSValueIsString(env->contextRef, (JSValueRef) msg), napi_string_expected);

    JSValueRef args[] = {
            (JSValueRef) msg
    };
    *result = (napi_value) JSObjectMakeError(env->contextRef, 1, args, &env->lastException);

    // TODO(ChasonTang): Implement code logic

    return napi_clear_last_error(env);
}

napi_status napi_create_type_error(napi_env env, napi_value code, napi_value msg, napi_value *result) {
    // TODO(ChasonTang): 未实现
    return napi_generic_failure;
}

napi_status napi_create_range_error(napi_env env, napi_value code, napi_value msg, napi_value *result) {
    // TODO(ChasonTang): 未实现
    return napi_generic_failure;
}

napi_status napi_typeof(napi_env env, napi_value value, napi_valuetype *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->contextRef);

    if (JSValueIsNumber(env->contextRef, (JSValueRef) value)) {
        *result = napi_number;
    } else if (JSValueIsString(env->contextRef, (JSValueRef) value)) {
        *result = napi_string;
    } else if (JSValueIsBoolean(env->contextRef, (JSValueRef) value)) {
        *result = napi_boolean;
    } else if (JSValueIsUndefined(env->contextRef, (JSValueRef) value)) {
        *result = napi_undefined;
    } else if (JSValueIsNull(env->contextRef, (JSValueRef) value)) {
        *result = napi_null;
    } else if (JSValueIsObject(env->contextRef, (JSValueRef) value)) {
        JSObjectRef objectRef = JSValueToObject(env->contextRef, (JSValueRef) value, &env->lastException);
        RETURN_STATUS_IF_FALSE(env, env->lastException, napi_pending_exception);
        // Function
        if (JSObjectIsFunction(env->contextRef, objectRef)) {
            *result = napi_function;
        } else {
            *result = napi_object;
        }
    } else {
        return napi_set_last_error_code(env, napi_invalid_arg);
    }

    return napi_clear_last_error(env);
}

napi_status napi_get_value_double(napi_env env, napi_value value, double *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    // JSValueIsNumber 不能传入 NULL，否则触发 release_assert
    CHECK_ARG(env, env->contextRef);

    RETURN_STATUS_IF_FALSE(env, JSValueIsNumber(env->contextRef, (JSValueRef) value), napi_number_expected);

    *result = JSValueToNumber(env->contextRef, (JSValueRef) value, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, env->lastException, napi_pending_exception);

    return napi_clear_last_error(env);
}

napi_status napi_get_value_int32(napi_env env, napi_value value, int32_t *result) {
    return napi_get_value_double(env, value, (double *) result);
}

napi_status napi_get_value_int64(napi_env env, napi_value value, int64_t *result) {
    return napi_get_value_double(env, value, (double *) result);
}

napi_status napi_get_value_uint32(napi_env env,
        napi_value value,
        uint32_t *result) {
    return napi_get_value_double(env, value, (double *) result);
}

napi_status napi_get_value_bool(napi_env env, napi_value value, bool *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    // JSValueIsNumber 不能传入 NULL，否则触发 release_assert
    CHECK_ARG(env, env->contextRef);

    RETURN_STATUS_IF_FALSE(env, JSValueIsBoolean(env->contextRef, (JSValueRef) value), napi_number_expected);
    *result = JSValueToBoolean(env->contextRef, (JSValueRef) value);

    return napi_clear_last_error(env);
}

napi_status napi_get_value_string_latin1(napi_env env, napi_value value, char *buf, size_t bufsize, size_t *result) {
    // TODO(ChasonTang): 未实现
    return napi_generic_failure;
}

napi_status napi_get_value_string_utf8(napi_env env, napi_value value, char *buf, size_t bufsize, size_t *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);

    CHECK_ARG(env, env->contextRef);

    RETURN_STATUS_IF_FALSE(env, JSValueIsString(env->contextRef, (JSValueRef) value), napi_string_expected);

    JSStringRef stringRef = JSValueToStringCopy(env->contextRef, (JSValueRef) value, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, env->lastException, napi_pending_exception);

    if (!buf) {
        CHECK_ARG(env, result);
        // 不包括 \0
        *result = JSStringGetLength(stringRef);
    } else if (bufsize != 0) {
        size_t copied = JSStringGetUTF8CString(stringRef, buf, bufsize);
        if (result != NULL) {
            *result = copied;
        }
    } else if (result != NULL) {
        *result = 0;
    }

    return napi_clear_last_error(env);
}

napi_status napi_get_value_string_utf16(napi_env env, napi_value value, uint_least16_t *buf, size_t bufsize, size_t *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, value);

    CHECK_ARG(env, env->contextRef);

    RETURN_STATUS_IF_FALSE(env, JSValueIsString(env->contextRef, (JSValueRef) value), napi_string_expected);

    JSStringRef stringRef = JSValueToStringCopy(env->contextRef, (JSValueRef) value, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, env->lastException, napi_pending_exception);

    if (!buf) {
        CHECK_ARG(env, result);
        *result = JSStringGetLength(stringRef);
    } else if (bufsize != 0) {
        const uint_least16_t *charactersPtr = JSStringGetCharactersPtr(stringRef);
        RETURN_STATUS_IF_FALSE(env, charactersPtr, napi_generic_failure);
        size_t copied = (size_t) fminf(bufsize - 1, sizeof(uint_least16_t) * JSStringGetLength(stringRef));
        memcpy(buf, JSStringGetCharactersPtr(stringRef), copied);

        buf[copied] = '\0';
        if (result != NULL) {
            *result = copied;
        }
    } else if (result != NULL) {
        *result = 0;
    }

    return napi_clear_last_error(env);
}

napi_status napi_coerce_to_bool(napi_env env, napi_value value, napi_value *result) {
    CHECK_ENV(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->contextRef);

    *result = (napi_value) JSValueMakeBoolean(env->contextRef, JSValueToBoolean(env->contextRef, (JSValueRef) value));

    return napi_clear_last_error(env);
}

napi_status napi_coerce_to_number(napi_env env, napi_value value, napi_value *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->contextRef);

    double doubleValue = JSValueToNumber(env->contextRef, (JSValueRef) value, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, env->lastException, napi_pending_exception);

    *result = (napi_value) JSValueMakeNumber(env->contextRef, doubleValue);

    return napi_clear_last_error(env);
}

napi_status napi_coerce_to_object(napi_env env, napi_value value, napi_value *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->contextRef);

    *result = (napi_value) JSValueToObject(env->contextRef, (JSValueRef) value, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, env->lastException, napi_pending_exception);

    return napi_clear_last_error(env);
}

napi_status napi_coerce_to_string(napi_env env, napi_value value, napi_value *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->contextRef);

    JSStringRef stringRef = JSValueToStringCopy(env->contextRef, (JSValueRef) value, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, env->lastException, napi_pending_exception);
    *result = (napi_value) JSValueMakeString(env->contextRef, stringRef);

    return napi_clear_last_error(env);
}

napi_status napi_get_prototype(napi_env env, JSValueRef object, napi_value *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->contextRef);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->contextRef, object), napi_object_expected);

    JSObjectRef objectRef = JSValueToObject(env->contextRef, object, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, env->lastException, napi_pending_exception);

    *result = (napi_value) JSObjectGetPrototype(env->contextRef, objectRef);

    return napi_clear_last_error(env);
}

napi_status napi_get_all_property_names(napi_env env,
        napi_value object,
        napi_key_collection_mode key_mode,
        napi_key_filter key_filter,
        napi_key_conversion key_conversion,
        napi_value *result) {
    // 只支持 napi_key_include_prototypes + napi_key_enumerable + napi_key_skip_symbols
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->contextRef);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->contextRef, (JSValueRef) object), napi_object_expected);

    JSObjectRef objectRef = JSValueToObject(env->contextRef, (JSValueRef) object, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, env->lastException, napi_pending_exception);

    JSPropertyNameArrayRef propertyNameArrayRef = JSObjectCopyPropertyNames(env->contextRef, objectRef);

    size_t propertyCount = JSPropertyNameArrayGetCount(propertyNameArrayRef);

    if (propertyCount > 0) {
        JSObjectRef arrayObjectRef = JSObjectMakeArray(env->contextRef, 0, NULL, &env->lastException);
        if (env->lastException) {
            JSPropertyNameArrayRelease(propertyNameArrayRef);

            return napi_set_last_error_code(env, napi_pending_exception);
        }
        RETURN_STATUS_IF_FALSE(env, env->lastException, napi_pending_exception);

        for (unsigned int i = 0; i < propertyCount; ++i) {
            JSStringRef propertyNameStringRef = JSPropertyNameArrayGetNameAtIndex(propertyNameArrayRef, i);
            JSValueRef exception = NULL;
            JSValueRef propertyValueRef = JSObjectGetProperty(env->contextRef, objectRef, propertyNameStringRef, &exception);
            if (!exception) {
                // 忽略异常，直接跳过
                JSObjectSetPropertyAtIndex(env->contextRef, arrayObjectRef, i, propertyValueRef, NULL);
            }
        }
        *result = (napi_value) arrayObjectRef;
    }
    JSPropertyNameArrayRelease(propertyNameArrayRef);

    return napi_clear_last_error(env);
}

napi_status napi_get_property_names(napi_env env, napi_value object, napi_value *result) {
    return napi_get_all_property_names(env, object, napi_key_include_prototypes, napi_key_enumerable |
            napi_key_skip_symbols, napi_key_numbers_to_strings, result);
}

napi_status napi_set_property(napi_env env, napi_value object, napi_value key, napi_value value) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, key);
    CHECK_ARG(env, value);

    CHECK_ARG(env, env->contextRef);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->contextRef, (JSValueRef) object), napi_object_expected);
    JSObjectRef objectRef = JSValueToObject(env->contextRef, (JSValueRef) object, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, env->lastException, napi_pending_exception);
    JSObjectSetPropertyForKey(env->contextRef, objectRef, (JSValueRef) key, (JSValueRef) value, kJSPropertyAttributeNone, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, env->lastException, napi_pending_exception);

    return napi_clear_last_error(env);
}

napi_status napi_has_property(napi_env env, napi_value object, napi_value key, bool *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);
    CHECK_ARG(env, key);

    CHECK_ARG(env, env->contextRef);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->contextRef, (JSValueRef) object), napi_object_expected);
    JSObjectRef objectRef = JSValueToObject(env->contextRef, (JSValueRef) object, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, env->lastException, napi_pending_exception);
    *result = JSObjectHasPropertyForKey(env->contextRef, objectRef, (JSValueRef) key, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, env->lastException, napi_pending_exception);

    return napi_clear_last_error(env);
}

napi_status napi_get_property(napi_env env, napi_value object, napi_value key, napi_value *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);
    CHECK_ARG(env, key);

    CHECK_ARG(env, env->contextRef);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->contextRef, (JSValueRef) object), napi_object_expected);
    JSObjectRef objectRef = JSValueToObject(env->contextRef, (JSValueRef) object, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, env->lastException, napi_pending_exception);
    *result = (napi_value) JSObjectGetPropertyForKey(env->contextRef, objectRef, (JSValueRef) key, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, env->lastException, napi_pending_exception);

    return napi_clear_last_error(env);
}

napi_status napi_delete_property(napi_env env, napi_value object, napi_value key, bool *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);
    CHECK_ARG(env, key);

    CHECK_ARG(env, env->contextRef);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->contextRef, (JSValueRef) object), napi_object_expected);
    JSObjectRef objectRef = JSValueToObject(env->contextRef, (JSValueRef) object, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, env->lastException, napi_pending_exception);
    *result = JSObjectDeletePropertyForKey(env->contextRef, objectRef, (JSValueRef) key, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, env->lastException, napi_pending_exception);

    return napi_clear_last_error(env);
}

napi_status napi_has_own_property(napi_env env, napi_value object, napi_value key, bool *result) {
    // TODO(ChasonTang): 没有 Symbol 支持，所以无法检查 key 为 napi_name_expected，也缺少 own 的检查
    return napi_generic_failure;
}

napi_status napi_set_named_property(napi_env env, napi_value object, const char *utf8name, napi_value value) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);

    CHECK_ARG(env, env->contextRef);

    RETURN_STATUS_IF_FALSE(env, utf8name, napi_string_expected);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->contextRef, (JSValueRef) object), napi_object_expected);
    JSObjectRef objectRef = JSValueToObject(env->contextRef, (JSValueRef) object, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, env->lastException, napi_pending_exception);
    JSStringRef stringRef = JSStringCreateWithUTF8CString(utf8name);
    JSObjectSetProperty(env->contextRef, objectRef, stringRef, (JSValueRef) value, kJSPropertyAttributeNone, &env->lastException);
    JSStringRelease(stringRef);
    RETURN_STATUS_IF_FALSE(env, env->lastException, napi_pending_exception);

    return napi_clear_last_error(env);
}

napi_status napi_has_named_property(napi_env env, napi_value object, const char *utf8name, bool *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->contextRef);

    RETURN_STATUS_IF_FALSE(env, utf8name, napi_string_expected);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->contextRef, (JSValueRef) object), napi_object_expected);
    JSObjectRef objectRef = JSValueToObject(env->contextRef, (JSValueRef) object, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, env->lastException, napi_pending_exception);
    JSStringRef stringRef = JSStringCreateWithUTF8CString(utf8name);
    *result = JSObjectHasProperty(env->contextRef, objectRef, stringRef);
    JSStringRelease(stringRef);

    return napi_clear_last_error(env);
}

napi_status napi_get_named_property(napi_env env, napi_value object, const char *utf8name, napi_value *result) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, result);

    CHECK_ARG(env, env->contextRef);

    RETURN_STATUS_IF_FALSE(env, utf8name, napi_string_expected);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->contextRef, (JSValueRef) object), napi_object_expected);
    JSObjectRef objectRef = JSValueToObject(env->contextRef, (JSValueRef) object, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, env->lastException, napi_pending_exception);
    JSStringRef stringRef = JSStringCreateWithUTF8CString(utf8name);
    *result = (napi_value) JSObjectGetProperty(env->contextRef, objectRef, stringRef, &env->lastException);
    JSStringRelease(stringRef);
    RETURN_STATUS_IF_FALSE(env, env->lastException, napi_pending_exception);

    return napi_clear_last_error(env);
}

napi_status napi_set_element(napi_env env, napi_value object, uint32_t index, napi_value value) {
    NAPI_PREAMBLE(env);
    CHECK_ARG(env, value);

    CHECK_ARG(env, env->contextRef);

    RETURN_STATUS_IF_FALSE(env, JSValueIsObject(env->contextRef, (JSValueRef) object), napi_object_expected);
    JSObjectRef objectRef = JSValueToObject(env->contextRef, (JSValueRef) object, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, env->lastException, napi_pending_exception);
    JSObjectSetPropertyAtIndex(env->contextRef, objectRef, index, (JSValueRef) value, &env->lastException);
    RETURN_STATUS_IF_FALSE(env, env->lastException, napi_pending_exception);

    return napi_clear_last_error(env);
}