#pragma once

#ifdef HERMES_ENABLE_DEBUGGER

namespace facebook {
namespace react {
class MessageQueueThread;
}
namespace jsi {
class Runtime;
}
namespace hermes {
namespace debugger {
class Debugger;
}
class HermesRuntime;
}  // namespace hermes
}  // namespace facebook

#include <hermes/inspector/RuntimeAdapter.h>

namespace orangelabs {

class HermesExecutorRuntimeAdapter final
    : public ::facebook::hermes::inspector::RuntimeAdapter {
 public:
  explicit HermesExecutorRuntimeAdapter(
      ::std::shared_ptr<::facebook::jsi::Runtime> runtime,
      ::facebook::hermes::HermesRuntime &hermesRuntime,
      ::std::shared_ptr<::facebook::react::MessageQueueThread>
          messageQueueThread);

  ~HermesExecutorRuntimeAdapter() override = default;

  HermesExecutorRuntimeAdapter(const HermesExecutorRuntimeAdapter &) = delete;

  HermesExecutorRuntimeAdapter(HermesExecutorRuntimeAdapter &&) = delete;

  HermesExecutorRuntimeAdapter &operator=(
      const HermesExecutorRuntimeAdapter &) = delete;

  HermesExecutorRuntimeAdapter &operator=(HermesExecutorRuntimeAdapter &&) =
      delete;

  ::facebook::jsi::Runtime &getRuntime() override;

  ::facebook::hermes::debugger::Debugger &getDebugger() override;

  void tickleJs() override;

 private:
  const ::std::shared_ptr<::facebook::jsi::Runtime> runtime;
  ::facebook::hermes::HermesRuntime &hermesRuntime;
  const ::std::shared_ptr<::facebook::react::MessageQueueThread>
      messageQueueThread;
};
}  // namespace orangelabs
#endif