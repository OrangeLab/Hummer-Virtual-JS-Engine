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

// JavaScriptCore 目标 iOS 9

// Use INT_MAX, this should only be consumed by the pre-processor anyway.
#define NAPI_VERSION_EXPERIMENTAL 2147483647
#ifndef NAPI_VERSION
#ifdef NAPI_EXPERIMENTAL
#define NAPI_VERSION NAPI_VERSION_EXPERIMENTAL
#else
// The baseline version for N-API.
// The NAPI_VERSION controls which version will be used by default when
// compilling a native addon. If the addon developer specifically wants to use
// functions available in a new version of N-API that is not yet ported in all
// LTS versions, they can set NAPI_VERSION knowing that they have specifically
// depended on that version.
#define NAPI_VERSION 4
#endif
#endif

#include <napi/js_native_api_types.h>

#define NAPI_AUTO_LENGTH SIZE_MAX

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

NAPIStatus napi_get_last_error_info(NAPIEnv env, const NAPIExtendedErrorInfo **result);

// Getters for defined singletons
NAPIStatus napi_get_undefined(NAPIEnv env, NAPIValue *result);

NAPIStatus napi_get_null(NAPIEnv env, NAPIValue *result);

NAPIStatus napi_get_global(NAPIEnv env, NAPIValue *result);

NAPIStatus napi_get_boolean(NAPIEnv env, bool value, NAPIValue *result);

// Methods to create Primitive types/Objects
NAPIStatus napi_create_object(NAPIEnv env, NAPIValue *result);

NAPIStatus napi_create_array(NAPIEnv env, NAPIValue *result);

NAPIStatus napi_create_array_with_length(NAPIEnv env, size_t length, NAPIValue *result);

NAPIStatus napi_create_double(NAPIEnv env, double value, NAPIValue *result);

NAPIStatus napi_create_int32(NAPIEnv env, int32_t value, NAPIValue *result);

NAPIStatus napi_create_uint32(NAPIEnv env, uint32_t value, NAPIValue *result);

NAPIStatus napi_create_int64(NAPIEnv env, int64_t value, NAPIValue *result);

NAPIStatus napi_create_string_latin1(NAPIEnv env, const char *str, size_t length, NAPIValue *result);

NAPIStatus napi_create_string_utf8(NAPIEnv env, const char *str, size_t length, NAPIValue *result);

// 原先为 const char16_t *str,
// See https://stackoverflow.com/questions/50965615/why-is-char16-t-defined-to-have-the-same-size-as-uint-least16-t-instead-of-uint1
NAPIStatus napi_create_string_utf16(NAPIEnv env, const uint_least16_t *str, size_t length, NAPIValue *result);

NAPIStatus napi_create_symbol(NAPIEnv env, NAPIValue description, NAPIValue *result);

NAPIStatus napi_create_function(NAPIEnv env, const char *utf8name, size_t length, NAPICallback cb, void *data, NAPIValue *result);

NAPIStatus napi_create_error(NAPIEnv env, NAPIValue code, NAPIValue msg, NAPIValue *result);

NAPIStatus napi_create_type_error(NAPIEnv env, NAPIValue code, NAPIValue msg, NAPIValue *result);

NAPIStatus napi_create_range_error(NAPIEnv env, NAPIValue code, NAPIValue msg, NAPIValue *result);

// Methods to get the native NAPIValue from Primitive type
NAPIStatus napi_typeof(NAPIEnv env, NAPIValue value, NAPIValueType *result);

NAPIStatus napi_get_value_double(NAPIEnv env, NAPIValue value, double *result);

NAPIStatus napi_get_value_int32(NAPIEnv env, NAPIValue value, int32_t *result);

NAPIStatus napi_get_value_uint32(NAPIEnv env, NAPIValue value, uint32_t *result);

NAPIStatus napi_get_value_int64(NAPIEnv env, NAPIValue value, int64_t *result);

NAPIStatus napi_get_value_bool(NAPIEnv env, NAPIValue value, bool *result);

// Copies LATIN-1 encoded bytes from a string into a buffer.
NAPIStatus napi_get_value_string_latin1(NAPIEnv env, NAPIValue value, char *buf, size_t bufsize, size_t *result);

// Copies UTF-8 encoded bytes from a string into a buffer.
NAPIStatus napi_get_value_string_utf8(NAPIEnv env, NAPIValue value, char *buf, size_t bufsize, size_t *result);

// 原先为 char16_t *buf,
// Copies UTF-16 encoded bytes from a string into a buffer.
NAPIStatus napi_get_value_string_utf16(NAPIEnv env, NAPIValue value, uint_least16_t *buf, size_t bufsize, size_t *result);

// Methods to coerce values
// These APIs may execute user scripts
NAPIStatus napi_coerce_to_bool(NAPIEnv env, NAPIValue value, NAPIValue *result);

NAPIStatus napi_coerce_to_number(NAPIEnv env, NAPIValue value, NAPIValue *result);

NAPIStatus napi_coerce_to_object(NAPIEnv env, NAPIValue value, NAPIValue *result);

NAPIStatus napi_coerce_to_string(NAPIEnv env, NAPIValue value, NAPIValue *result);

// Methods to work with Objects
NAPIStatus napi_get_prototype(NAPIEnv env, NAPIValue object, NAPIValue *result);

NAPIStatus napi_get_property_names(NAPIEnv env, NAPIValue object, NAPIValue *result);

NAPIStatus napi_set_property(NAPIEnv env, NAPIValue object, NAPIValue key, NAPIValue value);

NAPIStatus napi_has_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result);

NAPIStatus napi_get_property(NAPIEnv env, NAPIValue object, NAPIValue key, NAPIValue *result);

NAPIStatus napi_delete_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result);

NAPIStatus napi_has_own_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result);

NAPIStatus napi_set_named_property(NAPIEnv env, NAPIValue object, const char *utf8name, NAPIValue value);

NAPIStatus napi_has_named_property(NAPIEnv env, NAPIValue object, const char *utf8name, bool *result);

NAPIStatus napi_get_named_property(NAPIEnv env, NAPIValue object, const char *utf8name, NAPIValue *result);

NAPIStatus napi_set_element(NAPIEnv env, NAPIValue object, uint32_t index, NAPIValue value);

NAPIStatus napi_has_element(NAPIEnv env, NAPIValue object, uint32_t index, bool *result);

NAPIStatus napi_get_element(NAPIEnv env, NAPIValue object, uint32_t index, NAPIValue *result);

NAPIStatus napi_delete_element(NAPIEnv env, NAPIValue object, uint32_t index, bool *result);

NAPIStatus napi_define_properties(NAPIEnv env, NAPIValue object, size_t property_count, const NAPIPropertyDescriptor *properties);

// Methods to work with Arrays
// 需要处理 Proxy 的情况
NAPIStatus napi_is_array(NAPIEnv env, NAPIValue value, bool *result);

NAPIStatus napi_get_array_length(NAPIEnv env, NAPIValue value, uint32_t *result);

// Methods to compare values
NAPIStatus napi_strict_equals(NAPIEnv env, NAPIValue lhs, NAPIValue rhs, bool *result);

// Methods to work with Functions
NAPIStatus napi_call_function(NAPIEnv env, NAPIValue recv, NAPIValue func, size_t argc, const NAPIValue *argv, NAPIValue *result);

NAPIStatus napi_new_instance(NAPIEnv env, NAPIValue constructor, size_t argc, const NAPIValue *argv, NAPIValue *result);

NAPIStatus napi_instanceof(NAPIEnv env, NAPIValue object, NAPIValue constructor, bool *result);

// Methods to work with napi_callbacks

// Gets all callback info in a single call. (Ugly, but faster.)
NAPIStatus napi_get_cb_info(
    NAPIEnv env,             // [in] NAPI environment handle
    NAPICallbackInfo cbinfo, // [in] Opaque callback-info handle
    size_t *argc,            // [in-out] Specifies the size of the provided argv array and receives the actual count of args.
    NAPIValue *argv,         // [out] Array of values
    NAPIValue *thisArg,      // [out] Receives the JS 'this' arg for the call
    void **data);            // [out] Receives the data pointer for the callback.

NAPIStatus napi_get_new_target(NAPIEnv env, NAPICallbackInfo cbinfo, NAPIValue *result);

NAPIStatus napi_define_class(NAPIEnv env, const char *utf8name, size_t length, NAPICallback constructor, void *data, size_t property_count, const NAPIPropertyDescriptor *properties, NAPIValue *result);

// Methods to work with external data objects
NAPIStatus napi_wrap(NAPIEnv env, NAPIValue js_object, void *native_object, NAPIFinalize finalize_cb, void *finalize_hint, NAPIRef *result);

NAPIStatus napi_unwrap(NAPIEnv env, NAPIValue js_object, void **result);

NAPIStatus napi_remove_wrap(NAPIEnv env, NAPIValue js_object, void **result);

NAPIStatus napi_create_external(NAPIEnv env, void *data, NAPIFinalize finalizeCB, void *finalizeHint, NAPIValue *result);

NAPIStatus napi_get_value_external(NAPIEnv env, NAPIValue value, void **result);

// Methods to control object lifespan

// Set initial_refcount to 0 for a weak reference, >0 for a strong reference.
NAPIStatus napi_create_reference(NAPIEnv env, NAPIValue value, uint32_t initialRefCount, NAPIRef *result);

// Deletes a reference. The referenced value is released, and may
// be GC'd unless there are other references to it.
NAPIStatus napi_delete_reference(NAPIEnv env, NAPIRef ref);

// Increments the reference count, optionally returning the resulting count.
// After this call the  reference will be a strong reference because its
// refcount is >0, and the referenced object is effectively "pinned".
// Calling this when the refcount is 0 and the object is unavailable
// results in an error.
NAPIStatus napi_reference_ref(NAPIEnv env, NAPIRef ref, uint32_t *result);

// Decrements the reference count, optionally returning the resulting count.
// If the result is 0 the reference is now weak and the object may be GC'd
// at any time if there are no other references. Calling this when the
// refcount is already 0 results in an error.
NAPIStatus napi_reference_unref(NAPIEnv env, NAPIRef ref, uint32_t *result);

NAPIStatus napi_get_reference_value(NAPIEnv env, NAPIRef ref, NAPIValue *result);

NAPIStatus napi_open_handle_scope(NAPIEnv env, NAPIHandleScope *result);

NAPIStatus napi_close_handle_scope(NAPIEnv env, NAPIHandleScope scope);

NAPIStatus napi_open_escapable_handle_scope(NAPIEnv env, NAPIEscapableHandleScope *result);

NAPIStatus napi_close_escapable_handle_scope(NAPIEnv env, NAPIEscapableHandleScope scope);

NAPIStatus napi_escape_handle(NAPIEnv env, NAPIEscapableHandleScope scope, NAPIValue escapee, NAPIValue *result);

// Methods to support error handling
NAPIStatus napi_throw(NAPIEnv env, NAPIValue error);

NAPIStatus napi_throw_error(NAPIEnv env, const char *code, const char *msg);

NAPIStatus napi_throw_type_error(NAPIEnv env, const char *code, const char *msg);

NAPIStatus napi_throw_range_error(NAPIEnv env, const char *code, const char *msg);

NAPIStatus napi_is_error(NAPIEnv env, NAPIValue value, bool *result);

// Methods to support catching exceptions
NAPIStatus napi_is_exception_pending(NAPIEnv env, bool *result);

NAPIStatus napi_get_and_clear_last_exception(NAPIEnv env, NAPIValue *result);

// Methods to work with array buffers and typed arrays
NAPIStatus napi_is_arraybuffer(NAPIEnv env, NAPIValue value, bool *result);

NAPIStatus napi_create_arraybuffer(NAPIEnv env, size_t byte_length, void **data, NAPIValue *result);

NAPIStatus napi_create_external_arraybuffer(NAPIEnv env, void *external_data, size_t byte_length, NAPIFinalize finalize_cb, void *finalize_hint, NAPIValue *result);

NAPIStatus napi_get_arraybuffer_info(NAPIEnv env,
                                     NAPIValue arraybuffer,
                                     void **data,
                                     size_t *byte_length);

NAPIStatus napi_is_typedarray(NAPIEnv env,
                              NAPIValue value,
                              bool *result);

NAPIStatus napi_create_typedarray(NAPIEnv env,
                                  NAPITypedArrayType type,
                                  size_t length,
                                  NAPIValue arraybuffer,
                                  size_t byte_offset,
                                  NAPIValue *result);

NAPIStatus napi_get_typedarray_info(NAPIEnv env,
                                    NAPIValue typedarray,
                                    NAPITypedArrayType *type,
                                    size_t *length,
                                    void **data,
                                    NAPIValue *arraybuffer,
                                    size_t *byte_offset);

NAPIStatus napi_create_dataview(NAPIEnv env,
                                size_t length,
                                NAPIValue arraybuffer,
                                size_t byte_offset,
                                NAPIValue *result);

NAPIStatus napi_is_dataview(NAPIEnv env,
                            NAPIValue value,
                            bool *result);

NAPIStatus napi_get_dataview_info(NAPIEnv env,
                                  NAPIValue dataview,
                                  size_t *bytelength,
                                  void **data,
                                  NAPIValue *arraybuffer,
                                  size_t *byte_offset);

// version management
NAPIStatus napi_get_version(NAPIEnv env, uint32_t *result);

// Promises
NAPIStatus napi_create_promise(NAPIEnv env,
                               NAPIDeferred *deferred,
                               NAPIValue *promise);

NAPIStatus napi_resolve_deferred(NAPIEnv env,
                                 NAPIDeferred deferred,
                                 NAPIValue resolution);

NAPIStatus napi_reject_deferred(NAPIEnv env,
                                NAPIDeferred deferred,
                                NAPIValue rejection);

NAPIStatus napi_is_promise(NAPIEnv env,
                           NAPIValue value,
                           bool *is_promise);

// Running a script
NAPIStatus napi_run_script(NAPIEnv env, NAPIValue script, NAPIValue *result);

// Memory management
NAPIStatus napi_adjust_external_memory(NAPIEnv env,
                                       int64_t change_in_bytes,
                                       int64_t *adjusted_value);

#if NAPI_VERSION >= 5

// Dates
NAPIStatus napi_create_date(NAPIEnv env, double time, NAPIValue *result);

// instanceof
NAPIStatus napi_is_date(NAPIEnv env, NAPIValue value, bool *isDate);

NAPIStatus napi_get_date_value(NAPIEnv env, NAPIValue value, double *result);

// Add finalizer for pointer
NAPIStatus napi_add_finalizer(NAPIEnv env,
                              NAPIValue js_object,
                              void *native_object,
                              NAPIFinalize finalize_cb,
                              void *finalize_hint,
                              NAPIRef *result);

#endif // NAPI_VERSION >= 5

#if NAPI_VERSION >= 6
// BigInt
NAPIStatus napi_create_bigint_int64(NAPIEnv env,
                                    int64_t value,
                                    NAPIValue *result);
NAPIStatus napi_create_bigint_uint64(NAPIEnv env,
                                     uint64_t value,
                                     NAPIValue *result);
NAPIStatus napi_create_bigint_words(NAPIEnv env,
                                    int sign_bit,
                                    size_t word_count,
                                    const uint64_t *words,
                                    NAPIValue *result);
NAPIStatus napi_get_value_bigint_int64(NAPIEnv env,
                                       NAPIValue value,
                                       int64_t *result,
                                       bool *lossless);
NAPIStatus napi_get_value_bigint_uint64(NAPIEnv env,
                                        NAPIValue value,
                                        uint64_t *result,
                                        bool *lossless);
NAPIStatus napi_get_value_bigint_words(NAPIEnv env,
                                       NAPIValue value,
                                       int *sign_bit,
                                       size_t *word_count,
                                       uint64_t *words);

// Object
NAPIStatus napi_get_all_property_names(NAPIEnv env,
                                       NAPIValue object,
                                       NAPIKeyCollectionMode key_mode,
                                       NAPIKeyFilter key_filter,
                                       NAPIKeyConversion key_conversion,
                                       NAPIValue *result);

// Instance data
NAPIStatus napi_set_instance_data(NAPIEnv env,
                                  void *data,
                                  NAPIFinalize finalize_cb,
                                  void *finalize_hint);

NAPIStatus napi_get_instance_data(NAPIEnv env,
                                  void **data);

#endif // NAPI_VERSION >= 6

#if NAPI_VERSION >= 7

// ArrayBuffer detaching
NAPIStatus napi_detach_arraybuffer(NAPIEnv env,
                                   NAPIValue arraybuffer);

NAPIStatus napi_is_detached_arraybuffer(NAPIEnv env,
                                        NAPIValue value,
                                        bool *result);

#endif // NAPI_VERSION >= 7

#ifdef NAPI_EXPERIMENTAL

NAPIStatus NAPITypeTag_object(NAPIEnv env,
                              NAPIValue value,
                              const NAPITypeTag *type_tag);

NAPIStatus
napi_check_object_type_tag(NAPIEnv env,
                           NAPIValue value,
                           const NAPITypeTag *type_tag,
                           bool *result);
NAPIStatus napi_object_freeze(NAPIEnv env,
                              NAPIValue object);
NAPIStatus napi_object_seal(NAPIEnv env,
                            NAPIValue object);

#endif // NAPI_EXPERIMENTAL

// 自定义函数
// NAPIStatus NAPICreateEnv(NAPIEnv *env);

// NAPIStatus NAPIFreeEnv(NAPIEnv *env);

EXTERN_C_END

#endif /* js_native_api_h */
