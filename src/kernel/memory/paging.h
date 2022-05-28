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
    uint32_t pageTableID : 20;
} __attribute__((packed)) PageDirectoryEntry;

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
} __attribute__((packed)) PageTableEntry;

typedef struct {
    uint32_t isPageAllocated[0x8000];
    uint32_t isPageAllocatedCoarse[0x400];
    uint32_t isPageTableInUse[0x20];
    uint32_t pageSearchStart;
} PagingInfo;

typedef struct {
    uint32_t pageOffset : 12;
    uint32_t pageTableIndex : 10;
    uint32_t pageDirectoryIndex : 10;
} __attribute__((packed)) VirtualAddress;

#endif
