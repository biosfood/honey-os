#include "elf.h"
#include "service.h"
#include <memory.h>
#include <service.h>
#include <util.h>

PagingInfo testInfo;

void loadElf(void *elfStart) {
    // use this function ONLY to load the initrd/loader program!
    ElfHeader *header = elfStart;
    ProgramHeader *programHeader =
        elfStart + header->programHeaderTablePosition;
    PagingInfo *paging = &testInfo;
    memset(paging, 0, sizeof(PagingInfo));
    void *data;
    for (uint32_t i = 0; i < header->programHeaderEntryCount; i++) {
        for (uint32_t page = 0; page < programHeader->segmentMemorySize;
             page += 0x1000) {
            data = malloc(0x1000);
            memset(data, 0, 0x1000);
            memcpy(elfStart + programHeader->dataOffset, data,
                   MIN(0x1000, programHeader->segmentFileSize - page));
            // sharePage(paging, data,
            //          elfStart + programHeader->dataOffset + page);
        }
        programHeader = (void *)programHeader + header->programHeaderEntrySize;
    }
    asm("mov %%eax, %0" ::"r"(U32(data)));
    while (1)
        ;
}
