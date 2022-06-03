#ifndef ALLOC_H
#define ALLOC_H

#include <stdint.h>

#define ALLOCATION_MAGIC 0x44BB33DD

typedef struct AllocationBlock {
    uint8_t data[3948];
    uint32_t allocatedFine[32];
    uint32_t allocatedCoarse;
    uint32_t blockSize;
    struct AllocationBlock *next;
    struct AllocationBlock *previous;
    uint32_t magic;
} AllocationBlock;

typedef AllocationBlock *AllocationData[12];

#endif
