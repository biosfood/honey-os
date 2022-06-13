#include <memory.h>
#include <multiboot.h>
#include <service.h>
#include <stdint.h>
#include <syscall.h>
#include <syscalls.h>
#include <util.h>

extern ListElement *callsToProcess;
extern void (*syscallHandlers[])(Syscall *);

void loadAndScheduleLoader(void *multibootInfo) {
    void *address = kernelMapMultiplePhysicalPages(multibootInfo, 4);
    uint32_t tarSize = 0;
    void *initrd = findInitrd(address, &tarSize);
    void *loaderProgram = findTarFile(initrd, tarSize, "initrd/loader");
    loadElf(loaderProgram, "loader");

    Service *loader = findService("loader");
    Provider *provider = findProvider(loader, "main");
    Syscall *runLoader = malloc(sizeof(Syscall));
    runLoader->function = SYS_RUN;
    runLoader->address = provider->address;
    runLoader->resume = true;
    runLoader->cr3 = getPhysicalAddressKernel(loader->pagingInfo.pageDirectory);
    runLoader->esp = malloc(0x1000);
    runLoader->respondingTo = NULL;
    runLoader->service = loader;
    memset(runLoader->esp, 0, 0x1000);
    runLoader->esp += 0xFFC;
    *(void **)runLoader->esp = &runEnd;
    sharePage(&loader->pagingInfo, runLoader->esp, runLoader->esp);
    listAdd(&callsToProcess, runLoader);
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
            continue;
        }
        void (*handler)(Syscall *) = syscallHandlers[call->function];
        if (handler) {
            handler(call);
        }
    }
}
