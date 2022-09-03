#include <hermes/HermesExecutorRuntimeAdapter.h>

#ifdef HERMES_ENABLE_DEBUGGER

#include <cxxreact/MessageQueueThread.h>
#include <hermes/hermes.h>
#include <jsi/jsi.h>

orangelabs::HermesExecutorRuntimeAdapter::HermesExecutorRuntimeAdapter(
    ::std::shared_ptr<::facebook::jsi::Runtime> runtime,
    ::facebook::hermes::HermesRuntime &hermesRuntime,
    ::std::shared_ptr<::facebook::react::MessageQueueThread> messageQueueThread)
    : runtime(::std::move(runtime)), hermesRuntime(hermesRuntime),
      messageQueueThread(::std::move(messageQueueThread)) {}

::facebook::jsi::Runtime &
orangelabs::HermesExecutorRuntimeAdapter::getRuntime() {
  return *this->runtime;
}

::facebook::hermes::debugger::Debugger &
orangelabs::HermesExecutorRuntimeAdapter::getDebugger() {
  return this->hermesRuntime.getDebugger();
}

void orangelabs::HermesExecutorRuntimeAdapter::tickleJs() {
  this->messageQueueThread->runOnQueue([&runtime = this->runtime]() {
    auto func = runtime->global().getPropertyAsFunction(*runtime, "__tickleJs");
    func.call(*runtime);
  });
}

#endif