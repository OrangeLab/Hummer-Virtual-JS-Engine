#include <hermes/BCGen/HBC/BytecodeProviderFromSrc.h>
#include <hermes/Public/GCConfig.h>
#include <hermes/VM/Callable.h>
#include <hermes/VM/GCBase.h>
#include <hermes/VM/HostModel.h>
#include <hermes/VM/JSArray.h>
#include <hermes/VM/Operations.h>
#include <hermes/VM/Runtime.h>
#include <hermes/VM/StringPrimitive.h>
#include <hermes/VM/WeakRef.h>
#include <hermes/hermes.h>
#include <jsi/decorator.h>
#include <llvh/Support/ConvertUTF.h>
#include <napi/js_native_api.h>
#include <sys/queue.h>

#ifdef HERMES_ENABLE_DEBUGGER
#include <hermes/inspector/RuntimeAdapter.h>
#include <hermes/inspector/chrome/Registration.h>
#endif

#ifndef LIST_FOREACH_SAFE
#define LIST_FOREACH_SAFE(var, head, field, tvar)                                                                      \
    for ((var) = LIST_FIRST((head)); (var) && ((tvar) = LIST_NEXT((var), field), 1); (var) = (tvar))
#endif

#include <utility>

#define RETURN_STATUS_IF_FALSE(condition, status)                                                                      \
    if (!(condition))                                                                                                  \
    {                                                                                                                  \
        return status;                                                                                                 \
    }

#define CHECK_ARG(arg)                                                                                                 \
    if (!arg)                                                                                                          \
    {                                                                                                                  \
        assert(false);                                                                                                 \
    }

#define CHECK_NAPI(expr)                                                                                               \
    {                                                                                                                  \
        NAPIStatus status = expr;                                                                                      \
        if (status != NAPIOK)                                                                                          \
        {                                                                                                              \
            return status;                                                                                             \
        }                                                                                                              \
    }

#define CHECK_HERMES(expr)                                                                                             \
    if ((expr) == ::hermes::vm::ExecutionStatus::EXCEPTION)                                                            \
    {                                                                                                                  \
        return NAPIPendingException;                                                                                   \
    }

#define NAPI_PREAMBLE(env)                                                                                             \
    CHECK_ARG(env)                                                                                                     \
    CHECK_ARG(env->getRuntime())                                                                                       \
    RETURN_STATUS_IF_FALSE(env->getRuntime()->getThrownValue().isEmpty(), NAPIPendingException)

namespace
{
class External final : public ::hermes::vm::HostObjectProxy
{
  public:
    External(void *data, NAPIFinalize finalizeCallback, void *finalizeHint);

    [[nodiscard]] void *getData() const;

    ~External() override;

    ::hermes::vm::CallResult<::hermes::vm::HermesValue> get(::hermes::vm::SymbolID symbolId) override;

    ::hermes::vm::CallResult<bool> set(::hermes::vm::SymbolID symbolId, ::hermes::vm::HermesValue value) override;

    ::hermes::vm::CallResult<::hermes::vm::Handle<::hermes::vm::JSArray>> getHostPropertyNames() override;

    // copy ctor
    External(const External &) = delete;

    // move ctor
    External(External &&) = delete;

    // copy assign
    External &operator=(const External &) = delete;

    // move assign
    External &operator=(External &&) = delete;

  private:
    void *data;
    NAPIFinalize finalizeCallback;
    void *finalizeHint;
};

// hermes.cpp -> kMaxNumRegisters
constexpr unsigned kMaxNumRegisters =
    (512 * 1024 - sizeof(::hermes::vm::Runtime) - 4096 * 8) / sizeof(::hermes::vm::PinnedHermesValue);

#ifdef HERMES_ENABLE_DEBUGGER

class HermesExecutorRuntimeAdapter : public ::facebook::hermes::inspector::RuntimeAdapter
{
  public:
    HermesExecutorRuntimeAdapter(std::shared_ptr<::facebook::jsi::Runtime> runtime,
                                 ::facebook::hermes::HermesRuntime &hermesRuntime)
        : runtime_(std::move(runtime)), hermesRuntime_(hermesRuntime)
    {
    }

    ~HermesExecutorRuntimeAdapter() override = default;

    ::facebook::jsi::Runtime &getRuntime() override
    {
        return *runtime_;
    }

    ::facebook::hermes::debugger::Debugger &getDebugger() override
    {
        return hermesRuntime_.getDebugger();
    }

  private:
    std::shared_ptr<::facebook::jsi::Runtime> runtime_;
    ::facebook::hermes::HermesRuntime &hermesRuntime_;
};

#endif

struct ReentrancyCheck
{
// This is effectively a very subtle and complex assert, so only
// include it in builds which would include asserts.
#ifndef NDEBUG
    ReentrancyCheck() : tid(std::thread::id()), depth(0)
    {
    }

    void before()
    {
        std::thread::id this_id = std::this_thread::get_id();
        std::thread::id expected = std::thread::id();

        // A note on memory ordering: the main purpose of these checks is
        // to observe a before/before race, without an intervening after.
        // This will be detected by the compare_exchange_strong atomicity
        // properties, regardless of memory order.
        //
        // For everything else, it is easiest to think of 'depth' as a
        // proxy for any access made inside the VM.  If access to depth
        // are reordered incorrectly, the same could be true of any other
        // operation made by the VM.  In fact, using acquire/release
        // memory ordering could create barriers which mask a programmer
        // error.  So, we use relaxed memory order, to avoid masking
        // actual ordering errors.  Although, in practice, ordering errors
        // of this sort would be surprising, because the decorator would
        // need to call after() without before().

        if (tid.compare_exchange_strong(expected, this_id, std::memory_order_relaxed))
        {
            // Returns true if tid and expected were the same.  If they
            // were, then the stored tid referred to no thread, and we
            // atomically saved this thread's tid.  Now increment depth.
            assert(depth == 0 && "No thread id, but depth != 0");
            ++depth;
        }
        else if (expected == this_id)
        {
            // If the stored tid referred to a thread, expected was set to
            // that value.  If that value is this thread's tid, that's ok,
            // just increment depth again.
            assert(depth != 0 && "Thread id was set, but depth == 0");
            ++depth;
        }
        else
        {
            // The stored tid was some other thread.  This indicates a bad
            // programmer error, where VM methods were called on two
            // different threads unsafely.  Fail fast (and hard) so the
            // crash can be analyzed.
            __builtin_trap();
        }
    }

    void after()
    {
        assert(tid.load(std::memory_order_relaxed) == std::this_thread::get_id() && "No thread id in after()");
        if (--depth == 0)
        {
            // If we decremented depth to zero, store no-thread into tid.
            std::thread::id expected = std::this_thread::get_id();
            bool didWrite = tid.compare_exchange_strong(expected, std::thread::id(), std::memory_order_relaxed);
            assert(didWrite && "Decremented to zero, but no tid write");
        }
    }

    std::atomic<std::thread::id> tid;
    // This is not atomic, as it is only written or read from the owning
    // thread.
    unsigned int depth;
#endif
};

// This adds ReentrancyCheck and debugger enable/teardown to the given
// Runtime.
class DecoratedRuntime : public ::facebook::jsi::WithRuntimeDecorator<ReentrancyCheck>
{
  public:
    // The first argument may be another decorator which itself
    // decorates the real HermesRuntime, depending on the build config.
    // The second argument is the real HermesRuntime as well to
    // manage the debugger registration.
    DecoratedRuntime(std::unique_ptr<Runtime> runtime, ::facebook::hermes::HermesRuntime &hermesRuntime)
        : ::facebook::jsi::WithRuntimeDecorator<ReentrancyCheck>(*runtime, reentrancyCheck_),
          runtime_(std::move(runtime)), hermesRuntime_(hermesRuntime)
    {
        //#ifdef HERMES_ENABLE_DEBUGGER
        //        auto adapter = std::make_unique<HermesExecutorRuntimeAdapter>(runtime_, hermesRuntime_);
        //        ::std::string debuggerTitleString = debuggerTitle ? debuggerTitle : "Hummer Hermes";
        //        debuggerTitleString.append(" - React");
        //        facebook::hermes::inspector::chrome::enableDebugging(std::move(adapter), debuggerTitleString);
        //#else
        //        (void)hermesRuntime_;
        //#endif
    }

    ~DecoratedRuntime() override
    {
        disableDebugger();
    }

    void enableDebugger(const char *debuggerTitle)
    {
#ifdef HERMES_ENABLE_DEBUGGER
        auto adapter = std::make_unique<HermesExecutorRuntimeAdapter>(runtime_, hermesRuntime_);
        ::std::string debuggerTitleString = debuggerTitle ? debuggerTitle : "Hummer Hermes";
        debuggerTitleString.append(" - React");
        facebook::hermes::inspector::chrome::enableDebugging(std::move(adapter), debuggerTitleString);
#endif
    }

    void disableDebugger()
    {
#ifdef HERMES_ENABLE_DEBUGGER
        facebook::hermes::inspector::chrome::disableDebugging(hermesRuntime_);
#endif
    }

  private:
    // runtime_ is a potentially decorated Runtime.
    // hermesRuntime is a reference to a HermesRuntime managed by runtime_.
    //
    // HermesExecutorRuntimeAdapter requirements are kept, because the
    // dtor will disable debugging on the HermesRuntime before the
    // member managing it is destroyed.

    std::shared_ptr<Runtime> runtime_;
    ReentrancyCheck reentrancyCheck_;
    ::facebook::hermes::HermesRuntime &hermesRuntime_;
};
} // namespace

::External::External(void *data, NAPIFinalize finalizeCallback, void *finalizeHint)
    : data(data), finalizeCallback(finalizeCallback), finalizeHint(finalizeHint)
{
}

void *External::getData() const
{
    return data;
}
External::~External()
{
    if (finalizeCallback)
    {
        finalizeCallback(data, finalizeHint);
    }
}
::hermes::vm::CallResult<::hermes::vm::HermesValue> External::get(::hermes::vm::SymbolID symbolId)
{
    return ::hermes::vm::Runtime::getUndefinedValue().get();
}
::hermes::vm::CallResult<bool> External::set(::hermes::vm::SymbolID symbolId, ::hermes::vm::HermesValue value)
{
    return false;
}
::hermes::vm::CallResult<::hermes::vm::Handle<::hermes::vm::JSArray>> External::getHostPropertyNames()
{
    return ::hermes::vm::Runtime::makeNullHandle<::hermes::vm::JSArray>();
}

EXTERN_C_START

struct OpaqueNAPIRef;

struct OpaqueNAPIEnv final
{
    explicit OpaqueNAPIEnv();

    ~OpaqueNAPIEnv();

    [[nodiscard]] ::hermes::vm::Runtime *getRuntime() const
    {
        return runtime;
    }

    OpaqueNAPIEnv(const OpaqueNAPIEnv &) = delete;

    OpaqueNAPIEnv(OpaqueNAPIEnv &&) = delete;

    OpaqueNAPIEnv &operator=(const OpaqueNAPIEnv &) = delete;

    OpaqueNAPIEnv &operator=(OpaqueNAPIEnv &&) = delete;

    // TODO(ChasonTang): 析构函数移除所有链表内容

    LIST_HEAD(, OpaqueNAPIRef) valueList;

    LIST_HEAD(, OpaqueNAPIRef) weakRefList;

    LIST_HEAD(, OpaqueNAPIRef) strongRefList;

    void enableDebugger(const char *debuggerTitle);

    void disableDebugger();

  private:
    std::shared_ptr<DecoratedRuntime> decoratedRuntime;
    ::hermes::vm::Runtime *runtime;
};

void OpaqueNAPIEnv::enableDebugger(const char *debuggerTitle)
{
    decoratedRuntime->enableDebugger(debuggerTitle);
}

void OpaqueNAPIEnv::disableDebugger()
{
    decoratedRuntime->disableDebugger();
}

// !referenceCount && !isObject => (undefined, 0) => nullptr
// referenceCount > 0 => 强引用 => addStrong
// !referenceCount && isObject => weakRef => addWeak

// 1 + ref => undefined 强引用 => addStrong
// 2 + ref => 强引用
// (2 + unref) && isObject => weakRef => removeStrong + addWeak
// (2 + unref) && !isObject => (undefined, 0) => nullptr => removeStrong
// (3 + ref) && isValid => 强引用 => removeWeak + addStrong
// (3 + ref) && !isValid => undefined 强引用 => removeWeak + addStrong + isObject = false;

struct OpaqueNAPIRef final
{
    LIST_ENTRY(OpaqueNAPIRef) node;

    union {
        ::hermes::vm::WeakRef<::hermes::vm::HermesValue> hermesValueWeakRef; // size_t
        ::hermes::vm::PinnedHermesValue pinnedHermesValue;                   // 64
    };
    [[nodiscard]] const ::hermes::vm::PinnedHermesValue *getHermesValue() const
    {
        if (!referenceCount && !isObject)
        {
            return nullptr;
        }
        else if (referenceCount)
        {
            return &pinnedHermesValue;
        }
        else
        {
            auto hermesValueHandleOptional = hermesValueWeakRef.get(env->getRuntime(), &env->getRuntime()->getHeap());
            if (hermesValueHandleOptional.hasValue())
            {
                return hermesValueHandleOptional.getValue().unsafeGetPinnedHermesValue();
            }
            else
            {
                return nullptr;
            }
        }
    }

  private:
    NAPIEnv env;
    uint8_t referenceCount;
    bool isObject;

  public:
    OpaqueNAPIRef(NAPIEnv env, const ::hermes::vm::PinnedHermesValue &pinnedHermesValue, uint8_t referenceCount)
        : env(env), referenceCount(referenceCount), isObject(pinnedHermesValue.isObject())
    {
        // 标量 && 弱引用
        if (!referenceCount && !isObject)
        {
            LIST_INSERT_HEAD(&env->valueList, this, node);
            this->pinnedHermesValue = *::hermes::vm::Runtime::getUndefinedValue().unsafeGetPinnedHermesValue();
        }
        else if (referenceCount)
        {
            // 强引用
            this->pinnedHermesValue = pinnedHermesValue;
            LIST_INSERT_HEAD(&env->strongRefList, this, node);
        }
        else
        {
            // 对象 && 弱引用
            // addWeak
            // runtime->getHeap() mutable，因此不能使用 const ::hermes::vm::Runtime *
            this->hermesValueWeakRef =
                ::hermes::vm::WeakRef<::hermes::vm::HermesValue>(&env->getRuntime()->getHeap(), pinnedHermesValue);
            LIST_INSERT_HEAD(&env->weakRefList, this, node);
        }
    }
    OpaqueNAPIRef(const OpaqueNAPIRef &) = delete;

    OpaqueNAPIRef(OpaqueNAPIRef &&) = delete;

    OpaqueNAPIRef &operator=(const OpaqueNAPIRef &) = delete;

    OpaqueNAPIRef &operator=(OpaqueNAPIRef &&) = delete;

    ~OpaqueNAPIRef()
    {
        //        if (referenceCount || isObject)
        //        {
        LIST_REMOVE(this, node);
        //        }
    }
    void ref()
    {
        LIST_REMOVE(this, node);
        if (!referenceCount && !isObject)
        {
            LIST_INSERT_HEAD(&env->strongRefList, this, node);
        }
        else if (isObject && !referenceCount)
        {
            auto hermesValueOptional = hermesValueWeakRef.unsafeGetOptional(&env->getRuntime()->getHeap());
            if (hermesValueOptional.hasValue())
            {
                pinnedHermesValue = hermesValueOptional.getValue();
            }
            else
            {
                pinnedHermesValue = *::hermes::vm::Runtime::getUndefinedValue().unsafeGetPinnedHermesValue();
                isObject = false;
            }
            //            LIST_REMOVE(this, node);
            LIST_INSERT_HEAD(&env->strongRefList, this, node);
        }
        ++referenceCount;
    }
    void unref()
    {
        assert(referenceCount);
        if (referenceCount == 1)
        {
            LIST_REMOVE(this, node);
            if (isObject)
            {
                hermesValueWeakRef =
                    ::hermes::vm::WeakRef<::hermes::vm::HermesValue>(&env->getRuntime()->getHeap(), pinnedHermesValue);
                LIST_INSERT_HEAD(&env->weakRefList, this, node);
            }
            else
            {
                pinnedHermesValue = *::hermes::vm::Runtime::getUndefinedValue().unsafeGetPinnedHermesValue();
                LIST_INSERT_HEAD(&env->valueList, this, node);
            }
        }
        --referenceCount;
    }
    [[nodiscard]] uint8_t getReferenceCount() const
    {
        return referenceCount;
    }
};

EXTERN_C_END

OpaqueNAPIEnv::~OpaqueNAPIEnv()
{
    NAPIRef ref, temp;
    LIST_FOREACH_SAFE(ref, &valueList, node, temp)
    {
        //        LIST_REMOVE(ref, node);
        delete ref;
    }
    LIST_FOREACH_SAFE(ref, &strongRefList, node, temp)
    {
        //        LIST_REMOVE(ref, node);
        delete ref;
    }
    LIST_FOREACH_SAFE(ref, &weakRefList, node, temp)
    {
        //        LIST_REMOVE(ref, node);
        delete ref;
    }
}

OpaqueNAPIEnv::OpaqueNAPIEnv()
{
    // HermesExecutorFactory -> heapSizeMB 1024 -> HermesExecutor -> initHybrid
    // https://github.com/facebook/react-native/blob/v0.64.2/ReactAndroid/src/main/java/com/facebook/hermes/reactexecutor/OnLoad.cpp#L85
    // https://github.com/facebook/react-native/blob/v0.64.2/ReactAndroid/src/main/java/com/facebook/hermes/reactexecutor/OnLoad.cpp#L33
    auto gcConfigBuilder = ::hermes::vm::GCConfig::Builder()
                               .withName("N-API")
                               //                               .withAllocInYoung(false)
                               //                               .withRevertToYGAtTTI(true)
                               .withMaxHeapSize(1024 << 20);
    auto runtimeConfig = ::hermes::vm::RuntimeConfig::Builder()
                             .withGCConfig(gcConfigBuilder.build())
                             //                             .withRegisterStack(nullptr)
                             .withMaxNumRegisters(kMaxNumRegisters)
                             .build();
    std::unique_ptr<::facebook::hermes::HermesRuntime> hermesRuntime =
        ::facebook::hermes::makeHermesRuntime(runtimeConfig);
    ::facebook::hermes::HermesRuntime &hermesRuntimeRef = *hermesRuntime;
    decoratedRuntime = std::make_shared<DecoratedRuntime>(std::move(hermesRuntime), hermesRuntimeRef);
    runtime = facebook::hermes::HermesRuntime::getHermesRuntimeFromJSI(&hermesRuntimeRef);
    // 0.8.x 版本开始会执行 runInternalBytecode -> runBytecode -> clearThrownValue，0.7.2 版本没有执行，需要手动执行清空
    // RuntimeHermesValueFields.def 文件定义了 PinnedHermesValue thrownValue_ = {} => undefined
    //    assert(!runtime->getThrownValue().isUndefined());
    //    runtime->clearThrownValue();
    LIST_INIT(&valueList);
    LIST_INIT(&weakRefList);
    LIST_INIT(&strongRefList);
    runtime->addCustomRootsFunction([this](::hermes::vm::GC *, ::hermes::vm::SlotAcceptor &slotAcceptor) {
        NAPIRef ref;
        LIST_FOREACH(ref, &this->valueList, node)
        {
            slotAcceptor.accept(ref->pinnedHermesValue);
        }
    });
    runtime->addCustomWeakRootsFunction([this](::hermes::vm::GC *, ::hermes::vm::WeakRefAcceptor &weakRefAcceptor) {
        NAPIRef ref;
        LIST_FOREACH(ref, &this->weakRefList, node)
        {
            weakRefAcceptor.accept(ref->hermesValueWeakRef);
        }
    });
}

namespace
{

class FunctionInfo final
{
  public:
    FunctionInfo(NAPIEnv env, NAPICallback callback, void *data) : env(env), callback(callback), data(data)
    {
    }
    [[nodiscard]] NAPIEnv getEnv() const
    {
        return env;
    }
    [[nodiscard]] NAPICallback getCallback() const
    {
        return callback;
    }
    [[nodiscard]] void *getData() const
    {
        return data;
    }

    FunctionInfo(const FunctionInfo &) = delete;

    FunctionInfo(FunctionInfo &&) = delete;

    FunctionInfo &operator=(const FunctionInfo &) = delete;

    FunctionInfo &operator=(FunctionInfo &&) = delete;

  private:
    NAPIEnv env;
    NAPICallback callback;
    void *data;
};
} // namespace

EXTERN_C_START

struct OpaqueNAPICallbackInfo
{
    OpaqueNAPICallbackInfo(const hermes::vm::NativeArgs &nativeArgs, void *data) : nativeArgs(nativeArgs), data(data)
    {
    }
    [[nodiscard]] const hermes::vm::NativeArgs &getNativeArgs() const
    {
        return nativeArgs;
    }
    [[nodiscard]] void *getData() const
    {
        return data;
    }

    OpaqueNAPICallbackInfo(const OpaqueNAPICallbackInfo &) = delete;

    OpaqueNAPICallbackInfo(OpaqueNAPICallbackInfo &&) = delete;

    OpaqueNAPICallbackInfo &operator=(const OpaqueNAPICallbackInfo &) = delete;

    OpaqueNAPICallbackInfo &operator=(OpaqueNAPICallbackInfo &&) = delete;

  private:
    ::hermes::vm::NativeArgs nativeArgs;
    void *data;
};

EXTERN_C_END

NAPIStatus napi_get_undefined(NAPIEnv /*env*/, NAPIValue *result)
{
    CHECK_ARG(result)

    *result = (NAPIValue)::hermes::vm::Runtime::getUndefinedValue().unsafeGetPinnedHermesValue();

    return NAPIOK;
}

NAPIStatus napi_get_null(NAPIEnv /*env*/, NAPIValue *result)
{
    CHECK_ARG(result)

    *result = (NAPIValue)::hermes::vm::Runtime::getNullValue().unsafeGetPinnedHermesValue();

    return NAPIOK;
}

NAPIStatus napi_get_global(NAPIEnv env, NAPIValue *result)
{
    CHECK_ARG(env)
    CHECK_ARG(result)

    *result = (NAPIValue)env->getRuntime()->getGlobal().unsafeGetPinnedHermesValue();

    return NAPIOK;
}

NAPIStatus napi_get_boolean(NAPIEnv /*env*/, bool value, NAPIValue *result)
{
    CHECK_ARG(result)

    *result = (NAPIValue)::hermes::vm::Runtime::getBoolValue(value).unsafeGetPinnedHermesValue();

    return NAPIOK;
}

NAPIStatus napi_create_double(NAPIEnv env, double value, NAPIValue *result)
{
    CHECK_ARG(env)
    CHECK_ARG(result)

    // HermesValue.h -> encodeNumberValue
    *result = (NAPIValue)env->getRuntime()
                  ->makeHandle(::hermes::vm::HermesValue::encodeNumberValue(value))
                  .unsafeGetPinnedHermesValue();

    return NAPIOK;
}

NAPIStatus napi_create_string_utf8(NAPIEnv env, const char *str, NAPIValue *result)
{
    NAPI_PREAMBLE(env)
    CHECK_ARG(result)

    if (!str)
    {
        str = "";
    }
    size_t length = strlen(str);
    ::hermes::vm::CallResult<::hermes::vm::HermesValue> callResult = ::hermes::vm::ExecutionStatus::EXCEPTION;
    if (::hermes::isAllASCII(str, str + length))
    {
        // length 0 也算 ASCII，但是 ::hermes::vm::createASCIIRef 不能传入 nullptr，因此不能直接使用 (nullptr, 0) 构造
        callResult =
            ::hermes::vm::StringPrimitive::createEfficient(env->getRuntime(), ::hermes::vm::createASCIIRef(str));
    }
    else
    {
        // 多一个 \0
        auto out = static_cast<uint16_t *>(malloc(sizeof(uint16_t) * (length + 1)));
        RETURN_STATUS_IF_FALSE(out, NAPIMemoryError)
        // ::std::u16string resize 失败会抛出异常，因此使用 malloc
        auto sourceStart = (const ::llvh::UTF8 *)str;
        auto sourceEnd = sourceStart + length;
        auto targetStart = out;
        auto targetEnd = targetStart + length;
        ::llvh::ConversionResult conversionResult;
        conversionResult =
            ::llvh::ConvertUTF8toUTF16(&sourceStart, sourceEnd, &targetStart, targetEnd - 1, ::llvh::lenientConversion);
        if (conversionResult == ::llvh::ConversionResult::targetExhausted)
        {
            assert(false);
            free(out);

            return NAPIMemoryError;
        }
        else if (conversionResult != ::llvh::ConversionResult::conversionOK)
        {
            free(out);

            return NAPIMemoryError;
        }
        *targetStart = '\0';
        // ::std::u16string 会作为后备存储使用并调用 ::std::move，但是这里不是
        callResult = ::hermes::vm::StringPrimitive::createEfficient(
            env->getRuntime(), ::hermes::vm::createUTF16Ref(reinterpret_cast<const char16_t *>(out)));
        free(out);
    }
    CHECK_HERMES(callResult)
    *result = (NAPIValue)env->getRuntime()->makeHandle(callResult.getValue()).unsafeGetPinnedHermesValue();

    return NAPIOK;
}

NAPIStatus napi_create_function(NAPIEnv env, const char *utf8name, NAPICallback callback, void *data, NAPIValue *result)
{
    NAPI_PREAMBLE(env)
    CHECK_ARG(callback)
    CHECK_ARG(result)

    NAPIValue stringValue;
    CHECK_NAPI(napi_create_string_utf8(env, utf8name, &stringValue))
    auto stringPrimitive = ::hermes::vm::dyn_vmcast_or_null<::hermes::vm::StringPrimitive>(
        *(const ::hermes::vm::PinnedHermesValue *)stringValue);
    RETURN_STATUS_IF_FALSE(stringPrimitive, NAPIMemoryError)
    auto callResult =
        ::hermes::vm::stringToSymbolID(env->getRuntime(), ::hermes::vm::createPseudoHandle(stringPrimitive));
    CHECK_HERMES(callResult)
    auto symbolId = callResult.getValue().get();
    auto functionInfo = new (::std::nothrow) FunctionInfo(env, callback, data);
    RETURN_STATUS_IF_FALSE(functionInfo, NAPIMemoryError)
    ::hermes::vm::NativeFunctionPtr nativeFunctionPtr =
        [](void *context, ::hermes::vm::Runtime *runtime,
           ::hermes::vm::NativeArgs args) -> ::hermes::vm::CallResult<::hermes::vm::HermesValue> {
        ::hermes::vm::GCScope gcScope(runtime);
        auto innerFunctionInfo = (FunctionInfo *)context;
        if (!innerFunctionInfo->getCallback() || !innerFunctionInfo->getEnv())
        {
            assert(false);

            return ::hermes::vm::CallResult<::hermes::vm::HermesValue>(
                ::hermes::vm::Runtime::getUndefinedValue().get());
        }
        struct OpaqueNAPICallbackInfo callbackInfo(args, innerFunctionInfo->getData());
        NAPIValue returnValue = innerFunctionInfo->getCallback()(innerFunctionInfo->getEnv(), &callbackInfo);
        RETURN_STATUS_IF_FALSE(innerFunctionInfo->getEnv()->getRuntime()->getThrownValue().isEmpty(),
                               ::hermes::vm::ExecutionStatus::EXCEPTION)
        if (!returnValue)
        {
            return ::hermes::vm::CallResult<::hermes::vm::HermesValue>(
                ::hermes::vm::Runtime::getUndefinedValue().get());
        }

        return ::hermes::vm::CallResult<::hermes::vm::HermesValue>(
            *(const ::hermes::vm::PinnedHermesValue *)returnValue);
    };
    ::hermes::vm::FinalizeNativeFunctionPtr finalizeNativeFunctionPtr = [](void *context) {
        // 没有析构方法
        delete (FunctionInfo *)context;
    };
    auto functionCallResult = hermes::vm::FinalizableNativeFunction::createWithoutPrototype(
        env->getRuntime(), functionInfo, nativeFunctionPtr, finalizeNativeFunctionPtr, symbolId, 0);
    CHECK_HERMES(functionCallResult)
    *result = (NAPIValue)env->getRuntime()->makeHandle(functionCallResult.getValue()).unsafeGetPinnedHermesValue();

    return NAPIOK;
}

NAPIStatus napi_typeof(NAPIEnv /*env*/, NAPIValue value, NAPIValueType *result)
{
    CHECK_ARG(value)
    CHECK_ARG(result)

    ::hermes::vm::HermesValue hermesValue = *(const ::hermes::vm::PinnedHermesValue *)value;
    if (hermesValue.isUndefined())
    {
        *result = NAPIUndefined;
    }
    else if (hermesValue.isNull())
    {
        *result = NAPINull;
    }
    else if (hermesValue.isBool())
    {
        *result = NAPIBoolean;
    }
    else if (hermesValue.isNumber())
    {
        *result = NAPINumber;
    }
    else if (hermesValue.isString())
    {
        *result = NAPIString;
    }
    else if (hermesValue.isObject())
    {
        bool isFunction = ::hermes::vm::vmisa<::hermes::vm::Callable>(hermesValue);
        if (isFunction)
        {
            *result = NAPIFunction;
        }
        else
        {
            *result = ::hermes::vm::vmisa<::hermes::vm::HostObject>(hermesValue) ? NAPIExternal : NAPIObject;
        }
    }
    else
    {
        assert(false);

        return NAPIInvalidArg;
    }

    return NAPIOK;
}

NAPIStatus napi_get_value_double(NAPIEnv /*env*/, NAPIValue value, double *result)
{
    CHECK_ARG(value)
    CHECK_ARG(result)

    ::hermes::vm::HermesValue hermesValue = *(::hermes::vm::PinnedHermesValue *)value;
    if (hermesValue.isDouble())
    {
        *result = hermesValue.getDouble();
    }
    else
    {
        return NAPINumberExpected;
    }

    return NAPIOK;
}

NAPIStatus napi_get_value_bool(NAPIEnv /*env*/, NAPIValue value, bool *result)
{
    CHECK_ARG(value)
    CHECK_ARG(result)

    RETURN_STATUS_IF_FALSE(((const ::hermes::vm::PinnedHermesValue *)value)->isBool(), NAPIBooleanExpected)
    *result = ((const ::hermes::vm::PinnedHermesValue *)value)->getBool();

    return NAPIOK;
}

NAPIStatus napi_coerce_to_bool(NAPIEnv env, NAPIValue value, NAPIValue *result)
{
    CHECK_ARG(env)
    CHECK_ARG(value)
    CHECK_ARG(result)

    *result = (NAPIValue)env->getRuntime()
                  ->makeHandle(::hermes::vm::HermesValue::encodeBoolValue(
                      ::hermes::vm::toBoolean(*(::hermes::vm::PinnedHermesValue *)value)))
                  .unsafeGetPinnedHermesValue();

    return NAPIOK;
}

NAPIStatus napi_coerce_to_number(NAPIEnv env, NAPIValue value, NAPIValue *result)
{
    NAPI_PREAMBLE(env)
    CHECK_ARG(value)
    CHECK_ARG(result)

    auto callResult = ::hermes::vm::toNumber_RJS(
        env->getRuntime(), env->getRuntime()->makeHandle(*(const ::hermes::vm::PinnedHermesValue *)value));
    CHECK_HERMES(callResult)
    *result = (NAPIValue)env->getRuntime()->makeHandle(callResult.getValue()).unsafeGetPinnedHermesValue();

    return NAPIOK;
}

NAPIStatus napi_coerce_to_string(NAPIEnv env, NAPIValue value, NAPIValue *result)
{
    NAPI_PREAMBLE(env)
    CHECK_ARG(value)
    CHECK_ARG(result)

    auto callResult = ::hermes::vm::toString_RJS(
        env->getRuntime(), env->getRuntime()->makeHandle(*(const ::hermes::vm::PinnedHermesValue *)value));
    CHECK_HERMES(callResult)
    *result = (NAPIValue)env->getRuntime()->makeHandle(callResult->getHermesValue()).unsafeGetPinnedHermesValue();

    return NAPIOK;
}

NAPIStatus napi_set_property(NAPIEnv env, NAPIValue object, NAPIValue key, NAPIValue value)
{
    NAPI_PREAMBLE(env)
    CHECK_ARG(object)
    CHECK_ARG(key)
    CHECK_ARG(value)

    auto jsObject =
        ::hermes::vm::dyn_vmcast_or_null<::hermes::vm::JSObject>(*(::hermes::vm::PinnedHermesValue *)object);
    RETURN_STATUS_IF_FALSE(jsObject, NAPIObjectExpected)
    auto callResult = ::hermes::vm::valueToSymbolID(
        env->getRuntime(), env->getRuntime()->makeHandle(*(const ::hermes::vm::PinnedHermesValue *)key));
    CHECK_HERMES(callResult)
    auto setCallResult = ::hermes::vm::JSObject::putNamedOrIndexed(
        env->getRuntime()->makeHandle(jsObject), env->getRuntime(), callResult.getValue().get(),
        env->getRuntime()->makeHandle(*(const ::hermes::vm::PinnedHermesValue *)value));
    CHECK_HERMES(setCallResult)
    RETURN_STATUS_IF_FALSE(setCallResult.getValue(), NAPIGenericFailure)

    return NAPIOK;
}

NAPIStatus napi_has_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result)
{
    NAPI_PREAMBLE(env)
    CHECK_ARG(object)
    CHECK_ARG(key)
    CHECK_ARG(result)

    auto jsObject =
        ::hermes::vm::dyn_vmcast_or_null<::hermes::vm::JSObject>(*(::hermes::vm::PinnedHermesValue *)object);
    RETURN_STATUS_IF_FALSE(jsObject, NAPIObjectExpected)
    auto callResult = ::hermes::vm::valueToSymbolID(
        env->getRuntime(), env->getRuntime()->makeHandle(*(const ::hermes::vm::PinnedHermesValue *)key));
    CHECK_HERMES(callResult)
    auto hasCallResult = ::hermes::vm::JSObject::hasNamedOrIndexed(env->getRuntime()->makeHandle(jsObject),
                                                                   env->getRuntime(), callResult.getValue().get());
    CHECK_HERMES(hasCallResult)
    *result = hasCallResult.getValue();

    return NAPIOK;
}

NAPIStatus napi_get_property(NAPIEnv env, NAPIValue object, NAPIValue key, NAPIValue *result)
{
    NAPI_PREAMBLE(env)
    CHECK_ARG(object)
    CHECK_ARG(key)
    CHECK_ARG(result)

    auto jsObject =
        ::hermes::vm::dyn_vmcast_or_null<::hermes::vm::JSObject>(*(::hermes::vm::PinnedHermesValue *)object);
    RETURN_STATUS_IF_FALSE(jsObject, NAPIObjectExpected)
    auto callResult = ::hermes::vm::valueToSymbolID(
        env->getRuntime(), env->getRuntime()->makeHandle(*(const ::hermes::vm::PinnedHermesValue *)key));
    CHECK_HERMES(callResult)
    auto getCallResult = ::hermes::vm::JSObject::getNamedOrIndexed(env->getRuntime()->makeHandle(jsObject),
                                                                   env->getRuntime(), callResult.getValue().get());
    CHECK_HERMES(getCallResult)
    *result = (NAPIValue)env->getRuntime()->makeHandle(getCallResult.getValue().get()).unsafeGetPinnedHermesValue();

    return NAPIOK;
}

NAPIStatus napi_delete_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result)
{
    NAPI_PREAMBLE(env)
    CHECK_ARG(object)
    CHECK_ARG(key)

    RETURN_STATUS_IF_FALSE(((const ::hermes::vm::PinnedHermesValue *)key)->isDouble() ||
                               ((const ::hermes::vm::PinnedHermesValue *)key)->isString(),
                           NAPINameExpected)

    auto jsObject =
        ::hermes::vm::dyn_vmcast_or_null<::hermes::vm::JSObject>(*(::hermes::vm::PinnedHermesValue *)object);
    RETURN_STATUS_IF_FALSE(jsObject, NAPIObjectExpected)

    bool isSuccess = false;
    if (((const ::hermes::vm::PinnedHermesValue *)key)->isDouble())
    {
        isSuccess = ::hermes::vm::JSObject::deleteOwnIndexed(
            env->getRuntime()->makeHandle(jsObject), env->getRuntime(),
            (uint32_t)((const ::hermes::vm::PinnedHermesValue *)key)->getDouble());
    }
    else
    {
        auto callResult = ::hermes::vm::valueToSymbolID(
            env->getRuntime(), env->getRuntime()->makeHandle(*(const ::hermes::vm::PinnedHermesValue *)key));
        CHECK_HERMES(callResult)
        auto deleteCallResult = ::hermes::vm::JSObject::deleteNamed(env->getRuntime()->makeHandle(jsObject),
                                                                    env->getRuntime(), callResult.getValue().get());
        CHECK_HERMES(deleteCallResult)
        isSuccess = deleteCallResult.getValue();
    }
    if (result)
    {
        *result = isSuccess;
    }

    return NAPIOK;
}

NAPIStatus napi_is_array(NAPIEnv /*env*/, NAPIValue value, bool *result)
{
    CHECK_ARG(value)
    CHECK_ARG(result)

    // 虽然有 ArrayImpl，但是主要是为了提供给 Arguments 使用
    *result = ::hermes::vm::vmisa<::hermes::vm::JSArray>(*(const ::hermes::vm::PinnedHermesValue *)value);

    return NAPIOK;
}

NAPIStatus napi_call_function(NAPIEnv env, NAPIValue thisValue, NAPIValue func, size_t argc, const NAPIValue *argv,
                              NAPIValue *result)
{
    NAPI_PREAMBLE(env)
    CHECK_ARG(func)
    if (argc)
    {
        CHECK_ARG(argv)
    }

    RETURN_STATUS_IF_FALSE(::hermes::vm::vmisa<::hermes::vm::Callable>(*(const ::hermes::vm::PinnedHermesValue *)func),
                           NAPIFunctionExpected)

    // this
    if (!thisValue)
    {
        CHECK_NAPI(napi_get_global(env, &thisValue))
    }

    // 开启严格模式后，可以直接传入空句柄
    auto callResult = ::hermes::vm::Arguments::create(
        env->getRuntime(), argc, ::hermes::vm::HandleRootOwner::makeNullHandle<::hermes::vm::Callable>(), true);
    CHECK_HERMES(callResult)
    for (size_t i = 0; i < argc; ++i)
    {
        ::hermes::vm::ArrayImpl::setElementAt(
            callResult.getValue(), env->getRuntime(), i,
            env->getRuntime()->makeHandle(*(const ::hermes::vm::PinnedHermesValue *)argv[i]));
    }
    auto function =
        ::hermes::vm::dyn_vmcast_or_null<::hermes::vm::Callable>(*(const ::hermes::vm::PinnedHermesValue *)func);
    auto executeCallResult = ::hermes::vm::Callable::executeCall(
        env->getRuntime()->makeHandle(function), env->getRuntime(), ::hermes::vm::Runtime::getUndefinedValue(),
        env->getRuntime()->makeHandle(*(const ::hermes::vm::PinnedHermesValue *)thisValue), callResult.getValue());
    CHECK_HERMES(executeCallResult)
    if (result)
    {
        *result =
            (NAPIValue)env->getRuntime()->makeHandle(executeCallResult.getValue().get()).unsafeGetPinnedHermesValue();
    }

    return NAPIOK;
}

NAPIStatus napi_new_instance(NAPIEnv env, NAPIValue constructor, size_t argc, const NAPIValue *argv, NAPIValue *result)
{
    NAPI_PREAMBLE(env)
    CHECK_ARG(constructor)
    if (argc)
    {
        CHECK_ARG(argv)
    }
    CHECK_ARG(result)

    RETURN_STATUS_IF_FALSE(
        ::hermes::vm::isConstructor(env->getRuntime(), *(const ::hermes::vm::PinnedHermesValue *)constructor),
        NAPIFunctionExpected)

    // 开启严格模式后，可以直接传入空句柄
    auto callResult = ::hermes::vm::Arguments::create(
        env->getRuntime(), argc, ::hermes::vm::HandleRootOwner::makeNullHandle<::hermes::vm::Callable>(), true);
    CHECK_HERMES(callResult)
    for (size_t i = 0; i < argc; ++i)
    {
        ::hermes::vm::ArrayImpl::setElementAt(
            callResult.getValue(), env->getRuntime(), i,
            env->getRuntime()->makeHandle(*(const ::hermes::vm::PinnedHermesValue *)argv[i]));
    }
    auto function =
        ::hermes::vm::dyn_vmcast_or_null<::hermes::vm::Callable>(*(const ::hermes::vm::PinnedHermesValue *)constructor);
    auto functionHandle = env->getRuntime()->makeHandle(function);
    auto thisCallResult = ::hermes::vm::Callable::createThisForConstruct(functionHandle, env->getRuntime());
    auto thisHandle = env->getRuntime()->makeHandle(thisCallResult->getHermesValue());
    CHECK_HERMES(thisCallResult)
    auto executeCallResult = ::hermes::vm::Callable::executeCall(functionHandle, env->getRuntime(), functionHandle,
                                                                 thisHandle, callResult.getValue());
    CHECK_HERMES(executeCallResult)
    if (executeCallResult.getValue()->isObject())
    {
        *result =
            (NAPIValue)env->getRuntime()->makeHandle(executeCallResult.getValue().get()).unsafeGetPinnedHermesValue();
    }
    else
    {
        *result = (NAPIValue)thisHandle.unsafeGetPinnedHermesValue();
    }

    return NAPIOK;
}

NAPIStatus napi_instanceof(NAPIEnv env, NAPIValue object, NAPIValue constructor, bool *result)
{
    NAPI_PREAMBLE(env)
    CHECK_ARG(object)
    CHECK_ARG(constructor)
    CHECK_ARG(result)

    auto callResult = ::hermes::vm::instanceOfOperator_RJS(
        env->getRuntime(), env->getRuntime()->makeHandle(*(const ::hermes::vm::PinnedHermesValue *)object),
        env->getRuntime()->makeHandle(*(const ::hermes::vm::PinnedHermesValue *)constructor));
    CHECK_HERMES(callResult)
    *result = callResult.getValue();

    return NAPIOK;
}

NAPIStatus napi_get_cb_info(NAPIEnv env, NAPICallbackInfo callbackInfo, size_t *argc, NAPIValue *argv,
                            NAPIValue *thisArg, void **data)
{
    CHECK_ARG(env)
    CHECK_ARG(callbackInfo)

    if (argv)
    {
        CHECK_ARG(argc)
        unsigned int i = 0;
        size_t min = ::std::min(callbackInfo->getNativeArgs().getArgCount(), (unsigned int)*argc);
        for (; i < min; ++i)
        {
            argv[i] = (NAPIValue)callbackInfo->getNativeArgs().getArgHandle(i).unsafeGetPinnedHermesValue();
        }
        if (i < *argc)
        {
            NAPIValue undefined = (NAPIValue)::hermes::vm::Runtime::getUndefinedValue().unsafeGetPinnedHermesValue();
            for (; i < *argc; ++i)
            {
                argv[i] = undefined;
            }
        }
    }
    if (argc)
    {
        *argc = callbackInfo->getNativeArgs().getArgCount();
    }
    if (thisArg)
    {
        *thisArg = (NAPIValue)callbackInfo->getNativeArgs().getThisHandle().unsafeGetPinnedHermesValue();
    }
    if (data)
    {
        *data = callbackInfo->getData();
    }

    return NAPIOK;
}

NAPIStatus napi_get_new_target(NAPIEnv env, NAPICallbackInfo callbackInfo, NAPIValue *result)
{
    CHECK_ARG(env)
    CHECK_ARG(callbackInfo)
    CHECK_ARG(result)

    if (callbackInfo->getNativeArgs().getNewTarget().isUndefined())
    {
        *result = nullptr;
    }
    else
    {
        *result = (NAPIValue)callbackInfo->getNativeArgs().getNewTargetHandle().unsafeGetPinnedHermesValue();
    }

    return NAPIOK;
}

NAPIStatus napi_create_external(NAPIEnv env, void *data, NAPIFinalize finalizeCB, void *finalizeHint, NAPIValue *result)
{
    NAPI_PREAMBLE(env)
    CHECK_ARG(result)

    auto hermesExternalObject = new (::std::nothrow)::External(data, finalizeCB, finalizeHint);
    RETURN_STATUS_IF_FALSE(hermesExternalObject, NAPIMemoryError)

    auto callResult = hermes::vm::HostObject::createWithoutPrototype(
        env->getRuntime(), ::std::unique_ptr<::External>(hermesExternalObject));
    CHECK_HERMES(callResult)
    *result = (NAPIValue)env->getRuntime()->makeHandle(callResult.getValue()).unsafeGetPinnedHermesValue();

    return NAPIOK;
}

NAPIStatus napi_get_value_external(NAPIEnv env, NAPIValue value, void **result)
{
    CHECK_ARG(env)
    CHECK_ARG(value)
    CHECK_ARG(result)

    auto hostObject =
        ::hermes::vm::dyn_vmcast_or_null<::hermes::vm::HostObject>(*(const ::hermes::vm::PinnedHermesValue *)value);
    RETURN_STATUS_IF_FALSE(hostObject, NAPIInvalidArg)

    // 转换失败返回空指针
    auto external = (::External *)hostObject->getProxy();
    *result = external ? external->getData() : nullptr;

    return NAPIOK;
}

// ref: 0 -> 1 => removeWeak + addStrong || ++referenceCount => none
// unref: 1 -> 0 => removeStrong + addWeak || --referenceCount => none
// weak: isInvalid => false -> true
NAPIStatus napi_create_reference(NAPIEnv env, NAPIValue value, uint32_t initialRefCount, NAPIRef *result)
{
    CHECK_ARG(env)
    CHECK_ARG(value)
    CHECK_ARG(result)

    *result = new (::std::nothrow) OpaqueNAPIRef(env, *(const ::hermes::vm::PinnedHermesValue *)value, initialRefCount);
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError)

    return NAPIOK;
}

NAPIStatus napi_delete_reference(NAPIEnv env, NAPIRef ref)
{
    CHECK_ARG(env)
    CHECK_ARG(ref)

    delete ref;

    return NAPIOK;
}

NAPIStatus napi_reference_ref(NAPIEnv env, NAPIRef ref, uint32_t *result)
{
    CHECK_ARG(env)
    CHECK_ARG(ref)

    ref->ref();
    if (result)
    {
        *result = ref->getReferenceCount();
    }

    return NAPIOK;
}

NAPIStatus napi_reference_unref(NAPIEnv env, NAPIRef ref, uint32_t *result)
{
    CHECK_ARG(env)
    CHECK_ARG(ref)

    RETURN_STATUS_IF_FALSE(ref->getReferenceCount(), NAPIGenericFailure)
    ref->unref();
    if (result)
    {
        *result = ref->getReferenceCount();
    }

    return NAPIOK;
}

NAPIStatus napi_get_reference_value(NAPIEnv env, NAPIRef ref, NAPIValue *result)
{
    CHECK_ARG(env)
    CHECK_ARG(ref)
    CHECK_ARG(result)

    *result = (NAPIValue)ref->getHermesValue();

    return NAPIOK;
}

NAPIStatus napi_open_handle_scope(NAPIEnv env, NAPIHandleScope *result)
{
    CHECK_ARG(env)
    CHECK_ARG(result)

    *result = (NAPIHandleScope) new (::std::nothrow)::hermes::vm::GCScope(env->getRuntime());
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError)

    return NAPIOK;
}

void napi_close_handle_scope(NAPIEnv env, NAPIHandleScope scope)
{
    CHECK_ARG(env)
    CHECK_ARG(scope)

    delete (::hermes::vm::GCScope *)scope;
}

EXTERN_C_START

struct OpaqueNAPIEscapableHandleScope
{
    OpaqueNAPIEscapableHandleScope() : escapeCalled(false)
    {
    }

    OpaqueNAPIEscapableHandleScope(const OpaqueNAPIEscapableHandleScope &) = delete;
    OpaqueNAPIEscapableHandleScope(OpaqueNAPIEscapableHandleScope &&) = delete;
    OpaqueNAPIEscapableHandleScope &operator=(const OpaqueNAPIEscapableHandleScope &) = delete;
    OpaqueNAPIEscapableHandleScope &operator=(OpaqueNAPIEscapableHandleScope &&) = delete;

    [[nodiscard]] bool isEscapeCalled() const
    {
        return escapeCalled;
    }

    void setEscapeCalled(bool escapeCalled1)
    {
        escapeCalled = escapeCalled1;
    }

    ::hermes::vm::GCScope *gcScope = nullptr;

  private:
    bool escapeCalled;
};

EXTERN_C_END

NAPIStatus napi_open_escapable_handle_scope(NAPIEnv env, NAPIEscapableHandleScope *result)
{
    CHECK_ARG(env)
    CHECK_ARG(result)

    RETURN_STATUS_IF_FALSE(env->getRuntime()->getTopGCScope(), NAPIHandleScopeMismatch)

    auto gcScope = new (::std::nothrow)::hermes::vm::GCScope(env->getRuntime());
    RETURN_STATUS_IF_FALSE(gcScope, NAPIMemoryError)
    *result = new (::std::nothrow) OpaqueNAPIEscapableHandleScope();
    if (!*result)
    {
        delete gcScope;

        return NAPIMemoryError;
    }
    (*result)->gcScope = gcScope;

    return NAPIOK;
}

void napi_close_escapable_handle_scope(NAPIEnv env, NAPIEscapableHandleScope scope)
{
    CHECK_ARG(env)
    CHECK_ARG(scope)

    delete scope->gcScope;
    delete scope;
}

NAPIStatus napi_escape_handle(NAPIEnv env, NAPIEscapableHandleScope scope, NAPIValue escapee, NAPIValue *result)
{
    CHECK_ARG(env)
    CHECK_ARG(scope)
    CHECK_ARG(escapee)
    CHECK_ARG(result)

    RETURN_STATUS_IF_FALSE(!scope->isEscapeCalled(), NAPIEscapeCalledTwice)

    *result = (NAPIValue)::hermes::vm::Handle<::hermes::vm::HermesValue>(
                  scope->gcScope->getParentScope(), *(const ::hermes::vm::PinnedHermesValue *)escapee)
                  .unsafeGetPinnedHermesValue();
    scope->setEscapeCalled(true);

    return NAPIOK;
}

NAPIStatus napi_throw(NAPIEnv env, NAPIValue error)
{
    NAPI_PREAMBLE(env)
    CHECK_ARG(error)

    // 直接忽略返回值
    env->getRuntime()->setThrownValue(*(::hermes::vm::PinnedHermesValue *)error);

    return NAPIOK;
}

NAPIStatus napi_get_and_clear_last_exception(NAPIEnv env, NAPIValue *result)
{
    CHECK_ARG(env)
    CHECK_ARG(result)

    if (env->getRuntime()->getThrownValue().isEmpty())
    {
        *result = (NAPIValue)::hermes::vm::Runtime::getUndefinedValue().unsafeGetPinnedHermesValue();
    }
    else
    {
        *result =
            (NAPIValue)env->getRuntime()->makeHandle(env->getRuntime()->getThrownValue()).unsafeGetPinnedHermesValue();
        env->getRuntime()->clearThrownValue();
    }

    return NAPIOK;
}

void NAPIClearLastException(NAPIEnv env)
{
    CHECK_ARG(env)

    env->getRuntime()->clearThrownValue();
}

NAPIStatus NAPIRunScript(NAPIEnv env, const char *script, const char *sourceUrl, NAPIValue *result)
{
    NAPI_PREAMBLE(env)

    ::hermes::hbc::CompileFlags compileFlags = {};
    compileFlags.lazy = true;
    compileFlags.debug = true;
    auto callResult = env->getRuntime()->run(script, sourceUrl, compileFlags);
    CHECK_HERMES(callResult)
    if (result)
    {
        *result = (NAPIValue)env->getRuntime()->makeHandle(callResult.getValue()).unsafeGetPinnedHermesValue();
    }

    return NAPIOK;
}

NAPIStatus NAPIDefineClass(NAPIEnv env, const char *utf8name, NAPICallback constructor, void *data, NAPIValue *result)
{
    NAPI_PREAMBLE(env)
    CHECK_ARG(constructor)
    CHECK_ARG(result)

    auto functionInfo = new (::std::nothrow) FunctionInfo(env, constructor, data);
    RETURN_STATUS_IF_FALSE(functionInfo, NAPIMemoryError)
    NAPIValue externalValue;
    auto finalizeCallback = [](void *finalizeData, void * /*finalizeHint*/) { delete (FunctionInfo *)finalizeData; };
    CHECK_NAPI(napi_create_external(env, functionInfo, finalizeCallback, nullptr, &externalValue))

    ::hermes::vm::NativeFunctionPtr nativeFunctionPtr =
        [](void *context, ::hermes::vm::Runtime *runtime,
           ::hermes::vm::NativeArgs args) -> ::hermes::vm::CallResult<::hermes::vm::HermesValue> {
        ::hermes::vm::GCScope gcScope(runtime);
        auto innerFunctionInfo = (FunctionInfo *)context;
        if (!innerFunctionInfo->getCallback() || !innerFunctionInfo->getEnv())
        {
            assert(false);

            return args.getThisArg();
        }
        struct OpaqueNAPICallbackInfo callbackInfo(args, innerFunctionInfo->getData());
        NAPIValue returnValue = innerFunctionInfo->getCallback()(innerFunctionInfo->getEnv(), &callbackInfo);
        RETURN_STATUS_IF_FALSE(innerFunctionInfo->getEnv()->getRuntime()->getThrownValue().isEmpty(),
                               ::hermes::vm::ExecutionStatus::EXCEPTION)
        if (!returnValue)
        {
            return args.getThisArg();
        }
        if (!((const ::hermes::vm::PinnedHermesValue *)returnValue)->isObject())
        {
            return args.getThisArg();
        }

        return ::hermes::vm::CallResult<::hermes::vm::HermesValue>(
            *(const ::hermes::vm::PinnedHermesValue *)returnValue);
    };
    auto nativeConstructor = ::hermes::vm::NativeConstructor::create(
        env->getRuntime(), ::hermes::vm::Handle<::hermes::vm::JSObject>::vmcast(&env->getRuntime()->functionPrototype),
        functionInfo, nativeFunctionPtr, 0, ::hermes::vm::NativeConstructor::creatorFunction<::hermes::vm::JSObject>,
        hermes::vm::CellKind::ObjectKind);

    NAPIValue stringValue;
    CHECK_NAPI(napi_create_string_utf8(env, utf8name, &stringValue))
    auto stringPrimitive = ::hermes::vm::dyn_vmcast_or_null<::hermes::vm::StringPrimitive>(
        *(const ::hermes::vm::PinnedHermesValue *)stringValue);
    RETURN_STATUS_IF_FALSE(stringPrimitive, NAPIMemoryError)
    auto callResult =
        ::hermes::vm::stringToSymbolID(env->getRuntime(), ::hermes::vm::createPseudoHandle(stringPrimitive));
    CHECK_HERMES(callResult)
    auto symbolId = callResult.getValue().get();
    auto rawObject = ::hermes::vm::JSObject::create(env->getRuntime());
    auto defineCallResult = ::hermes::vm::Callable::defineNameLengthAndPrototype(
        env->getRuntime()->makeHandle(nativeConstructor.get()), env->getRuntime(), symbolId, 0,
        env->getRuntime()->makeHandle(rawObject.get()), ::hermes::vm::Callable::WritablePrototype::No, false);
    CHECK_HERMES(defineCallResult)
    *result = (NAPIValue)env->getRuntime()->makeHandle(nativeConstructor.getHermesValue()).unsafeGetPinnedHermesValue();
    NAPIValue privateKeyString;
    CHECK_NAPI(napi_create_string_utf8(env, "__constructor__", &privateKeyString))
    CHECK_NAPI(napi_set_property(env, *result, privateKeyString, externalValue))

    return NAPIOK;
}

NAPIStatus NAPICreateEnv(NAPIEnv *env)
{
    CHECK_ARG(env)

    *env = new (::std::nothrow) OpaqueNAPIEnv();

    return NAPIOK;
}

void NAPIEnableDebugger(NAPIEnv env, const char *debuggerTitle)
{
    CHECK_ARG(env)

    env->enableDebugger(debuggerTitle);
}

void NAPIDisableDebugger(NAPIEnv env)
{
    CHECK_ARG(env)

    env->disableDebugger();
}

void NAPIFreeEnv(NAPIEnv env)
{
    CHECK_ARG(env)

    delete env;
}

NAPIStatus NAPIGetValueStringUTF8(NAPIEnv env, NAPIValue value, const char **result)
{
    CHECK_ARG(env)
    CHECK_ARG(value)
    CHECK_ARG(result)

    RETURN_STATUS_IF_FALSE(
        ::hermes::vm::vmisa<::hermes::vm::StringPrimitive>(*(const ::hermes::vm::PinnedHermesValue *)value),
        NAPIStringExpected)

    auto stringPrimitive = ::hermes::vm::dyn_vmcast_or_null<::hermes::vm::StringPrimitive>(
        *(const ::hermes::vm::PinnedHermesValue *)value);
    if (stringPrimitive->isASCII())
    {
        auto asciiStringRef = stringPrimitive->getStringRef<char>();
        char *buffer = static_cast<char *>(malloc(sizeof(char) * (asciiStringRef.size() + 1)));
        RETURN_STATUS_IF_FALSE(buffer, NAPIMemoryError)
        ::std::memmove(buffer, asciiStringRef.data(), asciiStringRef.size());
        buffer[asciiStringRef.size()] = '\0';
        *result = buffer;
    }
    else
    {
        auto utf16StringRef = stringPrimitive->getStringRef<char16_t>();
        auto length = utf16StringRef.size() * 3 + 1;
        char *buffer = static_cast<char *>(malloc(sizeof(char) * length));
        RETURN_STATUS_IF_FALSE(buffer, NAPIMemoryError) auto sourceStart = utf16StringRef.begin();
        char *targetStart = buffer;
        auto conversionResult = ::llvh::ConvertUTF16toUTF8(
            reinterpret_cast<const llvh::UTF16 **>(&sourceStart),
            reinterpret_cast<const llvh::UTF16 *>(utf16StringRef.end()), reinterpret_cast<llvh::UTF8 **>(&targetStart),
            reinterpret_cast<llvh::UTF8 *>(buffer + length - 1), ::llvh::strictConversion);
        if (conversionResult == ::llvh::ConversionResult::targetExhausted)
        {
            assert(false);
            free(buffer);

            return NAPIMemoryError;
        }
        else if (conversionResult != ::llvh::ConversionResult::conversionOK)
        {
            free(buffer);

            return NAPIMemoryError;
        }
        *targetStart = '\0';
        *result = buffer;
    }

    return NAPIOK;
}

NAPIStatus NAPIFreeUTF8String(NAPIEnv env, const char *cString)
{
    CHECK_ARG(env)
    CHECK_ARG(cString)

    free((void *)cString);

    return NAPIOK;
}