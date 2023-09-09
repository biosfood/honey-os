#include <util.h>
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
    if (elfData) {
        return loadElf(elfData, name);
    }
    return NULL;
}

Service *loadProgram(char *name, Syscall *respondingTo, bool initialize) {
    Service *service = readInitrdProgram(name);
    if (initialize) {
        ServiceFunction *provider = findFunction(service, "main");
        scheduleFunction(provider, respondingTo);
    }
    return service;
}

void loadAndScheduleSystemServices(void *multibootInfo) {
    installKernelEvents();
    void *address = kernelMapPhysicalCount(multibootInfo, 4);
    initrd = findInitrd(address, &initrdSize);
    hlib = readInitrdProgram("hlib");
    loadProgram("loader", NULL, true);
}

void kernelMain(void *multibootInfo) {
    reservePagesUntilPhysical(0x900);
    loadAndScheduleSystemServices(multibootInfo);
    setupSyscalls();
    registerInterrupts();
    while (1) {
        Syscall *call = listPopFirst(&callsToProcess);
        if (!call) {
            asm("sti;hlt;cli");
            continue;
        }
        processSyscall(call);
    }
}
