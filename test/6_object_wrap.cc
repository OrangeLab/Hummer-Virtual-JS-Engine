#include <gtest/gtest.h>
#include <napi/js_native_api.h>
#include <common.h>

class MyObject {
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

NAPIRef MyObject::constructor;

MyObject::MyObject(double value)
        : value_(value), env_(nullptr), wrapper_(nullptr) {}

MyObject::~MyObject() { napi_delete_reference(env_, wrapper_); }

void MyObject::Destructor(
        NAPIEnv /*env*/, void *nativeObject, void * /*finalize_hint*/) {
    auto obj = static_cast<MyObject *>(nativeObject);
    delete obj;
}

void MyObject::Init(NAPIEnv env, NAPIValue exports) {
//    NAPIPropertyDescriptor properties[] = {
//            {"value", nullptr, nullptr, GetValue, SetValue, nullptr, NAPIDefault, nullptr},
//            {"valueReadonly", nullptr, nullptr, GetValue, nullptr, nullptr, NAPIDefault,
//             nullptr},
//            DECLARE_NAPI_PROPERTY("plusOne", PlusOne),
//            DECLARE_NAPI_PROPERTY("multiply", Multiply),
//    };

    NAPIValue cons;
    NAPI_CALL_RETURN_VOID(env, napi_define_class(
            env, "MyObject", -1, New, nullptr,
            0,
            nullptr, &cons));

    NAPI_CALL_RETURN_VOID(env, napi_create_reference(env, cons, 1, &constructor));

    NAPI_CALL_RETURN_VOID(env,
                          napi_set_named_property(env, exports, "MyObject", cons));
}

NAPIValue MyObject::New(NAPIEnv env, NAPICallbackInfo info) {
    NAPIValue new_target;
    NAPI_CALL(env, napi_get_new_target(env, info, &new_target));
    bool is_constructor = (new_target != nullptr);

    size_t argc = 1;
    NAPIValue args[1];
    NAPIValue _this;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &_this, nullptr));

    if (is_constructor) {
        // Invoked as constructor: `new MyObject(...)`
        double value = 0;

        NAPIValueType valuetype;
        NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));

        if (valuetype != NAPIUndefined) {
            NAPI_CALL(env, napi_get_value_double(env, args[0], &value));
        }

        auto obj = new MyObject(value);

        obj->env_ = env;
        NAPI_CALL(env, napi_wrap(env,
                                 _this,
                                 obj,
                                 MyObject::Destructor,
                                 nullptr,  // finalize_hint
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

NAPIValue MyObject::GetValue(NAPIEnv env, NAPICallbackInfo info) {
    NAPIValue _this;
    NAPI_CALL(env,
              napi_get_cb_info(env, info, nullptr, nullptr, &_this, nullptr));

    MyObject *obj;
    NAPI_CALL(env, napi_unwrap(env, _this, reinterpret_cast<void **>(&obj)));

    NAPIValue num;
    NAPI_CALL(env, napi_create_double(env, obj->value_, &num));

    return num;
}

NAPIValue MyObject::SetValue(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPIValue _this;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &_this, nullptr));

    MyObject *obj;
    NAPI_CALL(env, napi_unwrap(env, _this, reinterpret_cast<void **>(&obj)));

    NAPI_CALL(env, napi_get_value_double(env, args[0], &obj->value_));

    return nullptr;
}

NAPIValue MyObject::PlusOne(NAPIEnv env, NAPICallbackInfo info) {
    NAPIValue _this;
    NAPI_CALL(env,
              napi_get_cb_info(env, info, nullptr, nullptr, &_this, nullptr));

    MyObject *obj;
    NAPI_CALL(env, napi_unwrap(env, _this, reinterpret_cast<void **>(&obj)));

    obj->value_ += 1;

    NAPIValue num;
    NAPI_CALL(env, napi_create_double(env, obj->value_, &num));

    return num;
}

NAPIValue MyObject::Multiply(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPIValue _this;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &_this, nullptr));

    double multiple = 1;
    if (argc >= 1) {
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

TEST(ObjectWrap, MyObject) {
    NAPIEnv env = nullptr;
    ASSERT_EQ(NAPICreateEnv(&env), NAPIOK);

    NAPIValue global = nullptr;
    ASSERT_EQ(napi_get_global(env, &global), NAPIOK);

    NAPIValue exports;
    ASSERT_EQ(napi_create_object(env, &exports), NAPIOK);
    ASSERT_EQ(napi_set_named_property(env, global, "exports", exports), NAPIOK);

    MyObject::Init(env, exports);

//    EXPECT_EQ(initAssert(env, global), NAPIOK);

    NAPIValue result = nullptr;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(env, "(function () {\n"
                                              "    const obj = new globalThis.exports.MyObject(9);\n"
                                              "})();",
                                         "https://n-api.com/6_object_wrap.js",
                                         &result), NAPIOK);
    NAPIValueType valueType;
    EXPECT_EQ(napi_typeof(env, result, &valueType), NAPIOK);
    EXPECT_EQ(valueType, NAPIUndefined);

    ASSERT_EQ(NAPIFreeEnv(env), NAPIOK);
}