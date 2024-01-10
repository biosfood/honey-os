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
    kernelDataPageTable[3].targetAddress = PAGE_ID(physical);
    kernelDataPageTable[3].writable = 1;
    kernelDataPageTable[3].present = 1;
    invalidatePage(0xFF803);
    return temporaryPage + PAGE_OFFSET(physical);
}

void *getPhysicalAddress(PageDirectoryEntry *pageDirectory, void *address) {
    VirtualAddress *virtual = (void *)&address;
    uint32_t pageTableId =
        pageDirectory[virtual->pageDirectoryIndex].pageTableID;
    PageTableEntry *pageTable = mapTemporary(ADDRESS(pageTableId));
    uint32_t pageBase = pageTable[virtual->pageTableIndex].targetAddress;
    return ADDRESS(pageBase) + PAGE_OFFSET(address);
}

void *getPhysicalAddressKernel(void *address) {
    return getPhysicalAddress(kernelVirtualPages->pageDirectory, address);
}

void reservePage(PagingInfo *info, uint32_t pageId) {
    uint32_t coarsePosition = pageId / 32;
    info->isPageAllocated[coarsePosition] |= 1 << (pageId % 32);
    if (info->isPageAllocated[coarsePosition] == ~0) {
        info->isPageAllocatedCoarse[coarsePosition / 32] |=
            1 << (coarsePosition % 32);
    }
}

void reservePagesCount(PagingInfo *info, uint32_t startPageId, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        reservePage(info, startPageId + i);
    }
}

void mapPage(PagingInfo *info, void *physical, void *virtual, bool userPage,
             bool isVolatile);

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
    reservePagesCount(kernelVirtualPages, 0, 1);
    reservePagesCount(kernelPhysicalPages, 0, endPageId);
    reservePagesCount(kernelVirtualPages, 0xFF800, 0x800);
    kernelPhysicalPages->pageSearchStart = endPageId;
    kernelVirtualPages->pageSearchStart = 0;
    kernelVirtualPages->pageDirectory = pageDirectory;
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

void *kernelMapPhysicalCount(void *address, uint32_t size) {
    uint32_t physicalPageStart = PAGE_ID(address);
    reservePagesCount(kernelPhysicalPages, physicalPageStart, size);
    uint32_t virtualPageStart = findMultiplePages(kernelVirtualPages, size);
    reservePagesCount(kernelVirtualPages, virtualPageStart, size);
    for (uint32_t i = 0; i < size; i++) {
        mapPage(kernelVirtualPages, ADDRESS(physicalPageStart + i),
                ADDRESS(virtualPageStart + i), false, false);
        uint32_t pageId = virtualPageStart + i;
        kernelVirtualPages->isPageConnectedToNext[pageId / 32] |=
            1 << (pageId % 32);
    }
    return ADDRESS(virtualPageStart) + PAGE_OFFSET(address);
}

void *kernelMapPhysical(void *address) {
    uint32_t physicalPageId = PAGE_ID(address);
    reservePage(kernelPhysicalPages, physicalPageId);
    uint32_t virtualPageId = findPage(kernelVirtualPages);
    reservePage(kernelVirtualPages, virtualPageId);
    mapPage(kernelVirtualPages, ADDRESS(physicalPageId), ADDRESS(virtualPageId),
            false, false);
    return ADDRESS(virtualPageId) + PAGE_OFFSET(address);
}

void mapPage(PagingInfo *info, void *physical, void *virtual, bool userPage,
             bool isVolatile) {
    VirtualAddress *address = (void *)&virtual;
    PageDirectoryEntry *directory = info->pageDirectory;
    if (!directory[address->pageDirectoryIndex].present) {
        uint32_t newPageTable = findPage(kernelPhysicalPages);
        reservePage(kernelPhysicalPages, newPageTable);
        void *temporary = mapTemporary(ADDRESS(newPageTable));
        memset(temporary, 0, 0x1000);
        directory[address->pageDirectoryIndex].pageTableID = newPageTable;
        directory[address->pageDirectoryIndex].present = 1;
        directory[address->pageDirectoryIndex].writable = 1;
        directory[address->pageDirectoryIndex].belongsToUserProcess |= userPage;
    }
    void *pageTablePhysical =
        ADDRESS(directory[address->pageDirectoryIndex].pageTableID);
    void *temporary = mapTemporary(pageTablePhysical);
    PageTableEntry *pageTable = temporary;
    pageTable[address->pageTableIndex].targetAddress = PAGE_ID(physical);
    pageTable[address->pageTableIndex].present = 1;
    pageTable[address->pageTableIndex].writable = 1;
    pageTable[address->pageTableIndex].isVolatile = isVolatile;
    pageTable[address->pageTableIndex].belongsToUserProcess = userPage;
    invalidatePage(PAGE_ID(virtual));
}

void *getPage() {
    uint32_t physical = findPage(kernelPhysicalPages);
    reservePage(kernelPhysicalPages, physical);
    uint32_t virtual = findPage(kernelVirtualPages);
    reservePage(kernelVirtualPages, virtual);
    mapPage(kernelVirtualPages, ADDRESS(physical), ADDRESS(virtual), false,
            false);
    return ADDRESS(virtual);
}

void *getPagesCount(uint32_t count) {
    uint32_t virtualPageId = findMultiplePages(kernelVirtualPages, count);
    reservePagesCount(kernelVirtualPages, virtualPageId, count);
    for (uint32_t i = 0; i < count; i++) {
        uint32_t physical = findPage(kernelPhysicalPages);
        reservePage(kernelPhysicalPages, physical);
        mapPage(kernelVirtualPages, ADDRESS(physical),
                ADDRESS(virtualPageId + i), false, false);
    }
    return ADDRESS(virtualPageId);
}

void *sharePage(PagingInfo *destination, void *sourceAddress,
                void *destinationAddress) {
    PagingInfo *sourcePagingInfo = kernelVirtualPages;
    void *physicalSource =
        getPhysicalAddress(sourcePagingInfo->pageDirectory, sourceAddress);
    if (!destinationAddress) {
        void *target = ADDRESS(findPage(destination));
        mapPage(destination, physicalSource, target, true, false);
        return target + PAGE_OFFSET(sourceAddress);
    }
    mapPage(destination, physicalSource, destinationAddress, true, false);
    return destinationAddress;
}

void unmapSinglePageFrom(PagingInfo *info, void *pageAddress) {
    VirtualAddress *address = (void *)&pageAddress;
    PageDirectoryEntry *directory = info->pageDirectory;
    void *pageTablePhysical =
        ADDRESS(directory[address->pageDirectoryIndex].pageTableID);
    void *temporary = mapTemporary(pageTablePhysical);
    PageTableEntry *pageTable = temporary;
    pageTable[address->pageTableIndex].targetAddress = 0;
    pageTable[address->pageTableIndex].present = 0;
    invalidatePage(PAGE_ID(pageAddress));
}

void markPageFree(PagingInfo *info, uint32_t coarse, uint32_t fine,
                  uint32_t fineBit) {
    info->isPageAllocated[coarse] &= ~fineBit;
    info->isPageConnectedToNext[coarse] &= ~fineBit;
    info->isPageAllocatedCoarse[coarse / 32] &= ~(1 << (coarse % 32));
}

void unmapPageFrom(PagingInfo *info, void *address) {
    uint32_t pageId = PAGE_ID(address), coarse, fine, fineBit;
    do {
        coarse = pageId / 32;
        fine = pageId % 32;
        fineBit = 1 << fine;
        markPageFree(info, coarse, fine, fineBit);
        unmapSinglePageFrom(info, ADDRESS(pageId));
        pageId++;
    } while (info->isPageConnectedToNext[coarse] & fineBit);
}

void giveUpPage(PagingInfo *info, uint32_t pageId) {
    uint32_t coarse, fine, fineBit;
    do {
        {
            // mark physical page as free
            void *physical = getPhysicalAddress(info->pageDirectory, ADDRESS(pageId));
            uint32_t physicalId = PAGE_ID(physical);
            uint32_t coarse = physicalId / 32;
            uint32_t fine = physicalId % 32;
            uint32_t fineBit = 1 << fine;
            markPageFree(kernelPhysicalPages, coarse, fine, fineBit);
        }

        // mark virtual page as free
        coarse = pageId / 32;
        fine = pageId % 32;
        fineBit = 1 << fine;
        markPageFree(info, coarse, fine, fineBit);
        unmapSinglePageFrom(info, ADDRESS(pageId));
        pageId++;
    } while (info->isPageConnectedToNext[coarse] & fineBit);

}

void unmapPage(void *pageAddress) {
    unmapPageFrom(kernelVirtualPages, pageAddress);
}

void freePageFrom(PagingInfo *info, void *address) {
    uint32_t pageId = PAGE_ID(address), coarse, fine, fineBit;
    do {
        coarse = pageId / 32;
        fine = pageId % 32;
        fineBit = 1 << fine;
        markPageFree(info, coarse, fine, fineBit);
        PageTableEntry *pageTable = mapTemporary(
            ADDRESS(info->pageDirectory[PAGE_ID(pageId)].pageTableID));
        uint32_t physicalPageId = pageTable[PAGE_OFFSET(pageId)].targetAddress;
        uint32_t physicalFine = physicalPageId % 32;
        markPageFree(kernelPhysicalPages, physicalPageId / 32, physicalFine,
                     1 << physicalFine);
        unmapSinglePageFrom(info, ADDRESS(pageId));
        pageId++;
    } while (info->isPageConnectedToNext[coarse] & fineBit);
}

void freePage(void *address) { freePageFrom(kernelVirtualPages, address); }

void freePhysicalPage(uint32_t pageId) {
    markPageFree(kernelPhysicalPages, pageId / 32, pageId % 32,
                 1 << (pageId % 32));
}
