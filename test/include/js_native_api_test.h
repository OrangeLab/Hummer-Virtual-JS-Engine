#ifndef SKIA_JS_NATIVE_API_TEST_H
#define SKIA_JS_NATIVE_API_TEST_H

#include <gtest/gtest.h>
#include <napi/js_native_api.h>

class Test : public ::testing::Test
{

    void SetUp() override;

    void TearDown() override;

  protected:
    NAPIValue exports;
};

extern NAPIEnv globalEnv;

#endif // SKIA_JS_NATIVE_API_TEST_H
