#include <memory.h>
#include <multiboot.h>
#include <service.h>
#include <stdint.h>
#include <syscall.h>
#include <syscalls.h>
#include <util.h>

extern ListElement *callsToProcess;
extern void (*syscallHandlers[])(Syscall *);

void *initrd;
uint32_t initrdSize;

Syscall *loadInitrdProgram(char *name, Syscall *respondingTo) {
    char *fileName = combineStrings("initrd/", name);
    void *elfData = findTarFile(initrd, initrdSize, fileName);
    free(fileName);
    loadElf(elfData, name);

    Service *service = findService(name);
    Provider *provider = findProvider(service, "main");
    Syscall *runTask = malloc(sizeof(Syscall));
    runTask->function = SYS_RUN;
    runTask->resume = true;
    runTask->cr3 = getPhysicalAddressKernel(service->pagingInfo.pageDirectory);
    runTask->esp = malloc(0x1000);
    runTask->respondingTo = respondingTo;
    runTask->service = service;
    memset(runTask->esp, 0, 0x1000);
    runTask->esp += 0xFF8;
    *(void **)runTask->esp = provider->address;
    *(void **)(runTask->esp + 4) = &runEnd;
    sharePage(&service->pagingInfo, runTask->esp, runTask->esp);
    listAdd(&callsToProcess, runTask);
    return runTask;
}

void loadAndScheduleLoader(void *multibootInfo) {
    void *address = kernelMapPhysicalCount(multibootInfo, 4);
    initrd = findInitrd(address, &initrdSize);
    loadInitrdProgram("loader", NULL);
}

void kernelMain(void *multibootInfo) {
    setupMemory();
    // loading the loader also reserves the needed space for the
    // multiboot-loaded stuff
    loadAndScheduleLoader(multibootInfo);
    setupSyscalls();
    while (1) {
        Syscall *call = listPopFirst(&callsToProcess);
        if (!call) {
            asm("hlt");
            continue;
        }
        if (call->resume) {
            resume(call);
            free(call);
            continue;
        }
        void (*handler)(Syscall *) = syscallHandlers[call->function];
        if (handler) {
            handler(call);
        }
        call->resume = true;
        if (!call->avoidReschedule) {
            listAdd(&callsToProcess, call);
        }
    }
}
