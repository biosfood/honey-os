#include <memory.h>
#include <multiboot.h>
#include <service.h>
#include <stdint.h>

void kernelMain(void *multibootInfo) {
    setupMemory();
    void *address = kernelMapMultiplePhysicalPages(multibootInfo, 4);
    uint32_t tarSize = 0;
    void *initrd = findInitrd(address, &tarSize);
    void *loaderProgram = findTarFile(initrd, tarSize, "initrd/loader");
    loadElf(loaderProgram);
    asm("mov %%eax, %0" ::"r"(loaderProgram));
    while (1)
        ;
}
