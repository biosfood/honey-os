#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>

#define U32(x) (uint32_t)(uintptr_t)(x)
#define PTR(x) (void *)(uintptr_t)(x)

#define NULL PTR(0)

#define PAGE_COUNT(x) (((x - 1) / 4096) + 1)

extern bool stringEquals(char *string1, char *string2);

#endif
