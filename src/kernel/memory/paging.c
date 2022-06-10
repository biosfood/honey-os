#include "paging.h"
#include <memory.h>
#include <util.h>

PageTableEntry *kernelCodePageTable, *kernelDataPageTable;

PagingInfo *kernelPhysicalPages, *kernelVirtualPages;

void *temporaryPage;

void reservePage(PagingInfo *info, uint32_t pageId);

void invalidatePage(uint32_t pageId) {
    asm("invlpg (%0)" ::"r"(pageId << 12) : "memory");
}

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

void *mapTemporary(void *physical) {
    kernelDataPageTable[3].targetAddress = U32(physical) >> 12;
    kernelDataPageTable[3].writable = 1;
    kernelDataPageTable[3].present = 1;
    invalidatePage(0xFF803);
    return temporaryPage;
}

void *getPhysicalAddress(PageDirectoryEntry *pageDirectory, void *address) {
    VirtualAddress *virtual = (void *)&address;
    uint32_t pageTableId =
        pageDirectory[virtual->pageDirectoryIndex].pageTableID;
    PageTableEntry *pageTable = mapTemporary(PTR(pageTableId << 12));
    uint32_t pageBase = pageTable[virtual->pageTableIndex].targetAddress;
    return PTR(pageBase << 12 | virtual->pageOffset);
}

void *getPhysicalAddressKernel(void *address) {
    return getPhysicalAddress(kernelVirtualPages->pageDirectory, address);
}

void mapPage(PagingInfo *info, void *physical, void *virtual, bool userPage);

void reservePagesUntilPhysical(uint32_t endPageId) {
    void *buffer = (void *)0xFF800000;
    void *pageDirectory = buffer;
    memset(pageDirectory, 0, 0xFF8);
    buffer += 0x1000;
    kernelDataPageTable = buffer;
    buffer += 0x1000;
    kernelCodePageTable = buffer;
    buffer += 0x1000;
    temporaryPage = buffer;
    buffer += 0x1000;
    kernelPhysicalPages = buffer;
    buffer += sizeof(PagingInfo);
    kernelVirtualPages = buffer;
    buffer += sizeof(PagingInfo);
    memset(kernelPhysicalPages, 0, 2 * sizeof(PagingInfo));
    for (uint32_t i = 0; i < endPageId; i++) {
        reservePage(kernelPhysicalPages, i);
    }
    for (uint32_t i = 0; i < 0x800; i++) {
        reservePage(kernelVirtualPages, i + 0xFF800);
    }
    kernelPhysicalPages->pageSearchStart = endPageId;
    kernelVirtualPages->pageSearchStart = 0;
    kernelVirtualPages->pageDirectory = pageDirectory;
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
        reservePage(kernelPhysicalPages, physicalPageStart + i);
        reservePage(kernelVirtualPages, virtualPageStart + i);
    }
    for (uint32_t i = 0; i < size; i++) {
        mapPage(kernelVirtualPages, PTR((physicalPageStart + i) << 12),
                PTR((virtualPageStart + i) << 12), false);
    }
    return PTR((virtualPageStart << 12) + (U32(address) & 0xFFF));
}

void *kernelMapPhysical(void *address) {
    return kernelMapMultiplePhysicalPages(address, 1);
}

void mapPage(PagingInfo *info, void *physical, void *virtual, bool userPage) {
    VirtualAddress *address = (void *)&virtual;
    PageDirectoryEntry *directory = info->pageDirectory;
    if (!directory[address->pageDirectoryIndex].present) {
        uint32_t newPageTable = findPage(kernelPhysicalPages);
        reservePage(kernelPhysicalPages, newPageTable);
        void *temporary = mapTemporary(PTR(newPageTable << 12));
        memset(temporary, 0, 0x1000);
        directory[address->pageDirectoryIndex].pageTableID = newPageTable;
        directory[address->pageDirectoryIndex].present = 1;
        directory[address->pageDirectoryIndex].writable = 1;
        directory[address->pageDirectoryIndex].belongsToUserProcess |= userPage;
    }
    void *pageTablePhysical =
        PTR(directory[address->pageDirectoryIndex].pageTableID << 12);
    void *temporary = mapTemporary(pageTablePhysical);
    PageTableEntry *pageTable = temporary;
    pageTable[address->pageTableIndex].targetAddress = U32(physical) >> 12;
    pageTable[address->pageTableIndex].present = 1;
    pageTable[address->pageTableIndex].writable = 1;
    pageTable[address->pageTableIndex].belongsToUserProcess = userPage;
    invalidatePage(U32(virtual) >> 12);
}

void *getPage() {
    uint32_t physical = findPage(kernelPhysicalPages);
    reservePage(kernelPhysicalPages, physical);
    uint32_t virtual = findPage(kernelVirtualPages);
    reservePage(kernelVirtualPages, virtual);
    mapPage(kernelVirtualPages, PTR(physical << 12), PTR(virtual << 12), false);
    return PTR(virtual << 12);
}

void *getMultiplePages(uint32_t count) {
    uint32_t virtual = findMultiplePages(kernelVirtualPages, count);
    for (uint32_t i = 0; i < count; i++) {
        reservePage(kernelVirtualPages, virtual + i);
        uint32_t physical = findPage(kernelPhysicalPages);
        reservePage(kernelPhysicalPages, physical);
        mapPage(kernelVirtualPages, PTR(physical << 12),
                PTR((virtual + i) << 12), false);
    }
    return PTR(virtual << 12);
}

void sharePage(PagingInfo *destination, void *sourceAddress,
               void *destinationAddress) {
    PagingInfo *sourcePagingInfo = kernelVirtualPages;
    void *physicalSource =
        getPhysicalAddress(sourcePagingInfo->pageDirectory, sourceAddress);
    mapPage(destination, physicalSource, destinationAddress, true);
}
