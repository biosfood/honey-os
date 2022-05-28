#include <memory.h>
#include <multiboot.h>
#include <stdint.h>

void kernelMain(void *multibootInfo) {
    setupMemory();
    void *address = kernelMapPhysical(multibootInfo);
    asm("mov %%eax, %0" ::"r"(address));
    while (1)
        ;
}
