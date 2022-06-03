#ifndef MEMORY_H
#define MEMORY_H

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
    PageDirectoryEntry *pageDirectory;
    uint32_t isPageTableInUse[0x20];
    uint32_t isPageAllocated[0x8000];
    uint32_t isPageAllocatedCoarse[0x400];
    uint32_t pageSearchStart;
} PagingInfo;

extern void setupMemory();
extern void reservePagesUntilPhysical(uint32_t endPageId);
extern void memset(void *target, uint8_t byte, uint32_t size);
extern void *kernelMapPhysical(void *address);
extern void *kernelMapMultiplePhysicalPages(void *address, uint32_t size);
extern void *findTarFile(void *fileStart, uint32_t fileSize, char *filename);
extern void *mapPageForNewService(PagingInfo *info, void *virtualTarget);
extern void *malloc(uint32_t size);
extern void free(void *address);
extern void *getPage();
extern void sharePage(PagingInfo *destination, void *sourceAddress,
                      void *destinationAddress);

extern void *getPhysicalAddressKernel(void *address);

#endif
