#include <iostream>

#define INFO 0
#define WARNING 0

#define LOG(severity)                                                                                                  \
    if (0)                                                                                                             \
    ::std::cout

#define CHECK(condition)                                                                                               \
    if (0)                                                                                                             \
    ::std::cout
#define CHECK_GE(val1, val2)
#define CHECK_EQ(val1, val2)

#define DCHECK(condition)
#define DCHECK_EQ(val1, val2)                                                                                          \
    if (0)                                                                                                             \
    ::std::cout
#define DCHECK_LE(val1, val2)
#define DCHECK_GT(val1, val2)
#define DCHECK_LT(val1, val2)
#define DCHECK_NE(val1, val2)
#define DCHECK_GE(val1, val2)

#define LOG_FIRST_N(severity, n)                                                                                       \
    if (0)                                                                                                             \
    ::std::cout