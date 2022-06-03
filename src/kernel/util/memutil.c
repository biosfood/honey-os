#include <util.h>

void memcpy(void *source, void *destination, uint32_t size) {
    uint8_t *src = source, *dest = destination;
    for (uint32_t i = 0; i < size; i++) {
        dest[i] = src[i];
    }
}
