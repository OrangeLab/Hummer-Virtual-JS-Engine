#pragma once

#include <napi/js_native_api_types.h>

#include <memory>

namespace hermes::vm {
class RuntimeConfig;
class Runtime;
}  // namespace hermes::vm

namespace facebook::react {
class MessageQueueThread;
}

namespace facebook::hermes {
class HermesRuntime;
}

EXTERN_C_START

#include <sys/queue.h>

struct OpaqueNAPIEnv final {
 public:
  explicit OpaqueNAPIEnv(const ::hermes::vm::RuntimeConfig &runtimeConfig);

  ~OpaqueNAPIEnv();

  [[nodiscard]] ::hermes::vm::Runtime &getRuntime() const;

  OpaqueNAPIEnv(const OpaqueNAPIEnv &) = delete;

  OpaqueNAPIEnv(OpaqueNAPIEnv &&) = delete;

  OpaqueNAPIEnv &operator=(const OpaqueNAPIEnv &) = delete;

  OpaqueNAPIEnv &operator=(OpaqueNAPIEnv &&) = delete;

  LIST_HEAD(, OpaqueNAPIRef) valueList;

  LIST_HEAD(, OpaqueNAPIRef) weakRefList;

  LIST_HEAD(, OpaqueNAPIRef) strongRefList;

  SLIST_HEAD(, OpaqueNAPIHandleScope) handleScopeList;

#ifdef HERMES_ENABLE_DEBUGGER

  void enableDebugger(const char *debuggerTitle, bool waitForDebugger) const;

  void disableDebugger() const;

  void setMessageQueueThread(
      ::std::shared_ptr<::facebook::react::MessageQueueThread> jsQueue);

#endif

 private:
  ::std::shared_ptr<::facebook::react::MessageQueueThread> messageQueueThread;
  const ::std::shared_ptr<::facebook::hermes::HermesRuntime>
      hermesRuntimeSharedPointer;
  ::facebook::hermes::HermesRuntime &hermesRuntime;
  ::hermes::vm::Runtime &runtime;
};

EXTERN_C_END