#define ALLOC_MAIN
#include "memory/malloc.h"

#include <interrupts.h>
#include <memory.h>
#include <multiboot.h>
#include <service.h>
#include <stdint.h>
#include <syscall.h>
#include <util.h>

extern ListElement *callsToProcess;

void *initrd;
uint32_t initrdSize;
Service *hlib;

Service *readInitrdProgram(char *name) {
    char *fileName = combineStrings("initrd/", name);
    void *elfData = findTarFile(initrd, initrdSize, fileName);
    free(fileName);
    return loadElf(elfData, name);
}

Service *loadProgram(char *name, Syscall *respondingTo) {
    Service *service = readInitrdProgram(name);
    Provider *provider = findProvider(service, "main");
    scheduleProvider(provider, 0, 0, respondingTo);
    return service;
}

void loadAndScheduleSystemServices(void *multibootInfo) {
    void *address = kernelMapPhysicalCount(multibootInfo, 4);
    initrd = findInitrd(address, &initrdSize);
    hlib = readInitrdProgram("hlib");
    loadProgram("loader", NULL);
}

void kernelMain(void *multibootInfo) {
    setupMemory();
    loadAndScheduleSystemServices(multibootInfo);
    setupSyscalls();
    registerInterrupts();
    while (1) {
        Syscall *call = listPopFirst(&callsToProcess);
        if (!call) {
            asm("sti;hlt");
            continue;
        }
        processSyscall(call);
    }
}
