#include <memory.h>
#include <multiboot.h>
#include <service.h>
#include <stdint.h>
#include <syscall.h>

void kernelMain(void *multibootInfo) {
    setupMemory();
    void *address = kernelMapMultiplePhysicalPages(multibootInfo, 4);
    uint32_t tarSize = 0;
    void *initrd = findInitrd(address, &tarSize);
    setupSyscalls();
    void *loaderProgram = findTarFile(initrd, tarSize, "initrd/loader");
    loadElf(loaderProgram);
    asm("mov %%eax, %0" ::"r"(0xB105F00D));
    while (1)
        ;
}
