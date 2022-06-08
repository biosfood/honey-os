#include "elf.h"
#include "service.h"
#include <memory.h>
#include <service.h>
#include <util.h>

Service loaderService;
Service kernelService;
Context kernelContext;

extern void *kernelCodePageTable;

// these are temporary variables but I need the pointers in the asm parts
void *currentCr3 = 0;
void *currentEsp = 0;
void *mainFunction = NULL;
void *returnStack = 0;

extern void *functionsStart;
extern void(runFunction)();
extern void(runEnd)();

void run(Service *service, void *main) {
    currentEsp = malloc(0x1000);
    memset(currentEsp, 0, 0x1000);
    ((void **)currentEsp)[0x3FF] = runEnd;
    sharePage(&service->pagingInfo, currentEsp, currentEsp);
    currentCr3 = getPhysicalAddressKernel(service->pagingInfo.pageDirectory);
    mainFunction = main;
    asm("jmp runFunction");
}

void loadElf(void *elfStart) {
    // use this function ONLY to load the initrd/loader program(maybe also the
    // ELF loader service)!
    ElfHeader *header = elfStart;
    ProgramHeader *programHeader =
        elfStart + header->programHeaderTablePosition;
    PagingInfo *paging = &loaderService.pagingInfo;
    memset(paging, 0, sizeof(PagingInfo));
    paging->pageDirectory = malloc(0x1000);
    // todo: make this unwritable!
    // todo: use functionsStart as the reference
    sharePage(paging, PTR(0xFFC02000),
              PTR(0xFFC02000)); // functionsStart, functionsStart);
    for (uint32_t i = 0; i < header->programHeaderEntryCount; i++) {
        for (uint32_t page = 0; page < programHeader->segmentMemorySize;
             page += 0x1000) {
            void *data = malloc(0x1000);
            memset(data, 0, 0x1000);
            memcpy(elfStart + programHeader->dataOffset, data,
                   MIN(0x1000, programHeader->segmentFileSize - page));
            sharePage(paging, data, PTR(programHeader->virtualAddress + page));
        }
        programHeader = (void *)programHeader + header->programHeaderEntrySize;
    }
    run(&loaderService, PTR(header->entryPosition));
}
