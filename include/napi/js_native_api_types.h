#ifndef js_native_api_types_h
#define js_native_api_types_h

#ifdef __cplusplus
#define EXTERN_C_START \
    extern "C"         \
    {
#define EXTERN_C_END }
#else
#define EXTERN_C_START
#define EXTERN_C_END
#endif

EXTERN_C_START

// 基于 Node.js 14.16.0
// #define NAPI_VERSION 7
// #define NAPI_EXPERIMENTAL

#include <stdint.h> // NOLINT(modernize-deprecated-headers)

#if !defined __cplusplus || (defined(_MSC_VER) && _MSC_VER < 1900)
typedef uint16_t char16_t;
#endif

// JSVM API types are all opaque pointers for ABI stability
// typedef undefined structs instead of void* for compile time type safety
typedef struct OpaqueNAPIEnv *NAPIEnv;
// JavaScriptCore V8 使用指针
// Hermes 使用 uint64_t
// QuickJS 在 32 位使用 uint64_t，在 64 位使用 128 位存储
#if INTPTR_MAX >= INT64_MAX
typedef struct {
    uint64_t head;
    uint64_t tail;
} NAPIValue;
#else
typedef uint64_t NAPIValue;
#endif
typedef struct OpaqueNAPIRef *NAPIRef;
typedef struct OpaqueNAPIHandleScope *NAPIHandleScope;
typedef struct OpaqueNAPIEscapableHandleScope *NAPIEscapableHandleScope;
typedef struct OpaqueNAPICallbackInfo *NAPICallbackInfo;
typedef struct OpaqueNAPIDeferred *NAPIDeferred;

typedef enum {
    NAPIDefault = 0,
    NAPIWritable = 1 << 0,
    NAPIEnumerable = 1 << 1,
    NAPIConfigurable = 1 << 2,

    // Used with napi_define_class to distinguish static properties
    // from instance properties. Ignored by napi_define_properties.
    NAPIStatic = 1 << 10,

    // Default for class methods.
    NAPIDefaultMethod = NAPIWritable | NAPIConfigurable,

    // Default for object properties, like in JS obj[prop].
    NAPIDefaultJSProperty = NAPIWritable | NAPIEnumerable | NAPIConfigurable,
} NAPIPropertyAttributes;

typedef enum {
    // ES6 types (corresponds to typeof)
    NAPIUndefined,
    NAPINull,
    NAPIBoolean,
    NAPINumber,
    NAPIString,
    NAPISymbol,
    NAPIObject,
    NAPIFunction,
    NAPIExternal,
} NAPIValueType;

typedef enum {
    NAPIOK,
    NAPIInvalidArg,
    NAPIObjectExpected,
    NAPIStringExpected,
    NAPINameExpected,
    NAPIFunctionExpected,
    NAPINumberExpected,
    NAPIBooleanExpected,
    NAPIArrayExpected,
    NAPIGenericFailure,
    NAPIPendingException,
    NAPICancelled,
    NAPIEscapeCalledTwice,
    NAPIHandleScopeMismatch,
    NAPICallbackScopeMismatch,
    NAPIQueueFull,
    NAPIClosing,
    NAPIBigIntExpected,
    NAPIDateExpected,
    NAPIArrayBufferExpected,
    NAPIDetachableArrayBufferExpected,
    NAPIWouldDeadLock, // unused
    // 自定义添加错误
    NAPIMemoryError
} NAPIStatus;
// Note: when adding a new enum value to `napi_status`, please also update
//   * `const int last_status` in the definition of `napi_get_last_error_info()'
//     in file js_native_api_v8.cc.
//   * `const char* error_messages[]` in file js_native_api_v8.cc with a brief
//     message explaining the error.
//   * the definition of `napi_status` in doc/api/n-api.md to reflect the newly
//     added value(s).

typedef NAPIValue (*NAPICallback)(NAPIEnv env, NAPICallbackInfo info);

typedef void (*NAPIFinalize)(NAPIEnv env, void *finalizeData, void *finalizeHint);

typedef struct {
    // One of utf8name or name should be NULL.
    const char *utf8name;
    NAPIValue name;

    NAPICallback method;
    NAPICallback getter;
    NAPICallback setter;
    NAPIValue value;

    NAPIPropertyAttributes attributes;
    void *data;
} NAPIPropertyDescriptor;

typedef struct {
    const char *errorMessage;
    void *engineReserved;
    uint32_t engineErrorCode;
    NAPIStatus errorCode;
} NAPIExtendedErrorInfo;

EXTERN_C_END

#endif /* js_native_api_types_h */
