#include <memory.h>
#include <stdint.h>

void *multibootInfo; // set by bootloader

void kernelMain() {
    setupMemory();
    while (1)
        ;
}
