#include "malloc.h"
#include <memory.h>

#define LOG2(X) ((unsigned)(64 - __builtin_clzll((X)) - 1))

AllocationData allocationData;

void *reserveBlock(AllocationBlock *block, uint8_t coarse, uint8_t fine) {
    block->allocatedFine[coarse] |= 1 << fine;
    block->allocatedCoarse |= 1 << coarse;
    for (uint8_t i = 0; i < 32; i++) {
        if (block->allocatedFine[coarse] & 1 << i) {
            continue;
        }
        block->allocatedCoarse &= ~(1 << coarse);
        break;
    }
    void *result = ((uint8_t *)block) + block->blockSize * (32 * coarse + fine);
    memset(result, 0, block->blockSize);
    return result;
}

void *malloc(uint32_t size) {
    uint32_t sizeBit = LOG2(size) + 1;
    if (sizeBit > 10) {
        return getMultiplePages(((size - 1) >> 12) + 1);
    }
    AllocationBlock *block = allocationData[sizeBit], *last = 0;
    while (1) {
        if (!block) {
            block = getPage();
            memset(block, 0, 4096);
            block->blockSize = 1 << sizeBit;
            if (last) {
                block->previous = last;
                last->next = block;
            } else {
                allocationData[sizeBit] = block;
                block->previous = (void *)(uintptr_t)sizeBit;
            }
            block->magic = ALLOCATION_MAGIC;
        }
        if (block->allocatedCoarse == ~0) {
            goto end;
        }
        for (uint8_t coarse = 0; coarse < 32; coarse++) {
            for (uint8_t fine = 0; fine < 32; fine++) {
                if (block->allocatedFine[coarse] & (1 << fine)) {
                    continue;
                }
                return reserveBlock(block, coarse, fine);
            }
        }
    end:
        last = block;
        block = block->next;
    }
}

void free(void *location) {
    AllocationBlock *block = (void *)((uintptr_t)location & ~0xFFF);
    if (block->magic != ALLOCATION_MAGIC) {
        // freePage(location);
        return;
    }
    uint16_t index = (uint16_t)((uintptr_t)location & 0xFFF) / block->blockSize;
    uint8_t coarse = index / 32;
    uint8_t fine = index % 32;
    block->allocatedFine[coarse] &= ~(1 << fine);
    block->allocatedCoarse &= ~(1 << coarse);
    for (uint8_t i = 0; i < 32; i++) {
        if (block->allocatedFine[coarse] & (1 << i)) {
            continue;
        }
        block->allocatedCoarse |= 1 << coarse;
        return;
    }
}
