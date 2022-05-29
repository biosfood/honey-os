#include <memory.h>
#include <multiboot.h>
#include <stdint.h>

void kernelMain(void *multibootInfo) {
    setupMemory();
    void *address = kernelMapPhysical(multibootInfo);
    uint32_t tarSize = 0;
    void *initrd = findInitrd(address, &tarSize);
    void *loaderProgram = findTarFile(initrd, tarSize, "initrd/loader");
    asm("mov %%eax, %0" ::"r"(loaderProgram));
    while (1)
        ;
}
