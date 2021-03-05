//
//  js_native_api.h
//  NAPI
//
//  Created by 唐佳诚 on 2021/2/23.
//

#ifndef js_native_api_h
#define js_native_api_h

#include <stddef.h>
#include <stdbool.h>

#include <NAPI/js_native_api_types.h>

#define NAPI_AUTO_LENGTH SIZE_MAX

#ifdef __cplusplus
#define EXTERN_C_START extern "C" {
#define EXTERN_C_END }
#else
#define EXTERN_C_START
#define EXTERN_C_END
#endif

EXTERN_C_START

/*
// ArrayBuffer detaching
NAPIStatus napi_detach_arraybuffer(NAPIEnv env, NAPIValue arraybuffer);

NAPIStatus napi_is_detached_arraybuffer(NAPIEnv env, NAPIValue value, bool *result);

// Instance data
NAPIStatus napi_set_instance_data(NAPIEnv env, void *data, NAPIFinalize finalize_cb, void *finalize_hint);

NAPIStatus napi_get_instance_data(NAPIEnv env, void **data);

// BigInt
NAPIStatus napi_create_bigint_int64(NAPIEnv env, int64_t value, NAPIValue *result);

NAPIStatus napi_create_bigint_uint64(NAPIEnv env, uint64_t value, NAPIValue *result);

NAPIStatus napi_create_bigint_words(NAPIEnv env, int sign_bit, size_t word_count, const uint64_t *words, NAPIValue *result);

NAPIStatus napi_get_value_bigint_int64(NAPIEnv env, NAPIValue value, int64_t *result, bool *lossless);

NAPIStatus napi_get_value_bigint_uint64(NAPIEnv env, NAPIValue value, uint64_t *result, bool *lossless);

NAPIStatus napi_get_value_bigint_words(NAPIEnv env, NAPIValue value, int *sign_bit, size_t *word_count, uint64_t *words);

NAPIStatus napi_get_date_value(NAPIEnv env, NAPIValue value, double *result);

// Add finalizer for pointer
NAPIStatus napi_add_finalizer(NAPIEnv env, NAPIValue js_object, void *native_object, NAPIFinalize finalize_cb, void *finalize_hint, NAPIRef *result);

// Memory management
NAPIStatus napi_adjust_external_memory(NAPIEnv env, int64_t change_in_bytes, int64_t *adjusted_value);

// Promises
NAPIStatus napi_create_promise(NAPIEnv env, NAPIDeferred *deferred, NAPIValue *promise);

NAPIStatus napi_resolve_deferred(NAPIEnv env, NAPIDeferred deferred, NAPIValue resolution);

NAPIStatus napi_reject_deferred(NAPIEnv env, NAPIDeferred deferred, NAPIValue rejection);

NAPIStatus napi_is_promise(NAPIEnv env, NAPIValue value, bool *is_promise);

// version management
NAPIStatus napi_get_version(NAPIEnv env, uint32_t *result);

NAPIStatus napi_create_external_arraybuffer(NAPIEnv env, void *external_data, size_t byte_length, NAPIFinalize finalize_cb, void *finalize_hint, NAPIValue *result);

NAPIStatus napi_get_arraybuffer_info(NAPIEnv env, NAPIValue arraybuffer, void **data, size_t *byte_length);

NAPIStatus napi_is_typedarray(NAPIEnv env, NAPIValue value, bool *result);

NAPIStatus napi_create_typedarray(NAPIEnv env, NAPITypedArrayType type, size_t length, NAPIValue arraybuffer, size_t byte_offset, NAPIValue *result);

NAPIStatus napi_get_typedarray_info(NAPIEnv env, NAPIValue typedarray, NAPITypedArrayType *type, size_t *length, void **data, NAPIValue *arraybuffer, size_t *byte_offset);

NAPIStatus napi_create_dataview(NAPIEnv env, size_t length, NAPIValue arraybuffer, size_t byte_offset, NAPIValue *result);

NAPIStatus napi_is_dataview(NAPIEnv env, NAPIValue value, bool *result);

NAPIStatus napi_get_dataview_info(NAPIEnv env, NAPIValue dataview, size_t *bytelength, void **data, NAPIValue *arraybuffer, size_t *byte_offset);

NAPIStatus napi_get_new_target(NAPIEnv env, NAPICallbackInfo cbinfo, NAPIValue *result);

// Methods to work with array buffers and typed arrays
NAPIStatus napi_is_arraybuffer(NAPIEnv env, NAPIValue value, bool *result);

NAPIStatus napi_create_arraybuffer(NAPIEnv env, size_t byte_length, void **data, NAPIValue *result);

NAPIStatus napi_is_error(NAPIEnv env, NAPIValue value, bool *result);

NAPIStatus napi_throw_type_error(NAPIEnv env, const char *code, const char *msg);

NAPIStatus napi_throw_range_error(NAPIEnv env, const char *code, const char *msg);

// Attempts to get a referenced value. If the reference is weak,
// the value might no longer be available, in that case the call
// is still successful but the result is NULL.
NAPIStatus napi_get_reference_value(NAPIEnv env, NAPIRef ref, NAPIValue *result);

NAPIStatus napi_define_class(NAPIEnv env, const char *utf8name, size_t length, NAPICallback constructor, void *data, size_t property_count, const NAPIPropertyDescriptor *properties, NAPIValue *result);

// Methods to work with external data objects
NAPIStatus napi_wrap(NAPIEnv env, NAPIValue js_object, void *native_object, NAPIFinalize finalize_cb, void *finalize_hint, NAPIRef *result);

NAPIStatus napi_unwrap(NAPIEnv env, NAPIValue js_object, void **result);

NAPIStatus napi_remove_wrap(NAPIEnv env, NAPIValue js_object, void **result);

NAPIStatus napi_new_instance(NAPIEnv env, NAPIValue constructor, size_t argc, const NAPIValue *argv, NAPIValue *result);

NAPIStatus napi_delete_element(NAPIEnv env, NAPIValue object, uint32_t index, bool *result);

NAPIStatus napi_has_element(NAPIEnv env, NAPIValue object, uint32_t index, bool *result);

NAPIStatus napi_create_range_error(NAPIEnv env, NAPIValue code, NAPIValue msg, NAPIValue *result);

NAPIStatus napi_create_type_error(NAPIEnv env, NAPIValue code, NAPIValue msg, NAPIValue *result);

// 未实现，需要考虑 iOS 9 的情况，iOS 13 才可以创建 Symbol
NAPIStatus napi_create_symbol(NAPIEnv env, NAPIValue description, NAPIValue *result);

// 没有 Symbol 支持，所以无法检查 key 为 NAPINameExpected，也缺少 own 的检查
NAPIStatus napi_has_own_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result);

// JavaScriptCore 只支持 UTF16 和 UTF8，latin1 在 UTF8 中表现为两字节
// Copies LATIN-1 encoded bytes from a string into a buffer.
NAPIStatus napi_get_value_string_latin1(NAPIEnv env, NAPIValue value, char *buf, size_t bufsize, size_t *result);

NAPIStatus napi_create_string_latin1(NAPIEnv env, const char *str, size_t length, NAPIValue *result);
 */

NAPIStatus NAPIGetLastErrorInfo(NAPIEnv env, const NAPIExtendedErrorInfo **result);

// Getters for defined singletons
NAPIStatus NAPIGetUndefined(NAPIEnv env, NAPIValue *result);

NAPIStatus NAPIGetNull(NAPIEnv env, NAPIValue *result);

NAPIStatus NAPIGetGlobal(NAPIEnv env, NAPIValue *result);

NAPIStatus NAPIGetBoolean(NAPIEnv env, bool value, NAPIValue *result);

// Methods to create Primitive types/Objects
NAPIStatus NAPICreateObject(NAPIEnv env, NAPIValue *result);

NAPIStatus NAPICreateArray(NAPIEnv env, NAPIValue *result);

//NAPIStatus NAPICreateArrayWithLength(NAPIEnv env, size_t length, NAPIValue *result);

NAPIStatus NAPICreateDouble(NAPIEnv env, double value, NAPIValue *result);

NAPIStatus NAPICreateInt32(NAPIEnv env, int32_t value, NAPIValue *result);

NAPIStatus NAPICreateUInt32(NAPIEnv env, uint32_t value, NAPIValue *result);

NAPIStatus NAPICreateInt64(NAPIEnv env, int64_t value, NAPIValue *result);

NAPIStatus NAPICreateStringUTF8(NAPIEnv env, const char *str, size_t length, NAPIValue *result);

// 原先为 const char16_t *str,
// See https://stackoverflow.com/questions/50965615/why-is-char16-t-defined-to-have-the-same-size-as-uint-least16-t-instead-of-uint1
NAPIStatus NAPICreateStringUTF16(NAPIEnv env, const uint_least16_t *str, size_t length, NAPIValue *result);

NAPIStatus NAPICreateFunction(NAPIEnv env, const char *utf8name, size_t length, NAPICallback cb, void *data, NAPIValue *result);

NAPIStatus NAPICreateError(NAPIEnv env, NAPIValue code, NAPIValue msg, NAPIValue *result);

// Methods to get the native NAPIValue from Primitive type
NAPIStatus NAPITypeOf(NAPIEnv env, NAPIValue value, NAPIValueType *result);

NAPIStatus NAPIGetValueDouble(NAPIEnv env, NAPIValue value, double *result);

NAPIStatus NAPIGetValueInt32(NAPIEnv env, NAPIValue value, int32_t *result);

NAPIStatus NAPIGetValueUInt32(NAPIEnv env, NAPIValue value, uint32_t *result);

NAPIStatus NAPIGetValueInt64(NAPIEnv env, NAPIValue value, int64_t *result);

NAPIStatus NAPIGetValueBool(NAPIEnv env, NAPIValue value, bool *result);

// Copies UTF-8 encoded bytes from a string into a buffer.
NAPIStatus NAPIGetValueStringUTF8(NAPIEnv env, NAPIValue value, char *buf, size_t bufsize, size_t *result);

// 原先为 char16_t *buf,
// Copies UTF-16 encoded bytes from a string into a buffer.
NAPIStatus NAPIGetValueStringUTF16(NAPIEnv env, NAPIValue value, uint_least16_t *buf, size_t bufsize, size_t *result);

// Methods to coerce values
// These APIs may execute user scripts
NAPIStatus NAPICoerceToBool(NAPIEnv env, NAPIValue value, NAPIValue *result);

NAPIStatus NAPICoerceToNumber(NAPIEnv env, NAPIValue value, NAPIValue *result);

NAPIStatus NAPICoerceToObject(NAPIEnv env, NAPIValue value, NAPIValue *result);

NAPIStatus NAPICoerceToString(NAPIEnv env, NAPIValue value, NAPIValue *result);

// Methods to work with Objects
NAPIStatus NAPIGetPrototype(NAPIEnv env, NAPIValue object, NAPIValue *result);

NAPIStatus NAPIGetPropertyNames(NAPIEnv env, NAPIValue object, NAPIValue *result);

NAPIStatus NAPISetProperty(NAPIEnv env, NAPIValue object, NAPIValue key, NAPIValue value);

NAPIStatus NAPIHasProperty(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result);

NAPIStatus NAPIGetProperty(NAPIEnv env, NAPIValue object, NAPIValue key, NAPIValue *result);

NAPIStatus NAPIDeleteProperty(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result);

NAPIStatus NAPISetNamedProperty(NAPIEnv env, NAPIValue object, const char *utf8name, NAPIValue value);

NAPIStatus NAPIHasNamedProperty(NAPIEnv env, NAPIValue object, const char *utf8name, bool *result);

NAPIStatus NAPIGetNamedProperty(NAPIEnv env, NAPIValue object, const char *utf8name, NAPIValue *result);

NAPIStatus NAPISetElement(NAPIEnv env, NAPIValue object, uint32_t index, NAPIValue value);

NAPIStatus NAPIGetElement(NAPIEnv env, NAPIValue object, uint32_t index, NAPIValue *result);

NAPIStatus NAPIDefineProperties(NAPIEnv env, NAPIValue object, size_t propertyCount, const NAPIPropertyDescriptor *properties);

// Methods to work with Arrays
// Array.isArray
NAPIStatus NAPIIsArray(NAPIEnv env, NAPIValue value, bool *result);

NAPIStatus NAPIGetArrayLength(NAPIEnv env, NAPIValue value, uint32_t *result);

// Methods to compare values
NAPIStatus NAPIStrictEquals(NAPIEnv env, NAPIValue lhs, NAPIValue rhs, bool *result);

// Methods to work with Functions
NAPIStatus NAPICallFunction(NAPIEnv env, NAPIValue recv, NAPIValue func, size_t argc, const NAPIValue *argv, NAPIValue *result);

NAPIStatus NAPIInstanceOf(NAPIEnv env, NAPIValue object, NAPIValue constructor, bool *result);

// Methods to work with napi_callbacks

// Gets all callback info in a single call. (Ugly, but faster.)
NAPIStatus NAPIGetCBInfo(
        NAPIEnv env, // [in] NAPI environment handle
        NAPICallbackInfo cbinfo, // [in] Opaque callback-info handle
        size_t *argc, // [in-out] Specifies the size of the provided argv array and receives the actual count of args.
        NAPIValue *argv, // [out] Array of values
        NAPIValue *thisArg, // [out] Receives the JS 'this' arg for the call
        void **data); // [out] Receives the data pointer for the callback.

NAPIStatus NAPICreateExternal(NAPIEnv env, void *data, NAPIFinalize finalizeCB, void *finalizeHint, NAPIValue *result);

NAPIStatus NAPIGetValueExternal(NAPIEnv env, NAPIValue value, void **result);

// Methods to control object lifespan

// Set initial_refcount to 0 for a weak reference, >0 for a strong reference.
NAPIStatus NAPICreateReference(NAPIEnv env, NAPIValue value, uint32_t initialRefCount, NAPIRef *result);

// Deletes a reference. The referenced value is released, and may
// be GC'd unless there are other references to it.
NAPIStatus NAPIDeleteReference(NAPIEnv env, NAPIRef ref);

// Increments the reference count, optionally returning the resulting count.
// After this call the  reference will be a strong reference because its
// refcount is >0, and the referenced object is effectively "pinned".
// Calling this when the refcount is 0 and the object is unavailable
// results in an error.
NAPIStatus NAPIReferenceRef(NAPIEnv env, NAPIRef ref, uint32_t *result);

// Decrements the reference count, optionally returning the resulting count.
// If the result is 0 the reference is now weak and the object may be GC'd
// at any time if there are no other references. Calling this when the
// refcount is already 0 results in an error.
NAPIStatus NAPIReferenceUnref(NAPIEnv env, NAPIRef ref, uint32_t *result);

NAPIStatus NAPIOpenHandleScope(NAPIEnv env, NAPIHandleScope *result);

NAPIStatus NAPICloseHandleScope(NAPIEnv env, NAPIHandleScope scope);

NAPIStatus NAPIOpenEscapableHandleScope(NAPIEnv env, NAPIEscapableHandleScope *result);

NAPIStatus NAPICloseEscapableHandleScope(NAPIEnv env, NAPIEscapableHandleScope scope);

NAPIStatus NAPIEscapeHandle(NAPIEnv env, NAPIEscapableHandleScope scope, NAPIValue escapee, NAPIValue *result);

// Methods to support error handling
NAPIStatus NAPIThrow(NAPIEnv env, NAPIValue error);

NAPIStatus NAPIThrowError(NAPIEnv env, const char *code, const char *msg);

// Methods to support catching exceptions
NAPIStatus NAPIIsExceptionPending(NAPIEnv env, bool *result);

NAPIStatus NAPIGetAndClearLastException(NAPIEnv env, NAPIValue *result);

// Running a script
NAPIStatus NAPIRunScript(NAPIEnv env, NAPIValue script, NAPIValue *result);

// Dates
NAPIStatus NAPICreateDate(NAPIEnv env, double time, NAPIValue *result);

NAPIStatus NAPIIsDate(NAPIEnv env, NAPIValue value, bool *isDate);

// Object
NAPIStatus NAPIGetAllPropertyNames(NAPIEnv env, NAPIValue object, NAPIKeyCollectionMode keyMode, NAPIKeyFilter keyFilter, NAPIKeyConversion keyConversion, NAPIValue *result);

EXTERN_C_END

#endif /* js_native_api_h */
