#include "elf.h"
#include "service.h"
#include <memory.h>
#include <service.h>
#include <syscalls.h>
#include <util.h>

extern void *functionsStart;
extern void *functionsEnd;
extern void(runFunction)();

ListElement *services, *callsToProcess;
Syscall *currentSyscall;

void resume(Syscall *syscall) {
    if (U32(syscall) < 0x1000) {
        asm("hlt" ::"a"(syscall));
    }
    currentSyscall = syscall;
    runFunction();
}

void loadElf(void *elfStart, char *serviceName) {
    // use this function ONLY to load the initrd/loader program(maybe also the
    // ELF loader service)!
    ElfHeader *header = elfStart;
    ProgramHeader *programHeader =
        elfStart + header->programHeaderTablePosition;
    Service *service = malloc(sizeof(Service));
    memset(service, 0, sizeof(Service));
    service->pagingInfo.pageDirectory = malloc(0x1000);
    service->name = serviceName;
    void *current = &functionsStart;
    for (uint32_t i = 0; i < 3; i++) {
        // todo: make this unwritable!
        sharePage(&(service->pagingInfo), current, current);
        current += 0x1000;
    }
    for (uint32_t i = 0; i < header->programHeaderEntryCount; i++) {
        for (uint32_t page = 0; page < programHeader->segmentMemorySize;
             page += 0x1000) {
            void *data = malloc(0x1000);
            memset(data, 0, 0x1000);
            memcpy(elfStart + programHeader->dataOffset, data,
                   MIN(0x1000, programHeader->segmentFileSize - page));
            sharePage(&service->pagingInfo, data,
                      PTR(programHeader->virtualAddress + page));
        }
        programHeader = (void *)programHeader + header->programHeaderEntrySize;
    }
    Provider *main = malloc(sizeof(Provider));
    main->name = "main";
    main->service = service;
    main->address = PTR(header->entryPosition);
    listAdd(&services, service);
    listAdd(&service->providers, main);
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

Provider *findProvider(Service *service, char *name) {
    foreach (service->providers, Provider *, provider, {
        if (stringEquals(provider->name, name)) {
            return provider;
        }
    })
        ;
    return NULL;
}

void scheduleProvider(Provider *provider, void *data, uint32_t dataLength,
                      Syscall *respondingTo) {
    sharePage(&provider->service->pagingInfo, data, data);
    Syscall *runCall = malloc(sizeof(Syscall));
    runCall->function = SYS_RUN;
    runCall->esp = malloc(0x1000);
    runCall->respondingTo = respondingTo;
    runCall->cr3 =
        getPhysicalAddressKernel(provider->service->pagingInfo.pageDirectory);
    runCall->service = provider->service;
    runCall->resume = true;
    sharePage(&provider->service->pagingInfo, runCall->esp, runCall->esp);
    runCall->esp += 0xFF0;
    *(void **)runCall->esp = provider->address;
    *(void **)(runCall->esp + 0x4) = &runEnd;
    *(void **)(runCall->esp + 0x8) = data;
    *(uint32_t *)(runCall->esp + 0xC) = dataLength;
    listAdd(&callsToProcess, runCall);
}
