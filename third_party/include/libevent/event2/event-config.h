#include <stdint.h>

#if SIZE_MAX == UINT64_MAX
#define EVENT__SIZEOF_SIZE_T 8
#elif SIZE_MAX == UINT32_MAX
#define EVENT__SIZEOF_SIZE_T 4
#else
#error "No way to infer sizeof size_t"
#endif
