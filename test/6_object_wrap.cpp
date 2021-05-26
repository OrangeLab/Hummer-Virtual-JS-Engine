#include <common.h>
#include <js_native_api_test.h>

namespace
{
class MyObject
{
  public:
    static void Init(NAPIEnv env, NAPIValue exports);

    static void Destructor(NAPIEnv env, void *nativeObject, void *finalize_hint);

  private:
    explicit MyObject(double value_ = 0);

    ~MyObject();

    static NAPIValue New(NAPIEnv env, NAPICallbackInfo info);

    static NAPIValue GetValue(NAPIEnv env, NAPICallbackInfo info);

    static NAPIValue SetValue(NAPIEnv env, NAPICallbackInfo info);

    static NAPIValue PlusOne(NAPIEnv env, NAPICallbackInfo info);

    static NAPIValue Multiply(NAPIEnv env, NAPICallbackInfo info);

    static NAPIRef constructor;
    double value_;
    NAPIEnv env_;
    NAPIRef wrapper_;
};
} // namespace

NAPIRef MyObject::constructor;

MyObject::MyObject(double value) : value_(value), env_(nullptr), wrapper_(nullptr)
{
}

MyObject::~MyObject()
{
    napi_delete_reference(env_, wrapper_);
}

void MyObject::Destructor(NAPIEnv /*env*/, void *nativeObject, void * /*finalize_hint*/)
{
    auto obj = static_cast<MyObject *>(nativeObject);
    delete obj;
}

void MyObject::Init(NAPIEnv env, NAPIValue exports)
{
    NAPIPropertyDescriptor properties[] = {
        {"value", nullptr, nullptr, GetValue, SetValue, nullptr, NAPIDefault, nullptr},
        {"valueReadonly", nullptr, nullptr, GetValue, nullptr, nullptr, NAPIDefault,
         nullptr},
        DECLARE_NAPI_PROPERTY("plusOne", PlusOne),
        DECLARE_NAPI_PROPERTY("multiply", Multiply),
    };

    NAPIValue cons;
    NAPI_CALL_RETURN_VOID(env,
                          napi_define_class(env, "MyObject", -1, New, nullptr,
                                            sizeof(properties) / sizeof(NAPIPropertyDescriptor), properties, &cons));

    NAPI_CALL_RETURN_VOID(env, napi_create_reference(env, cons, 1, &constructor));

    NAPI_CALL_RETURN_VOID(env, napi_set_named_property(env, exports, "MyObject", cons));
}

NAPIValue MyObject::New(NAPIEnv env, NAPICallbackInfo info)
{
    NAPIValue new_target;
    NAPI_CALL(env, napi_get_new_target(env, info, &new_target));
    NAPIValueType valueType;
    NAPI_CALL(env, napi_typeof(env, new_target, &valueType));
    bool is_constructor = (valueType != NAPIUndefined);

    size_t argc = 1;
    NAPIValue args[1];
    NAPIValue _this;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &_this, nullptr));

    if (is_constructor)
    {
        // Invoked as constructor: `new MyObject(...)`
        double value = 0;

        NAPIValueType valuetype;
        NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));

        if (valuetype != NAPIUndefined)
        {
            NAPI_CALL(env, napi_get_value_double(env, args[0], &value));
        }

        auto obj = new MyObject(value);

        obj->env_ = env;
        NAPI_CALL(env, napi_wrap(env, _this, obj, MyObject::Destructor,
                                 nullptr, // finalize_hint
                                 &obj->wrapper_));

        return _this;
    }

    // Invoked as plain function `MyObject(...)`, turn into construct call.
    argc = 1;
    NAPIValue argv[1] = {args[0]};

    NAPIValue cons;
    NAPI_CALL(env, napi_get_reference_value(env, constructor, &cons));

    NAPIValue instance;
    NAPI_CALL(env, napi_new_instance(env, cons, argc, argv, &instance));

    return instance;
}

NAPIValue MyObject::GetValue(NAPIEnv env, NAPICallbackInfo info)
{
    NAPIValue _this;
    NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &_this, nullptr));

    MyObject *obj;
    NAPI_CALL(env, napi_unwrap(env, _this, reinterpret_cast<void **>(&obj)));

    NAPIValue num;
    NAPI_CALL(env, napi_create_double(env, obj->value_, &num));

    return num;
}

NAPIValue MyObject::SetValue(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue args[1];
    NAPIValue _this;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &_this, nullptr));

    MyObject *obj;
    NAPI_CALL(env, napi_unwrap(env, _this, reinterpret_cast<void **>(&obj)));

    NAPI_CALL(env, napi_get_value_double(env, args[0], &obj->value_));

    return nullptr;
}

NAPIValue MyObject::PlusOne(NAPIEnv env, NAPICallbackInfo info)
{
    NAPIValue _this;
    NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &_this, nullptr));

    MyObject *obj;
    NAPI_CALL(env, napi_unwrap(env, _this, reinterpret_cast<void **>(&obj)));

    obj->value_ += 1;

    NAPIValue num;
    NAPI_CALL(env, napi_create_double(env, obj->value_, &num));

    return num;
}

NAPIValue MyObject::Multiply(NAPIEnv env, NAPICallbackInfo info)
{
    size_t argc = 1;
    NAPIValue args[1];
    NAPIValue _this;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &_this, nullptr));

    double multiple = 1;
    if (argc >= 1)
    {
        NAPI_CALL(env, napi_get_value_double(env, args[0], &multiple));
    }

    MyObject *obj;
    NAPI_CALL(env, napi_unwrap(env, _this, reinterpret_cast<void **>(&obj)));

    NAPIValue cons;
    NAPI_CALL(env, napi_get_reference_value(env, constructor, &cons));

    const int kArgCount = 1;
    NAPIValue argv[kArgCount];
    NAPI_CALL(env, napi_create_double(env, obj->value_ * multiple, argv));

    NAPIValue instance;
    NAPI_CALL(env, napi_new_instance(env, cons, kArgCount, argv, &instance));

    return instance;
}

TEST_F(Test, ObjectWrap)
{
    MyObject::Init(globalEnv, exports);

    NAPIValue result;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(
                  globalEnv,
                  "(()=>{\"use strict\";var "
                  "s=Object.getOwnPropertyDescriptor(globalThis.addon.MyObject.prototype,\"value\"),l=Object."
                  "getOwnPropertyDescriptor(globalThis.addon.MyObject.prototype,\"valueReadonly\"),t=Object."
                  "getOwnPropertyDescriptor(globalThis.addon.MyObject.prototype,\"plusOne\");s?(globalThis.assert."
                  "strictEqual(typeof s.get,\"function\"),globalThis.assert.strictEqual(typeof "
                  "s.set,\"function\"),globalThis.assert.strictEqual(s.value,void "
                  "0),globalThis.assert.strictEqual(s.enumerable,!1),globalThis.assert.strictEqual(s.configurable,!1)):"
                  "globalThis.assert(!1),l?(globalThis.assert.strictEqual(typeof "
                  "l.get,\"function\"),globalThis.assert.strictEqual(l.set,void "
                  "0),globalThis.assert.strictEqual(l.value,void "
                  "0),globalThis.assert.strictEqual(l.enumerable,!1),globalThis.assert.strictEqual(l.configurable,!1)):"
                  "globalThis.assert(!1),t?(globalThis.assert.strictEqual(t.get,void "
                  "0),globalThis.assert.strictEqual(t.set,void 0),globalThis.assert.strictEqual(typeof "
                  "t.value,\"function\"),globalThis.assert.strictEqual(t.enumerable,!1),globalThis.assert.strictEqual("
                  "t.configurable,!1)):globalThis.assert(!1);var a=new "
                  "globalThis.addon.MyObject(9);globalThis.assert.strictEqual(a.value,9),a.value=10,globalThis.assert."
                  "strictEqual(a.value,10),globalThis.assert.strictEqual(a.valueReadonly,10),globalThis.assert."
                  "strictEqual(a.plusOne(),11),globalThis.assert.strictEqual(a.plusOne(),12),globalThis.assert."
                  "strictEqual(a.plusOne(),13),globalThis.assert.strictEqual(a.multiply().value,13),globalThis.assert."
                  "strictEqual(a.multiply(10).value,130);var "
                  "e=a.multiply(-1);globalThis.assert.strictEqual(e.value,-13),globalThis.assert.strictEqual(e."
                  "valueReadonly,-13),globalThis.assert.notStrictEqual(a,e)})();",
                  "https://www.didi.com/6_object_wrap.js", &result),
              NAPIOK);
}