//
// Created by didi on 2021/10/11.
//

#ifndef SRC_JS_NATIVE_API_DEBUGGER_H
#define SRC_JS_NATIVE_API_DEBUGGER_H

#include <napi/js_native_api_types.h>

#pragma mark - debugger

EXTERN_C_START

typedef struct OpaqueMessageQueueThreadWrapper *MessageQueueThreadWrapper;

// 直接引入 c++ 头文件，会导致 c/oc(非oc++) 编译可能出现出错。
// 需在 NAPIEnableDebugger 之前调用
NAPI_EXPORT NAPICommonStatus NAPISetMessageQueueThread(NAPIEnv env, MessageQueueThreadWrapper jsQueueWrapper);

// debuggerTitle 只对 Hermes 引擎生效，应当支持空指针
NAPI_EXPORT NAPICommonStatus NAPIEnableDebugger(NAPIEnv env, const char *debuggerTitle, bool waitForDebugger);

NAPI_EXPORT NAPICommonStatus NAPIDisableDebugger(NAPIEnv env);

EXTERN_C_END

#endif // SRC_JS_NATIVE_API_DEBUGGER_H
