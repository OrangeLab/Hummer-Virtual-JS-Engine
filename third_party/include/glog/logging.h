#include <iostream>

#define INFO
#define WARNING

#define LOG(severity)                                                                                                  \
    if (0)                                                                                                             \
    ::std::cout

#define CHECK(condition)
#define CHECK_EQ(val1, val2)

#define DCHECK(condition)
#define DCHECK_EQ(val1, val2)                                                                                          \
    if (0)                                                                                                             \
    ::std::cout
#define DCHECK_GT(val1, val2)
#define DCHECK_LT(val1, val2)
#define DCHECK_NE(val1, val2)
#define DCHECK_GE(val1, val2)