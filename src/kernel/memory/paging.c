#include "paging.h"
#include <memory.h>

PageDirectoryEntry *kernelDirectory = (void *)0xFF800000;
PageTableEntry *kernelPageTable = (void *)0xFF801000;
PageTableEntry *kernelDataPageTable = (void *)0xFF802000;
uint32_t *isPageAllocated = (void *)0xFF803000;
uint32_t *isPageAllocatedCoarse = (void *)0xFF843000;
uint32_t pageSearchStart;

void reservePage(uint32_t pageId);

void reservePagesUntil(uint32_t endPageId) {
    memset(isPageAllocated, 0, 0x40000);
    memset(isPageAllocatedCoarse, 0, 0x10000);
    for (uint32_t i = 0; i < endPageId; i++) {
        reservePage(i);
    }
    pageSearchStart = endPageId;
}

void reservePage(uint32_t pageId) {
    uint32_t coarsePosition = pageId / 32;
    isPageAllocated[coarsePosition] |= 1 << (pageId % 32);
    if (isPageAllocated[coarsePosition] == ~0) {
        isPageAllocatedCoarse[coarsePosition / 32] |= 1
                                                      << (coarsePosition % 32);
    }
}
