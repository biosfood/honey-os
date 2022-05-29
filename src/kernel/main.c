#include <memory.h>
#include <multiboot.h>
#include <stdint.h>

void kernelMain(void *multibootInfo) {
    setupMemory();
    void *address = kernelMapPhysical(multibootInfo);
    void *initrd = findInitrd(address);
    asm("mov %%eax, %0" ::"r"(initrd));
    while (1)
        ;
}
