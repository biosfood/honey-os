#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

typedef struct {
    uint32_t present : 1;
    uint32_t writable : 1;
    uint32_t belongsToUserProcess : 1;
    uint32_t reserved : 2;
    uint32_t accessed : 1;
    uint32_t dirty : 1;
    uint32_t reserved2 : 2;
    uint32_t available : 3;
    uint32_t pageTableID : 22;
} PageDirectoryEntry;

typedef struct {
    uint32_t present : 1;
    uint32_t writable : 1;
    uint32_t belongsToUserProcess : 1;
    uint32_t writeThrough : 1;
    uint32_t isVolatile : 1; // disables the cpu cache
    uint32_t accessed : 1;
    uint32_t reserved : 1;
    uint32_t is4MBPage : 1;
    uint32_t global : 1;
    uint32_t available : 3;
    uint32_t targetAddress : 20;
} PageTableEntry;

#endif
