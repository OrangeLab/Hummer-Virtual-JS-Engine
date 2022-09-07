#include <hermes/External.h>
#include <hermes/VM/JSArray.h>

orangelabs::External::External(::hermes::vm::Runtime &runtime, void *data,
                               const NAPIFinalize finalizeCallback,
                               void *finalizeHint)
    : runtime(runtime),
      data(data),
      finalizeCallback(finalizeCallback),
      finalizeHint(finalizeHint) {}

void *orangelabs::External::getData() const { return this->data; }

orangelabs::External::~External() {
  if (this->finalizeCallback) {
    this->finalizeCallback(this->data, this->finalizeHint);
  }
}

::hermes::vm::CallResult<::hermes::vm::HermesValue> orangelabs::External::get(
    ::hermes::vm::SymbolID symbolId) {
  return ::hermes::vm::Runtime::getUndefinedValue().get();
}

::hermes::vm::CallResult<bool> orangelabs::External::set(
    ::hermes::vm::SymbolID symbolId, ::hermes::vm::HermesValue value) {
  return false;
}

::hermes::vm::CallResult<::hermes::vm::Handle<::hermes::vm::JSArray>>
orangelabs::External::getHostPropertyNames() {
  return ::hermes::vm::JSArray::create(this->runtime, 0, 0);
}