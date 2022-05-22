#include <memory.h>

void memset(uint8_t *target, uint8_t byte, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        *target = byte;
        target++;
    }
}
