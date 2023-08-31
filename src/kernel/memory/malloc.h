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

extern void free(void *);

extern void _printf(void *(malloc)(uint32_t), const char *format, ...);
extern char *_asprintf(void *(malloc)(uint32_t), const char *format, ...);

#define printf(...) _printf(malloc, __VA_ARGS__)
#define asprintf(...) _asprintf(malloc, __VA_ARGS__)

#ifdef ALLOC_MAIN
#undef ALLOC_MAIN

AllocationData allocationData;

extern void *_malloc(void *, uintptr_t);
void *malloc(uint32_t size) { _malloc(&allocationData, size); }

#else
extern void *malloc(uint32_t size);

#endif

#endif
