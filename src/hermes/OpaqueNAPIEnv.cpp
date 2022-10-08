#include <hermes/VM/Runtime.h>
#include <hermes/hermes.h>

#ifdef HERMES_ENABLE_DEBUGGER
#include <cxxreact/MessageQueueThread.h>
#include <hermes/HermesExecutorRuntimeAdapter.h>
// #include <hermes/inspector/chrome/Registration.h>
#include <hermes/js_native_api_hermes_inspector.h>
#endif

#include <hermes/OpaqueNAPIEnv.h>
#include <hermes/OpaqueNAPIHandleScope.h>
#include <hermes/OpaqueNAPIRef.h>

#ifndef SLIST_FOREACH_SAFE
#define SLIST_FOREACH_SAFE(var, head, field, tvar) \
  for ((var) = SLIST_FIRST((head));                \
       (var) && ((tvar) = SLIST_NEXT((var), field), 1); (var) = (tvar))
#endif

#ifndef LIST_FOREACH_SAFE
#define LIST_FOREACH_SAFE(var, head, field, tvar) \
  for ((var) = LIST_FIRST((head));                \
       (var) && ((tvar) = LIST_NEXT((var), field), 1); (var) = (tvar))
#endif

OpaqueNAPIEnv::OpaqueNAPIEnv(const ::hermes::vm::RuntimeConfig &runtimeConfig)
    : hermesRuntimeSharedPointer(
          ::facebook::hermes::makeHermesRuntime(runtimeConfig)),
      hermesRuntime(*this->hermesRuntimeSharedPointer),
      runtime(::facebook::hermes::HermesRuntime::getHermesRuntimeFromJSI(
          this->hermesRuntime)) {
  // 0.8.x 版本开始会执行 runInternalBytecode -> runBytecode ->
  // clearThrownValue，0.7.2 版本没有执行，需要手动执行清空
  // Runtime.h 文件定义了 PinnedHermesValue thrownValue_ =
  // {}
  // => undefined
  LIST_INIT(&this->valueList);
  LIST_INIT(&this->weakRefList);
  LIST_INIT(&this->strongRefList);
  SLIST_INIT(&this->handleScopeList);

  this->runtime.addCustomRootsFunction(
      [this](::hermes::vm::GC *, ::hermes::vm::RootAcceptor &rootAcceptor) {
        NAPIRef ref;
        LIST_FOREACH(ref, &this->strongRefList, node) {
          rootAcceptor.accept(
              ::std::get<::hermes::vm::PinnedHermesValue>(ref->storage));
        }
      });
  this->runtime.addCustomWeakRootsFunction(
      [this](::hermes::vm::GC *,
             ::hermes::vm::WeakRootAcceptor &weakRootAcceptor) {
        NAPIRef ref;
        LIST_FOREACH(ref, &this->weakRefList, node) {
          weakRootAcceptor.acceptWeak(
              ::std::get<::hermes::vm::WeakRoot<::hermes::vm::JSObject>>(
                  ref->storage));
        }
      });
}

OpaqueNAPIEnv::~OpaqueNAPIEnv() {
  this->disableDebugger();

  {
    NAPIHandleScope handleScope, temp;
    SLIST_FOREACH_SAFE(handleScope, &this->handleScopeList, node, temp) {
      delete handleScope;
    }
  }
  NAPIRef ref, temp;
  LIST_FOREACH_SAFE(ref, &this->valueList, node, temp) { delete ref; }
  LIST_FOREACH_SAFE(ref, &this->strongRefList, node, temp) { delete ref; }
  LIST_FOREACH_SAFE(ref, &this->weakRefList, node, temp) { delete ref; }
}

::hermes::vm::Runtime &OpaqueNAPIEnv::getRuntime() const {
  return this->runtime;
}

#ifdef HERMES_ENABLE_DEBUGGER

void OpaqueNAPIEnv::enableDebugger(const char *debuggerTitle,
                                   bool waitForDebugger) const {
  auto adapter = ::std::make_unique<::orangelabs::HermesExecutorRuntimeAdapter>(
      this->hermesRuntimeSharedPointer, this->hermesRuntime,
      this->messageQueueThread);
  ::std::string debuggerTitleString =
      debuggerTitle ? debuggerTitle : "N-API Hermes";
  ::orangelab::hermes::inspector::chrome::enableDebugging(
      ::std::move(adapter), debuggerTitleString, waitForDebugger);
}

void OpaqueNAPIEnv::disableDebugger() const {
  ::orangelab::hermes::inspector::chrome::disableDebugging(this->hermesRuntime);
}

void OpaqueNAPIEnv::setMessageQueueThread(
    ::std::shared_ptr<::facebook::react::MessageQueueThread> jsQueue) {
  this->messageQueueThread = ::std::move(jsQueue);
}

#endif
