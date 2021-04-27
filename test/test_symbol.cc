#include <gtest/gtest.h>
#include <napi/js_native_api.h>
#include <napi_assert.h>

EXTERN_C_START

static NAPIValue New(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPIValue description = NULL;
    if (argc >= 1) {
        NAPIValueType valuetype;
        NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));

        NAPI_ASSERT(env, valuetype == NAPIString,
                    "Wrong type of arguments. Expects a string.");

        description = args[0];
    }

    NAPIValue symbol;
    NAPI_CALL(env, napi_create_symbol(env, description, &symbol));

    return symbol;
}

EXTERN_C_END

TEST(TestSymbol, Test) {
    NAPIEnv env = nullptr;
    ASSERT_EQ(NAPICreateEnv(&env), NAPIOK);

    NAPIValue global = nullptr;
    ASSERT_EQ(napi_get_global(env, &global), NAPIOK);

    NAPIValue exports;
    ASSERT_EQ(napi_create_object(env, &exports), NAPIOK);
    ASSERT_EQ(napi_set_named_property(env, global, "exports", exports), NAPIOK);

    NAPIPropertyDescriptor properties[] = {
            DECLARE_NAPI_PROPERTY("New", New),
    };

    ASSERT_EQ(napi_define_properties(
            env, exports, sizeof(properties) / sizeof(*properties), properties), NAPIOK);

    EXPECT_EQ(initAssert(env, global), NAPIOK);

    NAPIValue result = nullptr;
    ASSERT_EQ(NAPIRunScriptWithSourceUrl(env, "(function () {\n"
                                              "    const sym = globalThis.exports.New('test');\n"
                                              "    assert.strictEqual(sym.toString(), 'Symbol(test)');\n"
                                              "\n"
                                              "    const myObj = {};\n"
                                              "    const fooSym = globalThis.exports.New('foo');\n"
                                              "    const otherSym = globalThis.exports.New('bar');\n"
                                              "    myObj.foo = 'bar';\n"
                                              "    myObj[fooSym] = 'baz';\n"
                                              "    myObj[otherSym] = 'bing';\n"
                                              "    assert.strictEqual(myObj.foo, 'bar');\n"
                                              "    assert.strictEqual(myObj[fooSym], 'baz');\n"
                                              "    assert.strictEqual(myObj[otherSym], 'bing');\n"
                                              "})();",
                                         "https://n-api.com/test_symbol_test1.js",
                                         &result), NAPIOK);
    NAPIValueType valueType;
    EXPECT_EQ(napi_typeof(env, result, &valueType), NAPIOK);
    EXPECT_EQ(valueType, NAPIUndefined);

    ASSERT_EQ(NAPIRunScriptWithSourceUrl(env, "(function () {\n"
                                              "    const fooSym = globalThis.exports.New('foo');\n"
                                              "    const myObj = {};\n"
                                              "    myObj.foo = 'bar';\n"
                                              "    myObj[fooSym] = 'baz';\n"
                                              "    Object.keys(myObj); // -> [ 'foo' ]\n"
                                              "    Object.getOwnPropertyNames(myObj); // -> [ 'foo' ]\n"
                                              "    Object.getOwnPropertySymbols(myObj); // -> [ Symbol(foo) ]\n"
                                              "    assert.strictEqual(Object.getOwnPropertySymbols(myObj)[0], fooSym);\n"
                                              "})();",
                                         "https://n-api.com/test_symbol_test2.js",
                                         &result), NAPIOK);
    EXPECT_EQ(napi_typeof(env, result, &valueType), NAPIOK);
    EXPECT_EQ(valueType, NAPIUndefined);

    ASSERT_EQ(NAPIRunScriptWithSourceUrl(env, "(function () {\n"
                                              "    assert.notStrictEqual(globalThis.exports.New(), globalThis.exports.New());\n"
                                              "    assert.notStrictEqual(globalThis.exports.New('foo'), globalThis.exports.New('foo'));\n"
                                              "    assert.notStrictEqual(globalThis.exports.New('foo'), globalThis.exports.New('bar'));\n"
                                              "\n"
                                              "    const foo1 = globalThis.exports.New('foo');\n"
                                              "    const foo2 = globalThis.exports.New('foo');\n"
                                              "    const object = {\n"
                                              "        [foo1]: 1,\n"
                                              "        [foo2]: 2,\n"
                                              "    };\n"
                                              "    assert.strictEqual(object[foo1], 1);\n"
                                              "    assert.strictEqual(object[foo2], 2);\n"
                                              "})();",
                                         "https://n-api.com/test_symbol_test3.js",
                                         &result), NAPIOK);
    EXPECT_EQ(napi_typeof(env, result, &valueType), NAPIOK);
    EXPECT_EQ(valueType, NAPIUndefined);

    ASSERT_EQ(NAPIFreeEnv(env), NAPIOK);
}