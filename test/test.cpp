#include <gtest/gtest.h>

int add(int a, int b)
{
    return a + b;
}

TEST(myfunctions, add)
{
    GTEST_ASSERT_EQ(add(10, 22), 32);
}