#include <assert.h>
#include <hermes/DebuggerAPI.h>
#include <hermes/VM/Callable.h>
#include <hermes/VM/HandleRootOwner.h>
#include <hermes/VM/HermesValue.h>
#include <hermes/VM/JSArray.h>
#include <hermes/VM/JSError.h>
#include <hermes/VM/JSObject.h>
#include <hermes/VM/Operations.h>
#include <hermes/VM/Runtime.h>
#include <hermes/VM/HostModel.h>
#include <hermes/VM/WeakRef.h>
#include <hermes/hermes.h>
#include <llvh/Support/ConvertUTF.h>
#include <napi/js_native_api.h>
#include <hermes/VM/SlotAcceptor.h>
#include <hermes/BCGen/HBC/BytecodeProviderFromSrc.h>
#include <napi/js_native_api_types.h>
#include <stdio.h>
#include <stdlib.h>
#include <list>

// 如果一个函数返回的 NAPIStatus 为 NAPIOK，则没有异常被挂起，并且不需要做任何处理
// 如果 NAPIStatus 是除了 NAPIOK 或者 NAPIPendingException，为了尝试恢复并继续执行，而不是直接返回，必须调用
// napi_is_exception_pending 来确定是否存在挂起的异常

// 大多数情况下，调用 N-API 函数的时候，如果已经有异常被挂起，函数会立即返回 NAPIPendingException，但是，N-API
// 允许一些函数被调用用来在返回 JavaScript 环境前做最小化的清理 在这种情况下，NAPIStatus
// 会反映函数的执行结果状态，而不是当前存在异常，为了避免混淆，在每次函数执行后请检查结果

// 当异常被挂起的时候，有两种方法
// 1. 进行适当的清理并返回，那么执行环境会返回到 JavaScript，作为返回到 JavaScript 的过渡的一部分，异常将在 JavaScript
// 调用原生方法的语句处抛出，大多数 N-API 函数在出现异常被挂起的时候，都只是简单的返回
// NAPIPendingException，，所以在这种情况下，建议尽量少做事情，等待返回到 JavaScript 中处理异常
// 2.
// 尝试处理异常，在原生代码能捕获到异常的地方采取适当的操作，然后继续执行。只有当异常能被安全的处理的少数特定情况下推荐这样做，在这些情况下，napi_get_and_clear_last_exception
// 可以被用来获取并清除异常，在此之后，如果发现异常还是无法处理，可以通过 napi_throw 重新抛出错误

#include <sys/queue.h>

#define RETURN_STATUS_IF_FALSE(condition, status)                                                                      \
    {                                                                                                                  \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            return status;                                                                                             \
        }                                                                                                              \
    }

#define CHECK_ARG(arg) RETURN_STATUS_IF_FALSE(arg, NAPIInvalidArg)

// This does not call napi_set_last_error because the expression
// is assumed to be a NAPI function call that already did.
#define CHECK_NAPI(expr)                                                                                               \
    {                                                                                                                  \
        NAPIStatus status = expr;                                                                                      \
        if (status != NAPIOK)                                                                                          \
        {                                                                                                              \
            return status;                                                                                             \
        }                                                                                                              \
    }
//  获取hermes js 执行异常
#define CHECK_HERMES(env, status)                                                                                      \
    {                                                                                                                  \
        CHECK_ARG(env);                                                                                                \
        if (status != hermes::vm::ExecutionStatus::RETURNED)                                                           \
        {                                                                                                              \
            CHECK_EXCEPTION(env);                                                                                      \
        }                                                                                                              \
    }                                                                                                                  \

// check throw value
#define CHECK_EXCEPTION(env)                                                                                           \
    {                                                                                                                  \
        CHECK_ARG(env);                                                                                                \
        auto hermesRt = hermesImpl::HermesRuntimeFromJsEnv(env);                                                       \
        RETURN_STATUS_IF_FALSE((hermesImpl::HermesRuntimeFromJsEnv(env)->context_->getThrownValue().isEmpty()), NAPIPendingException);   \
    }                                                                                                                  \


// #define HASH_NONFATAL_OOM 1

// 先 hashError = false; 再判断是否变为 true，发生错误需要回滚内存分配
static bool hashError = false;

#define uthash_nonfatal_oom(elt) hashError = true;


namespace hermesImpl
{

class Reference;
class Runtime;
class EscapableHandleScope;
class CallBackInfoWrapper;
class HandleScopeWrapper;

/*
 * C++'s idea of a reinterpret_cast lacks sufficient cojones.
 */
template <typename ToType, typename FromType> inline ToType hermes_bitwise_cast(FromType from)
{
    static_assert(sizeof(FromType) == sizeof(ToType), "bitwise_cast size of FromType and ToType must be equal!");
    typename std::remove_const<ToType>::type to{};
    std::memcpy(static_cast<void *>(&to), static_cast<void *>(&from), sizeof(to));
    return to;
}
inline static NAPIEscapableHandleScope JsEscapableHandleScopeFromHermesEscapableHandleScope(EscapableHandleScope* s) {
    return reinterpret_cast<NAPIEscapableHandleScope>(s);
}

inline static EscapableHandleScope* HermesEscapableHandleScopeFromJsEscapableHandleScope(
    NAPIEscapableHandleScope s) {
    return reinterpret_cast<EscapableHandleScope*>(s);
}

inline static Runtime* HermesRuntimeFromJsEnv(NAPIEnv env) {
    return reinterpret_cast<Runtime*>(env);
}

inline NAPICallbackInfo JSCallBackInfoFormHermesCallBackInfo(CallBackInfoWrapper &info)
{
    return reinterpret_cast<NAPICallbackInfo>(&info);
}

inline CallBackInfoWrapper* HermesCallBackInfoFromJSCallBackInfo(NAPICallbackInfo info)
{
    return reinterpret_cast<CallBackInfoWrapper *>(info);
}
// 复制 hermesValue 的 bit 到 NAPIValue 指针 的值(指向的地址)，
// 即便 hermes_bitwise_cast 栈上构造，出栈之后成为野指针, 也没关系，我们只需要 该地址。
inline NAPIValue JsValueFromHermesHandle(hermes::vm::Handle<hermes::vm::HermesValue> handle)
{
    return hermes_bitwise_cast<NAPIValue>(handle.get());
}

inline NAPIValue JsValueFromHermesValue(hermes::vm::HermesValue value)
{
    return hermes_bitwise_cast<NAPIValue>(value);
}

inline hermes::vm::HermesValue HermesValueFromJsValue(NAPIValue value)
{
    return hermes_bitwise_cast<hermes::vm::HermesValue>(value);
}

inline NAPIValue JsValueFromPseudoHermesHandle(hermes::vm::PseudoHandle<hermes::vm::HermesValue> handle)
{
    return hermes_bitwise_cast<NAPIValue>(handle.get());
}

inline static NAPIHandleScope JsHandleScopeFromHermesScope(HandleScopeWrapper *scope)
{
    return reinterpret_cast<NAPIHandleScope>(scope);
}

inline static HandleScopeWrapper *HermesScopeFormeJsHandleScope(NAPIHandleScope scope)
{
    return reinterpret_cast<HandleScopeWrapper *>(scope);
}

class Runtime {


  public:
    static constexpr unsigned kMaxNumRegisters =
        (512 * 1024 - sizeof(::hermes::vm::Runtime) - 4096 * 8) /
        sizeof(::hermes::vm::PinnedHermesValue);
    explicit Runtime(){
        hermes::vm::RuntimeConfig rtConfig = hermes::vm::RuntimeConfig();
        this->rt_ = hermes::vm::Runtime::create(rtConfig.rebuild().withRegisterStack(nullptr).withMaxNumRegisters(kMaxNumRegisters).build());
        this->context_ = &(*this->rt_);
        this->context_->getThrownValue().isEmpty();
    }

    inline void AddEscapableHandleScope(EscapableHandleScope *scope) {
        this->escapableHandleScopes_.emplace_front(scope);
    }

    inline void ClearEscapableHandleScope(EscapableHandleScope *scope) {
        std::list<EscapableHandleScope *>::iterator iter;
        for(iter = escapableHandleScopes_.begin(); iter != escapableHandleScopes_.end() ;iter++)
        {
            if (*iter == scope){
                escapableHandleScopes_.erase(iter);
                break;
            }
        }
        escapableHandleScopes_.emplace_front(scope);
    }


    inline void AddStrong(Reference *ref) {
        strongSet_.emplace_front(ref);
    }

    inline void AddWeak(Reference *ref) {
        weakSet_.emplace_front(ref);
    }

    inline void ClearStrong(Reference *ref) {

        std::list<Reference *>::iterator iter;
        for(iter = strongSet_.begin(); iter != strongSet_.end() ;iter++)
        {
            if (*iter == ref){
                strongSet_.erase(iter);
                break;
            }
        }
    }

    inline void ClearWeak(Reference *ref) {

        std::list<Reference *>::iterator iter;
        for(iter = weakSet_.begin(); iter != weakSet_.end() ;iter++)
        {
            if (*iter == ref){
                weakSet_.erase(iter);
                break;
            }
        }
    }
    std::shared_ptr<hermes::vm::Runtime> rt_;
    hermes::vm::Runtime *context_;
    // 默认为 HermesValue::encodeEmptyValue();
    hermes::vm::PinnedHermesValue lastException;

    int open_handle_scopes_ = 0;
    //todo: 二叉搜索树替换，提高查找效率
    std::list<Reference *> strongSet_ = {};
    std::list<Reference *> weakSet_ = {};
    std::list<EscapableHandleScope *> escapableHandleScopes_ = {};
  private:
    void addCustomGCFunc();
};

/**
 * hermes 不支持 escapable scope，使用强引用来模拟。
 * */
class EscapableHandleScope {
  public:
    explicit EscapableHandleScope()
        : hv_(hermes::vm::HermesValue()),
          escapeCalled_(false){}

    hermes::vm::PinnedHermesValue hv_;
    inline bool GetEscapeCalled(){
        return this->escapeCalled_;
    };

    inline void SetEscapeCalled(bool escapeCalled){
         this->escapeCalled_ = escapeCalled;
    };

    inline hermes::vm::PinnedHermesValue & GetHV(hermes::vm::PinnedHermesValue hv){
        return this->hv_;
    };

    inline void SetHV(const hermes::vm::PinnedHermesValue &hv){
        this->hv_ = hv;
    };

  private:
    bool escapeCalled_;
};


/*
 * 基本类型不受GC影响: 当GC进行内存压缩时，基本类型的值不会被改变。
 * JS 基本类型：number bool string symbol
 * 但 string 在 JS 引擎（hermes jsc qjs）的实现都为堆实现。
 * iOS 目前的方案由于会直接持有 JSValue，因此存在 堆内存 被回收的问题。所以需要堆 string 支持引用。
 * 后续 iOS 侧做类型识别后，NAPI 内部可修改为：只支持对象 引用
 */
class Reference {

  public:
    ~ Reference(){
        runtimeFromEnv_->ClearStrong(this);
        runtimeFromEnv_->ClearWeak(this);
    }
    explicit Reference(NAPIEnv env, const hermes::vm::PinnedHermesValue &hv, uint32_t initial_count = 0)
        : env_(env),
          runtimeFromEnv_(reinterpret_cast<Runtime *>(env)),
          hv_(hv),
          wr_(hermes::vm::WeakRoot<hermes::vm::GCCell>(hermes::vm::dyn_vmcast_or_null<hermes::vm::GCCell>(hv), runtimeFromEnv_->context_)),
          count_(initial_count){
        this->isGCCell_ = hermes::vm::dyn_vmcast_or_null<hermes::vm::GCCell>(hv) ? true : false;
        //GCCell(string & object)才创建引用，否则只为护 count
        if (isGCCell_){
            if (initial_count){
                runtimeFromEnv_->AddStrong(this);
            }else{
                if (isGCCell_){
                    runtimeFromEnv_->AddWeak(this);
                }
            }
        }
    }

    NAPIEnv env_;
    Runtime *runtimeFromEnv_;
    hermes::vm::PinnedHermesValue hv_;
    // 当非 GCCell 类进行弱引用时。会返回空指针。
    hermes::vm::WeakRoot<hermes::vm::GCCell> wr_;
    uint32_t count_;

    uint32_t Ref() {

        // promte weak to strong
        if (++this->count_ == 1 && isGCCell_)
        {
            // 1. clear weak
            runtimeFromEnv_->ClearWeak(this);
            // 2. update hermesValue
            this->UpdateHermesValueFormWeakRoot();
            // 3. set strong
            runtimeFromEnv_->AddStrong(this);
        }
        return this->count_;
    }

    uint32_t Unref() {
        // promte strong to weak
        if (--this->count_ == 0 && isGCCell_)
        {
            // 1. set weak
            runtimeFromEnv_->AddWeak(this);
            // 2. update weakRoot
            this->UpdateWeakRootFromHermesValue();
            // 3. clear strong
            runtimeFromEnv_->ClearStrong(this);
        }
        return this->count_;
    }
    //如果是弱引用，需要判断是否已析构。
    inline hermes::vm::HermesValue GetValue(){

        if (isGCCell_){
            if (count_){
                //strong
                return this->hv_;
            }else{
                //weak
                this->UpdateHermesValueFormWeakRoot();
                return this->hv_;
            }
        }
        // 基本类型直接返回
        return hv_;
    }

    inline bool IsValid(){
        //若引用指针验证
        if (isGCCell_ && count_ == 0){
            return wr_.get(runtimeFromEnv_->context_, &(runtimeFromEnv_->context_->getHeap()));
        }
        return true;
    }
  private:
    // 是否为可引用类型。基本类型除外(string可引用)
    bool isGCCell_;

    //引用发生变化时(弱<->强)，原本的hermesValue 可以能已经失效。
    //只有GCCell类型才生效
    inline void UpdateHermesValueFormWeakRoot() {

        auto ptr = this->wr_.get(runtimeFromEnv_->context_, &runtimeFromEnv_->context_->getHeap());
        this->hv_ = this->hv_.updatePointer(ptr);
    };

    inline void UpdateWeakRootFromHermesValue() {

        this->wr_.set(runtimeFromEnv_->context_, hermes::vm::dyn_vmcast_or_null<hermes::vm::GCCell>(this->hv_));
    };
};

void Runtime::addCustomGCFunc(){
    // strong ref
    context_->addCustomRootsFunction([this](hermes::vm::GC *, hermes::vm::RootAcceptor &acceptor) -> void {
      {

          //strong
          std::list<Reference *>::iterator it = this->strongSet_.begin();
          while (it != this->strongSet_.end()) {
              Reference *value = *it;
              acceptor.accept(value->hv_);
          }
      }

      {
          //escapable
          std::list<hermesImpl::EscapableHandleScope *>::iterator it = this->escapableHandleScopes_.begin();
          while (it != this->escapableHandleScopes_.end()) {
              auto value = *it;
              acceptor.accept(value->hv_);
          }
      }

    });

    //weak
    context_->addCustomWeakRootsFunction([this](hermes::vm::GC *, hermes::vm::WeakRootAcceptor &acceptor) -> void {
      std::list<hermesImpl::Reference *>::iterator it = this->weakSet_.begin();
      while (it != this->weakSet_.end()) {
          auto value = *it;
          acceptor.acceptWeak(value->wr_);
      }
    });
}

class CallBackInfoWrapper {

  public:
    inline CallBackInfoWrapper(const hermes::vm::NativeArgs &args, void* data)
        : args_(args),
          data_(data) {}

    inline hermes::vm::HermesValue GetNewTarget(){
        return this->args_.getNewTarget();
    }
    inline size_t GetArgc(){
        return this->args_.getArgCount();
    }

    inline void * GetData(){
        return this->data_;
    }

    inline hermes::vm::HermesValue GetThisArg(){

        return this->args_.getThisArg();
    }

    inline void GetArgs(NAPIValue *args, size_t bound, size_t expectedSize){
        size_t i = 0;
        for (; i < bound; ++i)
        {
            auto val = hermesImpl::JsValueFromHermesValue(this->args_.getArg(i));
            args[i] = val;
        }
        if (i < expectedSize)
        {
            for (; i < expectedSize; ++i)
            {
                args[i] = hermesImpl::JsValueFromHermesValue(hermes::vm::HermesValue::encodeUndefinedValue());
            }
        }
    }

  protected:

    const hermes::vm::NativeArgs args_;
    void* data_;
};
//保存native function，data
class FunctionInfo {

  public:
    inline FunctionInfo(NAPIEnv env, NAPICallback cb,void* data)
        : env_(env),
          cb_(cb),
          data_(data) {}

    inline NAPICallback GetCb(){
        return this->cb_;
    }
    inline void* GetData(){
        return this->data_;
    }
    inline NAPIEnv GetEnv(){
        return this->env_;
    }

  protected:
    NAPIEnv env_ = nullptr;
    NAPICallback cb_ = nullptr;
    void *data_ = nullptr;
};

class HermesExternalObject : public hermes::vm::HostObjectProxy {

    NAPIEnv env_ = nullptr;
    void *data_ = nullptr;
    NAPIFinalize finalize_cb_ = nullptr;
    void *finalize_hint_ = nullptr;
    HermesExternalObject(NAPIEnv env, void *data, NAPIFinalize finalize_cb, void *finalize_hint)
        : env_(env),
          data_(data),
          finalize_cb_(finalize_cb),
          finalize_hint_(finalize_hint){

    }

  public:
    inline void *getData() const {
        return data_;
    }
    inline NAPIEnv getEnv() const {
        return env_;
    }
    inline NAPIFinalize getFinalizeCb() const {
        return finalize_cb_;
    }
    inline void *getFinalizeHint() const {
        return finalize_hint_;
    }

    virtual ~HermesExternalObject() override {
        if (finalize_cb_){
            finalize_cb_(env_,data_,finalize_hint_);
        }
    }

    virtual hermes::vm::CallResult<hermes::vm::HermesValue> get(hermes::vm::SymbolID symbolId) override {
        return hermes::vm::CallResult<hermes::vm::HermesValue>(hermes::vm::Runtime::getUndefinedValue().get());
    }

    virtual hermes::vm::CallResult<bool> set(hermes::vm::SymbolID symbolId, hermes::vm::HermesValue value) override {
        return hermes::vm::CallResult<bool>(false);
    }

    virtual hermes::vm::CallResult<hermes::vm::Handle<hermes::vm::JSArray>> getHostPropertyNames() override {
        return hermes::vm::CallResult<hermes::vm::Handle<hermes::vm::JSArray>>(hermes::vm::Runtime::makeNullHandle<hermes::vm::JSArray>());
    }

    HermesExternalObject(const HermesExternalObject &) = delete;

    HermesExternalObject(HermesExternalObject &&) = delete;

    HermesExternalObject &operator=(const HermesExternalObject &) = delete;

    HermesExternalObject &operator=(HermesExternalObject &&) = delete;

    static std::unique_ptr<HermesExternalObject> create(NAPIEnv env, void *data, NAPIFinalize finalize_cb, void *finalize_hint) {
        return std::unique_ptr<HermesExternalObject>(new HermesExternalObject(env,data,finalize_cb,finalize_hint));
    }
};

// c++ RVO
class HandleScopeWrapper
{
  public:
    explicit HandleScopeWrapper(hermes::vm::HandleRootOwner *runtime) : gcScope(runtime,"hermes-napi", INT32_MAX)
    {
    }

  private:
    hermes::vm::GCScope gcScope;
};


// 返回指定字节长度的 对应 字符串编码类型。
// utf8 字符串
static void convertUtf8ToUtf16(const uint8_t *utf8, size_t length, std::u16string &out)
{
    // length is the number of input bytes
    out.resize(length);
    const llvh::UTF8 *sourceStart = (const llvh::UTF8 *)utf8;
    const llvh::UTF8 *sourceEnd = sourceStart + length;
    llvh::UTF16 *targetStart = (llvh::UTF16 *)&out[0];
    llvh::UTF16 *targetEnd = targetStart + out.size();
    llvh::ConversionResult cRes;
    cRes = ConvertUTF8toUTF16(&sourceStart, sourceEnd, &targetStart, targetEnd, llvh::lenientConversion);
    (void)cRes;
    assert(cRes != llvh::ConversionResult::targetExhausted && "not enough space allocated for UTF16 conversion");
    out.resize((char16_t *)targetStart - &out[0]);
}
// length(为 llvh::UTF16 数组size) 字节长度 :utf16为2字节或4字节为一个 code unit，内部做 ++计算，每次取 UTF16(2字节)。
// 参数为 ArrayRef<llvh::UTF16> 形式：unicode 数组
static void convertUtf16ToUtf8(ArrayRef<char16_t> src, size_t length, std::string &out)
{
    // length is the number of input bytes
    out.resize(length);
    const llvh::UTF16 *sourceStart = (const llvh::UTF16 *)src.data();
    const llvh::UTF16 *sourceEnd = sourceStart + length;
    llvh::UTF8 *targetStart = (llvh::UTF8 *)&out[0];
    llvh::UTF8 *targetEnd = targetStart + out.size();
    llvh::ConversionResult cRes;
    cRes = ConvertUTF16toUTF8(&sourceStart, sourceEnd, &targetStart, targetEnd, llvh::lenientConversion);
    (void)cRes;
    assert(cRes != llvh::ConversionResult::targetExhausted && "not enough space allocated for UTF16 conversion");
    out.resize((char *)targetStart - &out[0]);
}

} // namespace hermesImpl
#pragma mark <Private>
// 以下函数需要 外置 scope

#pragma mark <Error Handling>

NAPIStatus napi_throw(NAPIEnv env, NAPIValue error)
{

    CHECK_ARG(env);
    CHECK_ARG(error);

    auto hermesRt = hermesImpl::HermesRuntimeFromJsEnv(env);

    hermes::vm::GCScope gcScope();
    auto hermesVal = hermesImpl::HermesValueFromJsValue(error);
    auto jsError = hermes::vm::dyn_vmcast_or_null<hermes::vm::JSError>(hermesVal);

    if (jsError){
        hermes::vm::HandleRootOwner *rtPtr = reinterpret_cast<hermes::vm::HandleRootOwner *>(hermesRt->context_);
        auto errHandle = rtPtr->makeHandle(jsError);
        hermes::vm::JSError::recordStackTrace(errHandle, hermesRt->context_);
        hermes::vm::JSError::setupStack(errHandle, hermesRt->context_);
        hermesRt->context_->setThrownValue(errHandle.getHermesValue());
    }else{
        hermesRt->context_->setThrownValue(hermesVal);
    }
    return NAPIOK;
}

// napi_throw_error
// napi_throw_type_error
// napi_throw_range_error
// napi_is_error
// napi_create_error
// napi_create_type_error
// napi_create_range_error
// Methods to support catching exceptions


NAPIStatus napi_get_and_clear_last_exception(NAPIEnv env, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    auto hermesRt = hermesImpl::HermesRuntimeFromJsEnv(env);
    auto exceptionValue = hermesRt->context_->getThrownValue();
    if (exceptionValue.isEmpty())
    {
        return napi_get_undefined(env, result);
    }
    else
    {
        *result = hermesImpl::JsValueFromHermesValue(exceptionValue);
        hermesRt->context_->clearThrownValue();
    }
    return NAPIOK;
}

#pragma mark <create | constructor>
NAPIStatus napi_get_undefined(NAPIEnv env, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);
    *result = hermesImpl::JsValueFromHermesHandle(hermes::vm::Runtime::getUndefinedValue());
    return NAPIOK;
}

NAPIStatus napi_get_null(NAPIEnv env, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);
    *result = hermesImpl::JsValueFromHermesHandle(hermes::vm::Runtime::getNullValue());
    return NAPIOK;
}

NAPIStatus napi_get_global(NAPIEnv env, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    auto hermesRt = hermesImpl::HermesRuntimeFromJsEnv(env);
    RETURN_STATUS_IF_FALSE(hermesRt->context_->getGlobal(), NAPIMemoryError);
    *result = hermesImpl::JsValueFromHermesHandle(hermesRt->context_->getGlobal());
    return NAPIOK;
}

NAPIStatus napi_get_boolean(NAPIEnv env, bool value, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);
    *result = hermesImpl::JsValueFromHermesHandle(hermes::vm::Runtime::getBoolValue(value));
    return NAPIOK;
}

/// If allocation fails, the GC declares an OOM.
// napi_create_object
// see common.c

NAPIStatus napi_create_external(NAPIEnv env, void *data, NAPIFinalize finalizeCB, void *finalizeHint,
                                NAPIValue *result){
    CHECK_ARG(env);
    CHECK_ARG(result);

    auto hermesRt = hermesImpl::HermesRuntimeFromJsEnv(env);
    auto hostObjectRes = hermes::vm::HostObject::createWithoutPrototype(hermesRt->context_, hermesImpl::HermesExternalObject::create(env,data,finalizeCB,finalizeHint));
    CHECK_HERMES(env,hostObjectRes.getStatus());
    *result = hermesImpl::JsValueFromHermesValue(hostObjectRes.getValue());
    return NAPIOK;
}
// napi_create_array & napi_create_array_with_length
// see common.c

NAPIStatus napi_create_double(NAPIEnv env, double value, NAPIValue *result)
{

    CHECK_ARG(env);
    CHECK_ARG(result);
    *result = hermesImpl::JsValueFromHermesValue(hermes::vm::HermesValue::encodeDoubleValue(value));
    return NAPIOK;
}

NAPIStatus napi_create_int32(NAPIEnv env, int32_t value, NAPIValue *result)
{

    CHECK_ARG(env);
    CHECK_ARG(result);
    *result = hermesImpl::JsValueFromHermesValue(hermes::vm::HermesValue::encodeNumberValue(value));
    return NAPIOK;
}

NAPIStatus napi_create_uint32(NAPIEnv env, uint32_t value, NAPIValue *result)
{

    CHECK_ARG(env);
    CHECK_ARG(result);
    *result = hermesImpl::JsValueFromHermesValue(hermes::vm::HermesValue::encodeNativeUInt32(value));
    return NAPIOK;
}

NAPIStatus napi_create_int64(NAPIEnv env, int64_t value, NAPIValue *result)
{

    CHECK_ARG(env);
    CHECK_ARG(result);
    *result = hermesImpl::JsValueFromHermesValue(hermes::vm::HermesValue::encodeNumberValue(value));
    return NAPIOK;
}
// 如果 length == NAPI_AUTO_LENGTH，则 str 包含 \0
NAPIStatus napi_create_string_utf8(NAPIEnv env, const char *str, size_t length, NAPIValue *result)
{

    CHECK_ARG(env);
    CHECK_ARG(result);

    // c++ 默认为 ""
    std::u16string str16;
         // strlen 以char 计算字节长度，uint8 = char
    if (length == NAPI_AUTO_LENGTH){
        if (str){
            length = strlen(str);
        }else{
            length = 0;
        }
    }
    //
    hermesImpl::convertUtf8ToUtf16(reinterpret_cast<const uint8_t *>(str), length, str16);
    auto hermesRt = hermesImpl::HermesRuntimeFromJsEnv(env);

    auto codeRes = hermes::vm::StringPrimitive::createEfficient(hermesRt->context_, std::move(str16));
    CHECK_HERMES(env, codeRes.getStatus());

    *result = hermesImpl::JsValueFromHermesValue(codeRes.getValue());

    return NAPIOK;
}
// todo:delete
NAPIStatus napi_create_string_utf16(NAPIEnv env, const char16_t *str, size_t length, NAPIValue *result)
{

    CHECK_ARG(env);
    CHECK_ARG(result);

    auto hermesRt = hermesImpl::HermesRuntimeFromJsEnv(env);
    auto codeRes = hermes::vm::StringPrimitive::createEfficient(hermesRt->context_, std::move(str));
    CHECK_HERMES(env, codeRes.getStatus());

    *result = hermesImpl::JsValueFromHermesValue(codeRes.getValue());
    return NAPIOK;
}
#pragma mark <Working with JavaScript Properties>


// todo
NAPIStatus napi_get_prototype(NAPIEnv env, NAPIValue object, NAPIValue *result)
{

    CHECK_ARG(env);
    CHECK_ARG(result);
    auto hermesRt = hermesImpl::HermesRuntimeFromJsEnv(env);

    auto jsObject = hermes::vm::dyn_vmcast_or_null<hermes::vm::JSObject>(hermesImpl::HermesValueFromJsValue(object));
    RETURN_STATUS_IF_FALSE(jsObject, NAPIObjectExpected);

    auto callRes = hermes::vm::JSObject::getPrototypeOf(hermes::vm::createPseudoHandle(jsObject), hermesRt->context_);
    CHECK_HERMES(env, callRes.getStatus());

    *result = hermesImpl::JsValueFromHermesValue(callRes->getHermesValue());
    return NAPIOK;
}


NAPIStatus napi_set_property(NAPIEnv env, NAPIValue object, NAPIValue key, NAPIValue value)
{
    CHECK_ARG(env);
    CHECK_ARG(object);

    auto hermesRt = hermesImpl::HermesRuntimeFromJsEnv(env);

    auto jsObject = hermes::vm::dyn_vmcast_or_null<hermes::vm::JSObject>(hermesImpl::HermesValueFromJsValue(object));
    RETURN_STATUS_IF_FALSE(jsObject, NAPIObjectExpected);

    auto keyString =
        hermes::vm::dyn_vmcast_or_null<hermes::vm::StringPrimitive>(hermesImpl::HermesValueFromJsValue(key));
    if (!keyString){
        //toString()
        NAPIValue toStringKey;
        CHECK_NAPI(napi_coerce_to_string(env, key, &toStringKey));
        keyString = hermes::vm::dyn_vmcast_or_null<hermes::vm::StringPrimitive>(hermesImpl::HermesValueFromJsValue(toStringKey));
    }
    RETURN_STATUS_IF_FALSE(keyString, NAPIStringExpected);

    // symbol
    auto symbolRes = hermes::vm::stringToSymbolID(hermesRt->context_, hermes::vm::createPseudoHandle(keyString));
    CHECK_HERMES(env, symbolRes.getStatus());


    hermes::vm::HandleRootOwner *superRuntimePtr = reinterpret_cast<hermes::vm::HandleRootOwner *>(hermesRt->context_);

    // call setter
    auto setRes = hermes::vm::JSObject::putNamed_RJS(
        superRuntimePtr->makeHandle(jsObject), hermesRt->context_, symbolRes.getValue().get(),
        superRuntimePtr->makeHandle(hermesImpl::HermesValueFromJsValue(value)));
    CHECK_HERMES(env, setRes.getStatus());

    return NAPIOK;
}

NAPIStatus napi_has_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result)
{

    CHECK_ARG(env);
    CHECK_ARG(result);
    CHECK_ARG(object);

    auto hermesRt = hermesImpl::HermesRuntimeFromJsEnv(env);

    auto jsObject = hermes::vm::dyn_vmcast_or_null<hermes::vm::JSObject>(hermesImpl::HermesValueFromJsValue(object));
    RETURN_STATUS_IF_FALSE(jsObject, NAPIObjectExpected);

    auto keyString =
        hermes::vm::dyn_vmcast_or_null<hermes::vm::StringPrimitive>(hermesImpl::HermesValueFromJsValue(key));
    if (!keyString){
        //toString()
        NAPIValue toStringKey;
        CHECK_NAPI(napi_coerce_to_string(env, key, &toStringKey));
        keyString = hermes::vm::dyn_vmcast_or_null<hermes::vm::StringPrimitive>(hermesImpl::HermesValueFromJsValue(toStringKey));
    }
    RETURN_STATUS_IF_FALSE(keyString, NAPIStringExpected);

    // symbol
    auto symbolRes = hermes::vm::stringToSymbolID(hermesRt->context_, hermes::vm::createPseudoHandle(keyString));
    CHECK_HERMES(env, symbolRes.getStatus());


    hermes::vm::HandleRootOwner *rtPtr = reinterpret_cast<hermes::vm::HandleRootOwner *>(hermesRt->context_);
    auto callRes = hermes::vm::JSObject::hasNamed(rtPtr->makeHandle(jsObject), hermesRt->context_, symbolRes->get());
    CHECK_HERMES(env, callRes.getStatus());

    *result = callRes.getValue();
    return NAPIOK;
}

NAPIStatus napi_get_property(NAPIEnv env, NAPIValue object, NAPIValue key, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);
    CHECK_ARG(object);

    auto hermesRt = hermesImpl::HermesRuntimeFromJsEnv(env);

    auto jsObject = hermes::vm::dyn_vmcast_or_null<hermes::vm::JSObject>(hermesImpl::HermesValueFromJsValue(object));
    RETURN_STATUS_IF_FALSE(jsObject, NAPIObjectExpected);

    auto keyString =
        hermes::vm::dyn_vmcast_or_null<hermes::vm::StringPrimitive>(hermesImpl::HermesValueFromJsValue(key));
    if (!keyString){
        //toString()
        NAPIValue toStringKey;
        CHECK_NAPI(napi_coerce_to_string(env, key, &toStringKey));
        keyString = hermes::vm::dyn_vmcast_or_null<hermes::vm::StringPrimitive>(hermesImpl::HermesValueFromJsValue(toStringKey));
    }
    RETURN_STATUS_IF_FALSE(keyString, NAPIStringExpected);

    // symbol
    auto symbolRes = hermes::vm::stringToSymbolID(hermesRt->context_, hermes::vm::createPseudoHandle(keyString));
    CHECK_HERMES(env, symbolRes.getStatus());


    hermes::vm::HandleRootOwner *superRuntimePtr = reinterpret_cast<hermes::vm::HandleRootOwner *>(hermesRt->context_);

    // call getter
    auto getterRes = hermes::vm::JSObject::getNamed_RJS(superRuntimePtr->makeHandle(jsObject), hermesRt->context_,
                                                        symbolRes.getValue().get());
    CHECK_HERMES(env, getterRes.getStatus());


    // PseudoHandle 本质上为指针
    *result = hermesImpl::JsValueFromHermesValue(getterRes.getValue().get());
    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);

    return NAPIOK;
}

NAPIStatus napi_delete_property(NAPIEnv env, NAPIValue object, NAPIValue key, bool *result)
{
    CHECK_ARG(env);
    CHECK_ARG(object);

    auto hermesRt = hermesImpl::HermesRuntimeFromJsEnv(env);

    auto jsObject = hermes::vm::dyn_vmcast_or_null<hermes::vm::JSObject>(hermesImpl::HermesValueFromJsValue(object));
    RETURN_STATUS_IF_FALSE(jsObject, NAPIObjectExpected);

    auto named = hermes::vm::dyn_vmcast_or_null<hermes::vm::StringPrimitive>(hermesImpl::HermesValueFromJsValue(key));
    if (!named){
        //toString()
        NAPIValue toStringKey;
        CHECK_NAPI(napi_coerce_to_string(env, key, &toStringKey));
        named = hermes::vm::dyn_vmcast_or_null<hermes::vm::StringPrimitive>(hermesImpl::HermesValueFromJsValue(toStringKey));
    }
    RETURN_STATUS_IF_FALSE(named, NAPIStringExpected);

    // symbol
    auto symbolRes = hermes::vm::stringToSymbolID(hermesRt->context_, hermes::vm::createPseudoHandle(named));
    CHECK_HERMES(env, symbolRes.getStatus());


    hermes::vm::HandleRootOwner *rt = reinterpret_cast<hermes::vm::HandleRootOwner *>(hermesRt->context_);
    auto callRes = hermes::vm::JSObject::deleteNamed(rt->makeHandle(jsObject), hermesRt->context_, symbolRes.getValue().get());
    CHECK_HERMES(env, callRes.getStatus());

    if (result){
        *result = callRes.getValue();
    }
    return NAPIOK;
}

// napi_set_named_property
// This method is equivalent to calling napi_set_property with a napi_value created from the string passed in as
// utf8Name.

// This method is equivalent to calling napi_has_property with a napi_value created from the string passed in as
// utf8Name.
NAPIStatus napi_has_named_property(NAPIEnv env, NAPIValue object, const char *utf8name, bool *result)
{

    CHECK_ARG(env);
    CHECK_ARG(result);

    auto hermesRt = hermesImpl::HermesRuntimeFromJsEnv(env);

    std::u16string nameUTF16;
    {
        hermesImpl::convertUtf8ToUtf16(reinterpret_cast<const uint8_t *>(utf8name), strlen(utf8name), nameUTF16);
    }
    RETURN_STATUS_IF_FALSE(!nameUTF16.empty(), NAPIMemoryError);

    // hermes sting
    auto keyCallRes = hermes::vm::StringPrimitive::createEfficient(hermesRt->context_, std::move(nameUTF16));
    CHECK_HERMES(env, keyCallRes.getStatus());


    CHECK_NAPI(napi_has_property(env, object, hermesImpl::JsValueFromHermesValue(keyCallRes.getValue()), result));
    return NAPIOK;
}

// This method is equivalent to calling napi_get_property with a napi_value created from the string passed in as
// utf8Name.
// napi_get_named_property
// see common.c
#pragma mark <Working with JavaScript functions>

NAPIStatus napi_get_cb_info(NAPIEnv env, NAPICallbackInfo callbackInfo, size_t *argc, NAPIValue *argv,
                            NAPIValue *thisArg, void **data){

    CHECK_ARG(env);
    CHECK_ARG(callbackInfo);

    auto hermesCbInfo = hermesImpl::HermesCallBackInfoFromJSCallBackInfo(callbackInfo);

    if (argv){
        CHECK_ARG(env);
        CHECK_ARG(argv);
        size_t inputSize = *argc;
        *argc = hermesCbInfo->GetArgc();
        size_t min = *argc < 0 || inputSize > *argc ? *argc : inputSize;
        hermesCbInfo->GetArgs(argv,min,inputSize);
    }
    if (argc){
        *argc = hermesCbInfo->GetArgc();
    }
    if (data){
        *data = hermesCbInfo->GetData();
    }
    if (thisArg){
        *thisArg = hermesImpl::JsValueFromHermesValue(hermesCbInfo->GetThisArg());
    }
    return NAPIOK;
}

NAPIStatus napi_get_new_target(NAPIEnv env, NAPICallbackInfo callbackInfo, NAPIValue *result){

    CHECK_ARG(env);
    CHECK_ARG(callbackInfo);

    auto hermesCbInfo = hermesImpl::HermesCallBackInfoFromJSCallBackInfo(callbackInfo);
    auto target = hermesCbInfo->GetNewTarget();
    if (target.isUndefined()){
        *result = NULL;
    }else{
        *result = hermesImpl::JsValueFromHermesValue(target);
    };
    return NAPIOK;
}

// result 可空
// thisValue 可空
NAPIStatus napi_call_function(NAPIEnv env, NAPIValue thisValue, NAPIValue func, size_t argc, const NAPIValue *argv,
                              NAPIValue *result){
    CHECK_ARG(env);
    CHECK_ARG(func);

    auto hermesRt = hermesImpl::HermesRuntimeFromJsEnv(env);

    // this
    auto hermesThis = hermesImpl::HermesValueFromJsValue(thisValue);
    auto thisObject = hermes::vm::dyn_vmcast_or_null<hermes::vm::JSObject>(hermesThis);


    //callable
    auto hermesFunc = hermesImpl::HermesValueFromJsValue(func);
    auto function = hermes::vm::dyn_vmcast_or_null<hermes::vm::Callable>(hermesFunc);
    RETURN_STATUS_IF_FALSE(function, NAPIObjectExpected);

    hermes::vm::HandleRootOwner *rtPtr = reinterpret_cast<hermes::vm::HandleRootOwner *>(hermesRt->context_);

    //arg
    auto argumentsRes = hermes::vm::Arguments::create(hermesRt->context_, static_cast<unsigned int>(argc), hermes::vm::HandleRootOwner::makeNullHandle<hermes::vm::Callable>(), true);
    hermes::vm::Handle<hermes::vm::Arguments> argumentsHandle = argumentsRes.getValue();
    for (int i = 0; i < argc; ++i)
    {
        auto arg = hermesImpl::HermesValueFromJsValue(argv[i]);
        hermes::vm::ArrayImpl::setElementAt(argumentsHandle, hermesRt->context_, static_cast<unsigned int>(i),
                                            rtPtr->makeHandle(arg));
    }
    auto thisHandle = thisObject ? rtPtr->makeHandle(thisObject) : hermes::vm::Runtime::getUndefinedValue();
    auto callRes = hermes::vm::Callable::executeCall(rtPtr->makeHandle(function), hermesRt->context_, hermes::vm::Runtime::getUndefinedValue(), thisHandle, argumentsHandle);
    CHECK_HERMES(env, callRes.getStatus());

    if (result){
        auto callRet = callRes->getHermesValue();
        *result = hermesImpl::JsValueFromHermesValue(callRet);
    }
    return NAPIOK;
}

// The newly created function is not automatically visible from script after this call.
// Instead, a property must be explicitly set on any object that is visible to JavaScript, in order for the function to be accessible from script.

NAPIStatus napi_create_function(NAPIEnv env, const char *utf8name, size_t length, NAPICallback cb, void *data,
                                NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);
    CHECK_ARG(cb);

    auto hermesRt = hermesImpl::HermesRuntimeFromJsEnv(env);

    NAPIValue nameValue;
    CHECK_NAPI(napi_create_string_utf8(env, utf8name, NAPI_AUTO_LENGTH, &nameValue));

    auto nameStr = hermes::vm::dyn_vmcast_or_null<hermes::vm::StringPrimitive>(hermesImpl::HermesValueFromJsValue(nameValue));
    RETURN_STATUS_IF_FALSE(nameStr,  NAPIMemoryError);

    auto nameSymRes = hermes::vm::stringToSymbolID(hermesRt->context_,  hermes::vm::createPseudoHandle(nameStr));
    CHECK_HERMES(env, nameSymRes.getStatus());

    auto nameSym = nameSymRes.getValue().get();

    //save functoin info
    auto funcInfo = new hermesImpl::FunctionInfo(env,cb,data);

    //NAPICallback -> hermes::nativeFunction
    hermes::vm::NativeFunctionPtr funcPtr = [](void *context, hermes::vm::Runtime *runtime, hermes::vm::NativeArgs args) -> hermes::vm::CallResult<hermes::vm::HermesValue>{
        hermes::vm::GCScope gcScope();
        hermesImpl::FunctionInfo *funcInfo = static_cast<hermesImpl::FunctionInfo *>(context);
        hermesImpl::CallBackInfoWrapper wrapper = hermesImpl::CallBackInfoWrapper(args, funcInfo->GetData());
        auto jsCb = hermesImpl::JSCallBackInfoFormHermesCallBackInfo(wrapper);
        auto callRes = funcInfo->GetCb()(funcInfo->GetEnv(), jsCb);
        auto exception = runtime->getThrownValue();
        if (!exception.isEmpty()){

            return hermes::vm::ExecutionStatus::EXCEPTION;
        }
        return hermes::vm::CallResult<hermes::vm::HermesValue>(hermesImpl::HermesValueFromJsValue(callRes));
    };

    hermes::vm::FinalizeNativeFunctionPtr finalizeFuncPtr = [](void *context) -> void {
        // free functionInfo
        hermesImpl::FunctionInfo *funcInfo = static_cast<hermesImpl::FunctionInfo *>(context);
        delete funcInfo;
    };


    //createNativefunction
    auto funcValRes = hermes::vm::FinalizableNativeFunction::createWithoutPrototype(hermesRt->context_, funcInfo, funcPtr, finalizeFuncPtr, nameSym, length);
    CHECK_HERMES(env, funcValRes.getStatus());


    *result = hermesImpl::JsValueFromHermesValue(funcValRes.getValue());
    return NAPIOK;
}

NAPIStatus napi_new_instance(NAPIEnv env, NAPIValue constructor, size_t argc, const NAPIValue *argv, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(constructor);
    if (argc > 0)
    {
        CHECK_ARG(argv);
    }
    CHECK_ARG(result);

    auto hermesRt = hermesImpl::HermesRuntimeFromJsEnv(env);

    auto ctor_hermesValue = hermesImpl::HermesValueFromJsValue(constructor);
    auto ctor_Object = hermes::vm::dyn_vmcast_or_null<hermes::vm::JSObject>(ctor_hermesValue);
    RETURN_STATUS_IF_FALSE(hermes::vm::isConstructor(hermesRt->context_, ctor_hermesValue), NAPIFunctionExpected);


    // todo:fix rt
    hermes::vm::HandleRootOwner *rtPtr = reinterpret_cast<hermes::vm::HandleRootOwner *>(hermesRt->context_);

    if (argc > std::numeric_limits<uint32_t>::max() ||
        !hermesRt->context_->checkAvailableStack((uint32_t)argc)) {
        RETURN_STATUS_IF_FALSE(false, NAPIMemoryError);
    }

    hermes::vm::Handle<hermes::vm::Callable> callableHandle =
        hermes::vm::Handle<hermes::vm::Callable>::dyn_vmcast(rtPtr->makeHandle(ctor_Object));

    // We follow es5 13.2.2 [[Construct]] here. Below F == func.
    // 13.2.2.5:
    //    Let proto be the value of calling the [[Get]] internal property of
    //    F with argument "prototype"
    // 13.2.2.6:
    //    If Type(proto) is Object, set the [[Prototype]] internal property
    //    of obj to proto
    // 13.2.2.7:
    //    If Type(proto) is not Object, set the [[Prototype]] internal property
    //    of obj to the standard built-in Object prototype object as described
    //    in 15.2.4
    //
    // Note that 13.2.2.1-4 are also handled by the call to newObject.
    auto thisRes = hermes::vm::Callable::createThisForConstruct(callableHandle, hermesRt->context_);
    CHECK_HERMES(env, thisRes.getStatus());

    // We need to capture this in case the ctor doesn't return an object,
    // we need to return this object.
    auto objHandle = rtPtr->makeHandle<hermes::vm::JSObject>(std::move(*thisRes));

    // 13.2.2.8:
    //    Let result be the result of calling the [[Call]] internal property of
    //    F, providing obj as the this value and providing the argument list
    //    passed into [[Construct]] as args.
    //
    // For us result == res.

    hermes::vm::ScopedNativeCallFrame newFrame{
        hermesRt->context_,
        static_cast<uint32_t>(argc),
        callableHandle.getHermesValue(),
        callableHandle.getHermesValue(),
        objHandle.getHermesValue()};
    if (newFrame.overflowed()) {
        auto res = hermesRt->context_->raiseStackOverflow(::hermes::vm::StackRuntime::StackOverflowKind::NativeStack);
        CHECK_HERMES(env, res);

    }
    for (uint32_t i = 0; i != argc; ++i) {
        newFrame->getArgRef(i) = hermesImpl::HermesValueFromJsValue(argv[i]);
    }
    // The last parameter indicates that this call should construct an object.
    auto callRes = hermes::vm::Callable::call(callableHandle, hermesRt->context_);
    CHECK_HERMES(env, callRes.getStatus());

    // 13.2.2.9:
    //    If Type(result) is Object then return result
    // 13.2.2.10:
    //    Return obj
    auto resultValue = callRes->get();
    hermes::vm::HermesValue resultHValue = resultValue.isObject() ? resultValue : objHandle.getHermesValue();

    *result = hermesImpl::JsValueFromHermesValue(resultHValue);
    return NAPIOK;
}

#pragma mark <Working with JavaScript values and abstract operations>
// value 可能是 0 的number
NAPIStatus napi_is_array(NAPIEnv env, NAPIValue value, bool *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    auto arrObj = hermes::vm::dyn_vmcast_or_null<hermes::vm::JSObject>(hermesImpl::HermesValueFromJsValue(value));
    auto hermesRt = hermesImpl::HermesRuntimeFromJsEnv(env);
    auto res = hermes::vm::isArray(hermesRt->context_, arrObj);
    CHECK_HERMES(env, res.getStatus());

    *result = res.getValue();
    return NAPIOK;
}

// hermes 的 number为 原始值，因此 0(空指针)会被识别为 number
NAPIStatus napi_typeof(NAPIEnv env, NAPIValue value, NAPIValueType *result)
{

    CHECK_ARG(env);
    CHECK_ARG(result);

    // Hermes does not support BigInt
    hermes::vm::HermesValue hv = hermesImpl::HermesValueFromJsValue(value);
    if (hv.isUndefined())
    {
        *result = NAPIUndefined;
    }
    else if (hv.isNull())
    {
        *result = NAPINull;
    }
    else if (hv.isBool())
    {
        *result = NAPIBoolean;
    }
    else if (hv.isNumber())
    {
        *result = NAPINumber;
    }
    else if (hv.isString())
    {
        *result = NAPIString;
    }
    else if (hv.isObject())
    {
        auto function = hermes::vm::dyn_vmcast_or_null<hermes::vm::Callable>(hermesImpl::HermesValueFromJsValue(value));
        if (function)
        {
            *result = NAPIFunction;
        }
        else
        {
            auto hostObject =
                hermes::vm::dyn_vmcast_or_null<hermes::vm::HostObject>(hermesImpl::HermesValueFromJsValue(value));
            if (hostObject)
            {
                *result = NAPIExternal;
            }
            else
            {
                *result = NAPIObject;
            }
        }
    }
    else
    {
        assert(false);
        return NAPIInvalidArg;
    }
    return NAPIOK;
}

NAPIStatus napi_coerce_to_bool(NAPIEnv env, NAPIValue value, NAPIValue *result)
{

    CHECK_ARG(env);
    CHECK_ARG(result);

    *result = hermesImpl::JsValueFromHermesValue(
        hermes::vm::HermesValue::encodeBoolValue(hermes::vm::toBoolean(hermesImpl::HermesValueFromJsValue(value))));
    return NAPIOK;
}

// The difference between this and toNumber is that it can return a
// BigInt.  But Hermes doesn't support BigInt yet, so for now, this
// is a placeholder.
NAPIStatus napi_coerce_to_number(NAPIEnv env, NAPIValue value, NAPIValue *result)
{

    CHECK_ARG(env);
    CHECK_ARG(result);

    auto hermesRt = hermesImpl::HermesRuntimeFromJsEnv(env);

    // todo:fix rt
    auto *rtPtr = reinterpret_cast<hermes::vm::HandleRootOwner *>(hermesRt->context_);
    auto callRes = hermes::vm::toNumeric_RJS(hermesRt->context_, rtPtr->makeHandle(hermesImpl::HermesValueFromJsValue(value)));
    CHECK_HERMES(env, callRes.getStatus());

    *result = hermesImpl::JsValueFromHermesValue(callRes.getValue());
    return NAPIOK;
}
// napi_coerce_to_object
NAPIStatus napi_coerce_to_string(NAPIEnv env, NAPIValue value, NAPIValue *result)
{

    CHECK_ARG(env);
    CHECK_ARG(result);

    auto hermesRt = hermesImpl::HermesRuntimeFromJsEnv(env);

    // todo:fix rt
    auto *rtPtr = reinterpret_cast<hermes::vm::HandleRootOwner *>(hermesRt->context_);
    auto callRes = hermes::vm::toString_RJS(hermesRt->context_, rtPtr->makeHandle(hermesImpl::HermesValueFromJsValue(value)));
    CHECK_HERMES(env, callRes.getStatus());

    *result = hermesImpl::JsValueFromHermesValue(callRes->getHermesValue());
    return NAPIOK;
}

NAPIStatus napi_instanceof(NAPIEnv env, NAPIValue object, NAPIValue constructor, bool *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    auto hermesRt = hermesImpl::HermesRuntimeFromJsEnv(env);

    auto obj_hv = hermesImpl::HermesValueFromJsValue(object);
    auto ctor_hv = hermesImpl::HermesValueFromJsValue(constructor);

    auto jsObject = hermes::vm::dyn_vmcast_or_null<hermes::vm::JSObject>(obj_hv);
    RETURN_STATUS_IF_FALSE(jsObject, NAPIObjectExpected);

    RETURN_STATUS_IF_FALSE(hermes::vm::isConstructor(hermesRt->context_, obj_hv), NAPIFunctionExpected);

    // todo:fix rt
    hermes::vm::HandleRootOwner *rt = reinterpret_cast<hermes::vm::HandleRootOwner *>(hermesRt->context_);
    auto callRes = hermes::vm::instanceOfOperator_RJS(hermesRt->context_, rt->makeHandle(obj_hv), rt->makeHandle(ctor_hv));
    CHECK_HERMES(env, callRes.getStatus());
    *result = callRes.getValue();
    return NAPIOK;
}
// napi_strict_equals


#pragma mark <Methods to work with Objects>
#pragma mark <Functions to convert from Node-API to C types>
// NAPIBooleanExpected
NAPIStatus napi_get_value_bool(NAPIEnv env, NAPIValue value, bool *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    auto hermesVal = hermesImpl::HermesValueFromJsValue(value);

    RETURN_STATUS_IF_FALSE(hermesVal.isBool(), NAPIBooleanExpected);
    *result = hermesVal.getBool();
    return NAPIOK;
}
// NAPINumberExpected
NAPIStatus napi_get_value_double(NAPIEnv env, NAPIValue value, double *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    auto hermesVal = hermesImpl::HermesValueFromJsValue(value);
    if (hermesVal.isNumber()){
        *result = hermesVal.getNumber();
    }else if(hermesVal.isDouble()){
        *result = hermesVal.getDouble();
    }
    else
    {
        return NAPINumberExpected;
    }
    return NAPIOK;
}

// NAPINumberExpected
NAPIStatus napi_get_value_int64(NAPIEnv env, NAPIValue value, int64_t *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    auto hermesVal = hermesImpl::HermesValueFromJsValue(value);
    if (hermesVal.isNumber()){
        *result =(int64_t)hermesVal.getNumber();
    }else if(hermesVal.isDouble()){
        double doubleValue = hermesVal.getDouble();
        if (isfinite(doubleValue))
        {
            if (isnan(doubleValue))
            {
                *result = 0;
            }
            else if (doubleValue >= (double)INT64_MAX)
            {
                *result = INT64_MAX;
            }
            else if (doubleValue <= (double)INT64_MIN)
            {
                *result = INT64_MIN;
            }
            else
            {
                *result = (int64_t)doubleValue;
            }
        }
        else
        {
            *result = 0;
        }
    }else{
        return NAPINumberExpected;
    }
    return NAPIOK;
}

NAPIStatus napi_get_value_uint32(NAPIEnv env, NAPIValue value, uint32_t *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    auto hermesVal = hermesImpl::HermesValueFromJsValue(value);
    if (hermesVal.isNumber()){
        *result =(uint32_t)hermesVal.getNumber();
    }else if(hermesVal.isDouble()){
        *result =(uint32_t)hermesVal.getDouble();
    }else{
        return NAPINumberExpected;
    }
    return NAPIOK;
}

NAPIStatus napi_get_value_int32(NAPIEnv env, NAPIValue value, int32_t *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    auto hermesVal = hermesImpl::HermesValueFromJsValue(value);
    if (hermesVal.isNumber()){
        *result =(uint32_t)hermesVal.getNumber();
    }else if(hermesVal.isDouble()){
        *result =(uint32_t)hermesVal.getDouble();
    }else{
        return NAPINumberExpected;
    }
    return NAPIOK;
}

NAPIStatus napi_get_value_external(NAPIEnv env, NAPIValue value, void **result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    auto hostObject = hermes::vm::dyn_vmcast_or_null<hermes::vm::HostObject>(hermesImpl::HermesValueFromJsValue(value));
    RETURN_STATUS_IF_FALSE(hostObject, NAPIObjectExpected);

    // 转换失败返回空指针
    hermesImpl::HermesExternalObject *external =
        dynamic_cast<hermesImpl::HermesExternalObject *>(hostObject->getProxy());

    RETURN_STATUS_IF_FALSE(external, NAPIMemoryError);
    *result = external ? external->getData() : NULL;
    return NAPIOK;
}
// NAPIStringExpected/NAPIPendingException/NAPIGenericFailure
// 当 buf 为 null，result 返回字符串字节长度
//
NAPIStatus napi_get_value_string_utf8(NAPIEnv env, NAPIValue value, char *buf, size_t bufSize, size_t *result)
{
    CHECK_ARG(env);

    auto hermesVal = hermesImpl::HermesValueFromJsValue(value);
    auto jsString = hermes::vm::dyn_vmcast_or_null<hermes::vm::StringPrimitive>(hermesVal);
    RETURN_STATUS_IF_FALSE(jsString, NAPIStringExpected);

    if (buf && !bufSize)
    {
        if (result)
        {
            *result = 0;
        }

        return NAPIOK;
    }
    if (bufSize == 1 && buf)
    {
        buf[0] = '\0';
        if (result)
        {
            *result = 0;
        }

        return NAPIOK;
    }

    // 总长度：字节
    size_t length;
    if (jsString->isASCII()){
        //ascii code unit = 1byte
        length = jsString->getStringLength();
    }else{
        //sizeof(utf16) * array.length
        auto utf16Str = jsString->getStringRef<char16_t>();
        length = utf16Str.size() * sizeof(char16_t);
    }
    // buf = NULL 返回不包括/0 的字节长度。
    if (!buf)
    {
        *result = length;
        return NAPIOK;
    }

    // 最小长度：字节
    size_t minLength = bufSize > length ? length : bufSize;

    // 有效长度：最后一字节为 \0， 因此实际为 minLength - 1;
    size_t effectiveLen = minLength - 1;
    if (jsString->isASCII()){
        auto ascString = jsString->getStringRef<char>();
        //避免重叠
        memmove(buf, ascString.data(), effectiveLen);
        buf[effectiveLen] = '\0';
        *result = effectiveLen;
        return NAPIOK;
    }

    // utf16
    auto u16String = jsString->getStringRef<char16_t>();
    // utf16 -> utf8, 如果 minLength 为偶数，可能出现 unpaird surrogate， 导致 u8string 出现 0xfffd(�)
    // 因此提前取偶数(可完整转换为utf8)，之后再替换末位 0xfffd(�) -> '\0'
    std::string u8string;//C++ 默认为初始 \0
    auto evenLength = effectiveLen >> 1 << 1;
    hermesImpl::convertUtf16ToUtf8(u16String, evenLength, u8string);
    // C++ 默认补 \0
    u8string.resize(minLength);
    // 复制长度为 minLength(不需要 -1 补零)。
    memmove(buf, &u8string, minLength);
    // 真正长度为 evenLength。
    *result = evenLength;
    return NAPIOK;
}

#pragma mark <Object lifetime management>
#pragma mark <Making handle lifespan shorter than that of the native method>
// hermes gcscope 为栈构造，这里包装为堆成员之后，需要保持 napi_open_handle_scope 和 napi_close_handle_scope 对应关系。
// 否则 在 hermes 执行 gc 时会造成 crash。
NAPIStatus napi_open_handle_scope(NAPIEnv env, NAPIHandleScope *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    auto hermesRt = hermesImpl::HermesRuntimeFromJsEnv(env);

    hermes::vm::HandleRootOwner *superPtr = reinterpret_cast<hermes::vm::HandleRootOwner *>(hermesRt->context_);
    *result = hermesImpl::JsHandleScopeFromHermesScope(new hermesImpl::HandleScopeWrapper(superPtr));
    hermesRt->open_handle_scopes_++;
    return NAPIOK;
}

NAPIStatus napi_close_handle_scope(NAPIEnv env, NAPIHandleScope result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    auto hermesRt = hermesImpl::HermesRuntimeFromJsEnv(env);
    if (hermesRt->open_handle_scopes_ == 0)
    {
        return NAPIHandleScopeMismatch;
    }
    hermesRt->open_handle_scopes_--;
    hermesImpl::HandleScopeWrapper *scopeWrapper = hermesImpl::HermesScopeFormeJsHandleScope(result);
    delete scopeWrapper;
    return NAPIOK;
}


// NAPIMemoryError
NAPIStatus napi_open_escapable_handle_scope(NAPIEnv env, NAPIEscapableHandleScope *result)
{
    CHECK_ARG(env);
    CHECK_ARG(result);

    auto scope = new hermesImpl::EscapableHandleScope();
    *result = hermesImpl::JsEscapableHandleScopeFromHermesEscapableHandleScope(scope);

    RETURN_STATUS_IF_FALSE(*result, NAPIMemoryError);
    return NAPIOK;
}

NAPIStatus napi_close_escapable_handle_scope(NAPIEnv env, NAPIEscapableHandleScope scope)
{
    CHECK_ARG(env);
    CHECK_ARG(scope);

    auto hermesScope = hermesImpl::HermesEscapableHandleScopeFromJsEscapableHandleScope(scope);
    auto rt = hermesImpl::HermesRuntimeFromJsEnv(env);
    rt->ClearEscapableHandleScope(hermesScope);
    delete hermesScope;
    return NAPIOK;
}

// NAPIMemoryError/NAPIEscapeCalledTwice/NAPIHandleScopeMismatch
NAPIStatus napi_escape_handle(NAPIEnv env, NAPIEscapableHandleScope scope, NAPIValue escapee, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(scope);
    CHECK_ARG(escapee);
    CHECK_ARG(result);

    auto rt = hermesImpl::HermesRuntimeFromJsEnv(env);
    auto hermesScope = hermesImpl::HermesEscapableHandleScopeFromJsEscapableHandleScope(scope);
    RETURN_STATUS_IF_FALSE(!hermesScope->GetEscapeCalled(), NAPIEscapeCalledTwice);

    auto hermesValue = hermesImpl::HermesValueFromJsValue(escapee);
    hermesScope->SetHV(hermesValue);
    hermesScope->SetEscapeCalled(true);

    rt->AddEscapableHandleScope(hermesScope);
    *result = escapee;

    return NAPIOK;
}

#pragma mark <References to objects with a lifespan longer than that of the native method>
/*
 * 值类型（string symbol除外）只会返回 Reference 对象(因为值类型不受GC影响，因此只维护 引用计数)，如果不显式 delete，只会造成 Reference 对象的内存泄漏
 * 对象类型(包括 string symbol)支持引用
 * */
NAPIStatus napi_create_reference(NAPIEnv env, NAPIValue value, uint32_t initialRefCount, NAPIRef *result)
{
    CHECK_ARG(env);
    CHECK_ARG(value);
    CHECK_ARG(result);


    auto hermsValue = hermesImpl::HermesValueFromJsValue(value);
    auto ref = new hermesImpl::Reference(env, hermsValue, initialRefCount);
    RETURN_STATUS_IF_FALSE(ref, NAPIMemoryError);

    *result = reinterpret_cast<NAPIRef>(ref);
    return NAPIOK;
}


// clearWeak
NAPIStatus napi_reference_ref(NAPIEnv env, NAPIRef ref, uint32_t *result)
{
    CHECK_ARG(env);
    CHECK_ARG(ref);
    CHECK_ARG(result);

    hermesImpl::Reference *_ref = reinterpret_cast<hermesImpl::Reference *>(ref);
    //has been freed
    RETURN_STATUS_IF_FALSE(_ref->IsValid(), NAPIGenericFailure);
    *result = _ref->Ref();
    return NAPIOK;
}


// NAPIGenericFailure + setWeak
NAPIStatus napi_reference_unref(NAPIEnv env, NAPIRef ref, uint32_t *result)
{
    CHECK_ARG(env);
    CHECK_ARG(ref);
    CHECK_ARG(result);

    hermesImpl::Reference *_ref = reinterpret_cast<hermesImpl::Reference *>(ref);
    RETURN_STATUS_IF_FALSE(_ref->count_, NAPIGenericFailure);
    *result = _ref->Unref();
    return NAPIOK;
}

NAPIStatus napi_get_reference_value(NAPIEnv env, NAPIRef ref, NAPIValue *result)
{
    CHECK_ARG(env);
    CHECK_ARG(ref);
    CHECK_ARG(result);

    hermesImpl::Reference *_ref = reinterpret_cast<hermesImpl::Reference *>(ref);
    if (!_ref->IsValid()){

        *result = NULL;
    } else {

        auto hermesValue = _ref->GetValue();
        *result = hermesImpl::JsValueFromHermesValue(hermesValue);
    }
    return NAPIOK;
}

NAPIStatus napi_delete_reference(NAPIEnv env, NAPIRef ref)
{
    CHECK_ARG(env);
    CHECK_ARG(ref);

    hermesImpl::Reference *_ref = reinterpret_cast<hermesImpl::Reference *>(ref);
    delete _ref;
    return NAPIOK;
}



#pragma mark <Custom Function>


// utf8Script/utf8SourceUrl/result 可空
NAPI_NATIVE NAPIStatus NAPIRunScript(NAPIEnv env, const char *script, const char *sourceUrl, NAPIValue *result){

    CHECK_ARG(env);
    // script 传入 NULL 会崩
    if (!script)
    {
        script = "";
    }
    if (!sourceUrl)
    {
        sourceUrl = "";
    }
    hermes::hbc::CompileFlags compileFlags;
    compileFlags.debug = true;
    auto rt = hermesImpl::HermesRuntimeFromJsEnv(env);
    auto runRes = rt->context_->run(script, sourceUrl, compileFlags);
    CHECK_HERMES(env, runRes.getStatus());

    auto hermesVal = hermesImpl::JsValueFromHermesValue(runRes.getValue());
    if (result){
        *result = hermesVal;
    }
    return NAPIOK;
}

// NAPIMemoryError/NAPIPendingException + addValueToHandleScope
NAPIStatus NAPIDefineClass(NAPIEnv env, const char *utf8name, size_t length, NAPICallback constructor, void *data,
                           NAPIValue *result)
{
        return NAPIOK;
}

NAPI_NATIVE NAPIStatus NAPIGetValueStringUTF8(NAPIEnv env, NAPIValue value, const char **result){

    CHECK_ARG(env);
    CHECK_ARG(value);
    CHECK_ARG(result);

    auto hermesVal = hermesImpl::HermesValueFromJsValue(value);
    auto jsString = hermes::vm::dyn_vmcast_or_null<hermes::vm::StringPrimitive>(hermesVal);
    RETURN_STATUS_IF_FALSE(jsString, NAPIStringExpected);

    // 总长度：字节
    size_t length;
    if (jsString->isASCII()){
        //ascii code unit = 1byte
        length = jsString->getStringLength();
    }else{
        //sizeof(utf16) * array.length
        auto utf16Str = jsString->getStringRef<char16_t>();
        length = utf16Str.size() * sizeof(char16_t);
    }
    // \0
    char *cString = (char *)malloc(sizeof(char) * (length+1));

    // 最小长度：字节
    size_t minLength = length + 1;//

    // 有效长度：最后一字节为 \0， 因此实际为 minLength - 1;
    size_t effectiveLen = minLength - 1;
    if (jsString->isASCII()){
        auto ascString = jsString->getStringRef<char>();
        //避免重叠
        memmove(cString, ascString.data(), effectiveLen);
        cString[effectiveLen] = '\0';
        *result = cString;
        return NAPIOK;
    }

    // utf16
    auto u16String = jsString->getStringRef<char16_t>();
    // utf16 -> utf8, 如果 minLength 为偶数，可能出现 unpaird surrogate， 导致 u8string 出现 0xfffd(�)
    // 因此提前取偶数(可完整转换为utf8)，之后再替换末位 0xfffd(�) -> '\0'
    std::string u8string;//C++ 默认为初始 \0
    auto evenLength = effectiveLen >> 1 << 1;
    hermesImpl::convertUtf16ToUtf8(u16String, evenLength, u8string);
    // C++ 默认补 \0
    u8string.resize(minLength);
    // 复制长度为 minLength(不需要 -1 补零)。
    memmove(cString, &u8string, minLength);
    // 真正长度为 evenLength。
    *result = cString;
    return NAPIOK;

}

NAPI_NATIVE NAPIStatus NAPIFreeUTF8String(NAPIEnv env, const char *cString){

    free((void *)cString);
    return NAPIOK;
}

// facebook::hermes::runtime -> js::runtime -> hermesRuntimeImp -> hermes::vm::runtime
NAPIStatus NAPICreateEnv(NAPIEnv *env)
{
    CHECK_ARG(env);
    auto hermesRt = new hermesImpl::Runtime();
    *env = reinterpret_cast<NAPIEnv>(hermesRt);
    return NAPIOK;
}

NAPIStatus NAPIFreeEnv(NAPIEnv env)
{
    CHECK_ARG(env);
    auto hermesRt = hermesImpl::HermesRuntimeFromJsEnv(env);
    delete hermesRt;
    return NAPIOK;
}