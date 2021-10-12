//
// Created by didi on 2021/10/11.
//

#ifndef SKIA_JS_NATIVE_API_DEBUGGER_TYPES_H
#define SKIA_JS_NATIVE_API_DEBUGGER_TYPES_H

//forward declare
namespace facebook {
namespace react {

class MessageQueueThread;
}
}

struct OpaqueMessageQueueThreadWrapper {
    std::shared_ptr<facebook::react::MessageQueueThread> thread_;
};

#endif // SKIA_JS_NATIVE_API_DEBUGGER_TYPES_H
