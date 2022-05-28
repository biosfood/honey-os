#include <memory.h>

void memset(void *_target, uint8_t byte, uint32_t size) {
    uint8_t *target = _target;
    for (uint32_t i = 0; i < size; i++) {
        *target = byte;
        target++;
    }
}
