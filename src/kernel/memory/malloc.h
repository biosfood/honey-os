#ifndef ALLOC_H
#define ALLOC_H

#include <memory.h>
#include <stdint.h>

#define ALLOCATION_MAGIC 0x44BB33DD

typedef AllocationBlock *AllocationData[12];

#endif
