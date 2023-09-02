#ifndef ALLOC_H
#define ALLOC_H

#include <stdint.h>

#include <stdbool.h>
extern void *getPage();
extern void *getPagesCount(uint32_t);
extern void freePage(void *);
extern void memset(void *, uint8_t, uint32_t);

#define LOG2(X) ((unsigned)(64 - __builtin_clzll((X)) - 1))

#define ALLOCATION_MAGIC 0xB105F00D // == biosfood

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

extern AllocationData allocationData;

extern void free(void *);
extern void *_malloc(AllocationData, uint32_t);
#define malloc(size) _malloc(allocationData, size)

#endif
