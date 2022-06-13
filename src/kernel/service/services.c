#include "elf.h"
#include "service.h"
#include <memory.h>
#include <service.h>
#include <syscalls.h>
#include <util.h>

extern void *kernelCodePageTable;

void *serviceCR3 = 0;
void *serviceESP = 0;
void *mainFunction = NULL;

extern void *functionsStart;
extern void(runFunction)();

Service *currentService;
ListElement *services, *callsToProcess;
Syscall *currentSyscall;

void resume(Syscall *syscall) {
    currentSyscall = syscall;
    mainFunction = syscall->address;
    serviceESP = syscall->esp;
    currentService = syscall->service;
    serviceCR3 = syscall->cr3;
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
    // todo: make this unwritable!
    // todo: use functionsStart as the reference
    sharePage(&(service->pagingInfo), PTR(0xFFC02000),
              PTR(0xFFC02000)); // functionsStart, functionsStart);
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
