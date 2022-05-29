#include "paging.h"
#include <memory.h>
#include <util.h>

PageDirectoryEntry *kernelDirectory;
PageTableEntry *kernelCodePageTable, *kernelDataPageTable, *kernelUtilityPages;

PagingInfo *physicalPages, *kernelVirtualPages;

void reservePage(PagingInfo *info, uint32_t pageId);

void *kernelGetVirtualAddress(void *_address) {
    uint32_t address = (uint32_t)(uintptr_t)_address;
    if (address < 0x100000) {
        return 0;
    }
    if (address <= 0x500000) {
        return _address + 0xFFC00000 - 0x100000;
    }
    if (address <= 0x900000) {
        return _address + 0xFF800000 - 0x500000;
    }
    return 0;
}

void *getPhysicalAddress(void *address, PageDirectoryEntry *pageDirectory) {
    VirtualAddress *virtual = (void *)&address;
    uint32_t pageTableLocation =
        pageDirectory[virtual->pageDirectoryIndex].pageTableID;
    PageTableEntry *pageTable = PTR(pageTableLocation << 12);
    pageTable = kernelGetVirtualAddress(pageTable);
    uint32_t pageBase = pageTable[virtual->pageTableIndex].targetAddress;
    return PTR(pageBase << 12 | virtual->pageOffset);
}

void mapDirectoryEntry(PageDirectoryEntry *directory, uint32_t pageTableIndex,
                       PageTableEntry *pageTable, PagingInfo *pagingInfo) {
    directory[pageTableIndex].pageTableID =
        U32(getPhysicalAddress(pageTable, directory)) >> 12;
    directory[pageTableIndex].writable = 1;
    directory[pageTableIndex].present = 1;
    pagingInfo->isPageTableInUse[pageTableIndex / 32] |=
        1 << (pageTableIndex % 32);
}

void reservePagesUntilPhysical(uint32_t endPageId) {
    void *buffer = (void *)0xFF800000;
    kernelDirectory = buffer;
    memset(kernelDirectory, 0, 0xFF8);
    buffer += 0x1000;
    kernelCodePageTable = buffer;
    buffer += 0x1000;
    kernelDataPageTable = buffer;
    buffer += 0x1000;
    kernelUtilityPages = buffer;
    buffer += 0x1000;
    physicalPages = buffer;
    buffer += sizeof(PagingInfo);
    kernelVirtualPages = buffer;
    buffer += sizeof(PagingInfo);
    memset(physicalPages, 0, 2 * sizeof(PagingInfo));
    for (uint32_t i = 0; i < endPageId; i++) {
        reservePage(physicalPages, i);
    }
    for (uint32_t i = 0; i < 0x800; i++) {
        reservePage(kernelVirtualPages, i + 0xFF800);
    }
    memset(kernelUtilityPages, 0, 0x1000);
    mapDirectoryEntry(kernelDirectory, 0, kernelUtilityPages,
                      kernelVirtualPages);
    physicalPages->pageSearchStart = endPageId;
}

void reservePage(PagingInfo *info, uint32_t pageId) {
    uint32_t coarsePosition = pageId / 32;
    info->isPageAllocated[coarsePosition] |= 1 << (pageId % 32);
    if (info->isPageAllocated[coarsePosition] == ~0) {
        info->isPageAllocatedCoarse[coarsePosition / 32] |=
            1 << (coarsePosition % 32);
    }
}

uint32_t findMultiplePages(PagingInfo *info, uint32_t size) {
    for (uint32_t veryCoarse = info->pageSearchStart / 1024;; veryCoarse++) {
        if (info->isPageAllocatedCoarse[veryCoarse] == ~0) {
            continue;
        }
        for (uint8_t coarse = 0; coarse < 32; coarse++) {
            if (info->isPageAllocatedCoarse[veryCoarse] & (1 << coarse)) {
                continue;
            }
            uint32_t coarsePageId = veryCoarse * 32 + coarse;
            for (uint8_t fine = 0; fine < 32; fine++) {
                bool fail = false;
                for (uint32_t check = 0; check < size; check++) {
                    uint32_t currentFine = fine + check;
                    if (info->isPageAllocated[coarsePageId + currentFine / 32] &
                        (1 << (currentFine % 32))) {
                        fail = true;
                        break;
                    }
                }
                if (fail) {
                    continue;
                }
                return coarsePageId * 32 + fine;
            }
        }
    }
}

uint32_t findPage(PagingInfo *info) { return findMultiplePages(info, 1); }

void *kernelMapMultiplePhysicalPages(void *address, uint32_t size) {
    uint32_t physicalPageStart = U32(address) >> 12;
    uint32_t virtualPageStart = findMultiplePages(kernelVirtualPages, size);
    for (uint32_t i = 0; i < size; i++) {
        reservePage(physicalPages, physicalPageStart + i);
        reservePage(kernelVirtualPages, virtualPageStart + i);
        kernelUtilityPages[virtualPageStart + i].targetAddress =
            physicalPageStart + i;
        kernelUtilityPages[virtualPageStart + i].writable = 1;
        kernelUtilityPages[virtualPageStart + i].present = 1;
    }
    return PTR((virtualPageStart << 12) + (U32(address) & 0xFFF));
}

void *kernelMapPhysical(void *address) {
    return kernelMapMultiplePhysicalPages(address, 1);
}
