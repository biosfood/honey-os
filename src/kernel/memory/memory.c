#include <memory.h>
#include <stdint.h>

void setupMemory() {
    reservePagesUntil(0x900); // address 0x900000, until the end of where the
                              // kernel data was mapped
}
