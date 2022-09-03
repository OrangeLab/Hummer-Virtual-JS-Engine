#include <hermes/VM/Casting.h>
#include <hermes/VM/JSObject.h>

#include <hermes/OpaqueNAPIEnv.h>
#include <hermes/OpaqueNAPIRef.h>
#include <sys/queue.h>
#include <variant>

OpaqueNAPIRef::OpaqueNAPIRef(
    NAPIEnv env, const ::hermes::vm::PinnedHermesValue &pinnedHermesValue,
    uint8_t referenceCount)
    : env(env), referenceCount(referenceCount) {
  if (!referenceCount && !pinnedHermesValue.isObject()) {
    LIST_INSERT_HEAD(&this->env->valueList, this, node);
    this->storage = ::std::monostate();
  } else if (referenceCount) {
    LIST_INSERT_HEAD(&this->env->strongRefList, this, node);
    this->storage = pinnedHermesValue;
  } else {
    LIST_INSERT_HEAD(&this->env->weakRefList, this, node);
    this->storage.emplace<::hermes::vm::WeakRoot<::hermes::vm::JSObject>>(
        ::hermes::vm::WeakRoot<::hermes::vm::JSObject>(
            ::hermes::vm::dyn_vmcast<::hermes::vm::JSObject>(pinnedHermesValue),
            this->env->getRuntime()));
  }
}

OpaqueNAPIRef::~OpaqueNAPIRef() { LIST_REMOVE(this, node); }

void OpaqueNAPIRef::ref() {
  if (!this->referenceCount) {
    LIST_REMOVE(this, node);
    LIST_INSERT_HEAD(&this->env->strongRefList, this, node);
    if (!this->storage.index()) {
      this->storage = ::hermes::vm::Runtime::getUndefinedValue().get();
    } else {
      auto weakRoot =
          ::std::get<::hermes::vm::WeakRoot<::hermes::vm::JSObject>>(
              this->storage);
      auto jsObject = weakRoot.get(this->env->getRuntime(),
                                   this->env->getRuntime().getHeap());
      if (jsObject) {
        this->storage = ::hermes::vm::HermesValue::encodeObjectValue(jsObject);
      } else {
        this->storage = ::hermes::vm::Runtime::getUndefinedValue().get();
      }
    }
  }
  ++this->referenceCount;
}

void OpaqueNAPIRef::unref() {
  assert(this->referenceCount);
  if (this->referenceCount == 1) {
    LIST_REMOVE(this, node);
    if (!::std::get<::hermes::vm::PinnedHermesValue>(this->storage)
             .isObject()) {
      LIST_INSERT_HEAD(&this->env->valueList, this, node);
      this->storage = ::std::monostate();
    } else {
      LIST_INSERT_HEAD(&this->env->weakRefList, this, node);
      this->storage.emplace<::hermes::vm::WeakRoot<::hermes::vm::JSObject>>(
          ::hermes::vm::dyn_vmcast<::hermes::vm::JSObject>(
              ::std::get<::hermes::vm::PinnedHermesValue>(this->storage)),
          this->env->getRuntime());
    }
  }
  --this->referenceCount;
}

uint8_t OpaqueNAPIRef::getReferenceCount() const {
  return this->referenceCount;
}

const ::hermes::vm::PinnedHermesValue *OpaqueNAPIRef::getHermesValue() const {
  switch (this->storage.index()) {
  case 0:
    return nullptr;
    break;
  case 1:
    return &::std::get<::hermes::vm::PinnedHermesValue>(this->storage);
    break;
  case 3: {
    auto weakRoot = ::std::get<::hermes::vm::WeakRoot<::hermes::vm::JSObject>>(
        this->storage);
    auto jsObject = weakRoot.get(this->env->getRuntime(),
                                 this->env->getRuntime().getHeap());
    if (!jsObject) {
      return nullptr;
    } else {
      auto handle =
          this->env->getRuntime().makeHandle<::hermes::vm::JSObject>(jsObject);

      return handle.unsafeGetPinnedHermesValue();
    }
  } break;

  default:
    return nullptr;

    break;
  }
}
