#include <napi/js_native_api.h>
#include <common.h>

#include <stdio.h>

void add_returned_status(NAPIEnv env,
                         const char *key,
                         NAPIValue object,
                         const char *expected_message,
                         NAPIStatus expected_status,
                         NAPIStatus actual_status) {

    char napi_message_string[100] = "";
    NAPIValue prop_value;

    if (actual_status != expected_status) {
        snprintf(napi_message_string, sizeof(napi_message_string), "Invalid status [%d]", actual_status);
    }

    NAPI_CALL_RETURN_VOID(env,
                          napi_create_string_utf8(
                                  env,
                                  (actual_status == expected_status ?
                                   expected_message :
                                   napi_message_string),
                                  NAPI_AUTO_LENGTH,
                                  &prop_value));
    NAPI_CALL_RETURN_VOID(env,
                          napi_set_named_property(env,
                                                  object,
                                                  key,
                                                  prop_value));
}

void add_last_status(NAPIEnv env, const char *key, NAPIValue return_value) {
    NAPIValue prop_value;
    const NAPIExtendedErrorInfo *p_last_error;
    NAPI_CALL_RETURN_VOID(env, napi_get_last_error_info(env, &p_last_error));

    NAPI_CALL_RETURN_VOID(env,
                          napi_create_string_utf8(env,
                                                  (p_last_error->errorMessage == NULL ?
                                                   "napi_ok" :
                                                   p_last_error->errorMessage),
                                                  NAPI_AUTO_LENGTH,
                                                  &prop_value));
    NAPI_CALL_RETURN_VOID(env, napi_set_named_property(env,
                                                       return_value,
                                                       key,
                                                       prop_value));
}
