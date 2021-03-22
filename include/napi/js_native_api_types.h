//
//  js_native_api_types.h
//  NAPI
//
//  Created by 唐佳诚 on 2021/2/23.
//

#ifndef js_native_api_types_h
#define js_native_api_types_h

// 基于 Node.js 14.16.0
// #define NAPI_VERSION 7
// #define NAPI_EXPERIMENTAL
// JavaScript Engine Only
// LLVM Only

#include <stdint.h>

// JS VM API types are all opaque pointers for ABI stability
// typedef undefined structs instead of void* for compile time type safety
typedef struct OpaqueNAPIEnv *NAPIEnv;
typedef struct OpaqueNAPIValue *NAPIValue;
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
    // from instance properties. Ignored by NAPIDefineProperties.
    NAPIStatic = 1 << 10,

    // Default for class methods.
//    NAPIDefaultMethod = NAPIWritable | NAPIConfigurable,

    // Default for object properties, like in JS obj[prop].
//    NAPIDefaultJSProperty = NAPIWritable | NAPIEnumerable | NAPIConfigurable,
} NAPIPropertyAttributes;

typedef enum {
    // ES6 types (corresponds to typeof)
    NAPIUndefined,
    NAPINull,
    NAPIBoolean,
    NAPINumber,
    NAPIString,
    NAPIObject,
    NAPIFunction,
    NAPIExternal,
} NAPIValueType;

typedef enum {
    NAPIOk,
    NAPIInvalidArg,
    NAPIObjectExpected,
    NAPIStringExpected,
    NAPIFunctionExpected,
    NAPINumberExpected,
    NAPIBooleanExpected,
    NAPIArrayExpected,
    NAPIGenericFailure,
    NAPIPendingException,
    NAPIEscapeCalledTwice,
    NAPIHandleScopeMismatch,
    NAPIDateExpected,

    NAPIStatusLast // 用于静态断言
} NAPIStatus;
// 当添加一个新枚举到 `NAPIStatus` 时
// 1. 必须放在 NAPIStatusLast 前
// 2. 同时更新 js_native_api_{engine}.{implementation_unit_extension} 中的 const char *error_messages[] 数组的错误文案

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

typedef enum {
    NAPIKeyIncludePrototypes,
    NAPIKeyOwnOnly
} NAPIKeyCollectionMode;

typedef enum {
    NAPIKeyAllProperties = 0,
    NAPIKeyWritable = 1,
    NAPIKeyEnumerable = 1 << 1,
    NAPIKeyConfigurable = 1 << 2,
    NAPIKeySkipStrings = 1 << 3,
} NAPIKeyFilter;

typedef enum {
    NAPIKeyKeepNumbers,
    NAPIKeyNumbersToStrings
} NAPIKeyConversion;

#endif /* js_native_api_types_h */
