#include <iostream>

#define INFO
#define WARNING

#define LOG(severity) ::std::cout

#define CHECK(condition) ::std::cout
#define CHECK_EQ(val1, val2)
#define CHECK_GE(val1, val2)

#define DCHECK(condition)
#define DCHECK_EQ(val1, val2) ::std::cout
#define DCHECK_GT(val1, val2)
#define DCHECK_LT(val1, val2)
#define DCHECK_NE(val1, val2)
#define DCHECK_GE(val1, val2)
#define DCHECK_LE(val1, val2)