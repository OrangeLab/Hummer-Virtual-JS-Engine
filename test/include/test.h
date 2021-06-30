#ifndef SKIA_TEST_H
#define SKIA_TEST_H

#include <gtest/gtest.h>
#include <napi/js_native_api.h>

class Test : public ::testing::Test
{

    void SetUp() override;

    void TearDown() override;

  protected:
    NAPIHandleScope handleScope;
    NAPIValue addonValue;
};

extern bool finalizeIsCalled;

extern NAPIEnv globalEnv;

#endif // SKIA_TEST_H
