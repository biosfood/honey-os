#include "service.h"
#include "elf.h"
#include <memory.h>
#include <service.h>
#include <stdarg.h>
#include <stringmap.h>
#include <util.h>

extern void *functionsStart;
extern void *functionsEnd;
extern void(runFunction)();

ListElement *services, *callsToProcess;
Syscall *currentSyscall;
extern Service *hlib;
extern Event *loadInitrdEvent;

void resume(Syscall *syscall) {
    if (U32(syscall) < 0x1000) {
        asm("hlt" ::"a"(syscall));
    }
    currentSyscall = syscall;
    runFunction();
}

Service *loadElf(void *elfStart, char *serviceName) {
    // use this function ONLY to load the initrd/loader program(maybe also the
    // ELF loader service)!
    ElfHeader *header = elfStart;
    ProgramHeader *programHeader =
        elfStart + header->programHeaderTablePosition;
    Service *service = malloc(sizeof(Service));
    memset(service, 0, sizeof(Service));
    service->pagingInfo.pageDirectory = malloc(0x1000);
    service->name = serviceName;
    service->nameHash = insertString(serviceName);
    service->id = listCount(services);
    fireEvent(loadInitrdEvent, service->nameHash, 0);
    void *current = &functionsStart;
    if (hlib) {
        service->pagingInfo.pageDirectory[0x3FC].pageTableID =
            hlib->pagingInfo.pageDirectory[0x3FC].pageTableID;
        service->pagingInfo.pageDirectory[0x3FC].belongsToUserProcess = 1;
        service->pagingInfo.pageDirectory[0x3FC].present = 1;
        service->pagingInfo.pageDirectory[0x3FC].writable = 1;
    }
    for (uint32_t i = 0; i < 3; i++) {
        // todo: make this unwritable!
        sharePage(&(service->pagingInfo), current, current);
        current += 0x1000;
    }
    for (uint32_t i = 0; i < header->programHeaderEntryCount; i++) {
        if (hlib && programHeader->virtualAddress >= 0xF0000000) {
            goto end;
        }
        for (uint32_t page = 0; page < programHeader->segmentMemorySize;
             page += 0x1000) {
            void *data = malloc(0x1000);
            if (programHeader->segmentFileSize > page) {
                memcpy(elfStart + programHeader->dataOffset + page, data,
                       MIN(0x1000, programHeader->segmentFileSize - page));
            }
            sharePage(&service->pagingInfo, data,
                      PTR(programHeader->virtualAddress + page));
        }
    end:
        programHeader = (void *)programHeader + header->programHeaderEntrySize;
    }
    for (uint32_t i = 0; i < header->sectionHeaderEntryCount; i++) {
        SectionHeader *sectionHeader = elfStart +
                                       header->sectionHeaderTablePosition +
                                       i * header->sectionHeaderEntrySize;
        if (sectionHeader->type == 2 && !service->symbolTable) {
            service->symbolTable = elfStart + sectionHeader->offset;
            service->symbolTableSize = sectionHeader->size;
        }
        if (sectionHeader->type == 3 && !service->stringTable) {
            service->stringTable = elfStart + sectionHeader->offset;
        }
    }
    ServiceFunction *main = malloc(sizeof(ServiceFunction));
    main->name = "main";
    main->service = service;
    main->address = PTR(header->entryPosition);
    listAdd(&services, service);
    listAdd(&service->functions, main);
    return service;
}

Service *findService(char *name) {
    foreach (services, Service *, service, {
        if (stringEquals(service->name, name)) {
            return service;
        }
    })
        ;
    return NULL;
}

ServiceFunction *findFunction(Service *service, char *name) {
    foreach (service->functions, ServiceFunction *, provider, {
        if (stringEquals(provider->name, name)) {
            return provider;
        }
    })
        ;
    return NULL;
}

void scheduleFunction(ServiceFunction *provider, Syscall *respondingTo, ...) {
    va_list valist;
    va_start(valist, respondingTo);
    uint32_t parameterCount = 0;
    while (va_arg(valist, uint32_t) || parameterCount <= 3) {
        parameterCount++;
    }
    va_start(valist, respondingTo);

    Syscall *runCall = malloc(sizeof(Syscall));
    runCall->function = 0;
    runCall->esp = malloc(0x1000); // todo: free this
    runCall->respondingTo = respondingTo;
    runCall->cr3 =
        getPhysicalAddressKernel(provider->service->pagingInfo.pageDirectory);
    runCall->service = provider->service;
    runCall->resume = true;
    sharePage(&provider->service->pagingInfo, runCall->esp, runCall->esp);
    runCall->esp += 0x1000 - 0x10 - (parameterCount * 0x4);
    *(void **)runCall->esp = provider->address;
    *(void **)(runCall->esp + 0x4) = &runEnd;
    for (uint32_t i = 0; i < parameterCount; i++) {
        *(uint32_t *)(runCall->esp + 0x8 + 4 * i) = va_arg(valist, uint32_t);
    }
    listAdd(&callsToProcess, runCall);
}
