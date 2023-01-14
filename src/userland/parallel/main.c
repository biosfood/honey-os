#include <hlib.h>

uint32_t event0, event1;

void parallelOut(uint32_t data, uint32_t dataLength) {
    if (data == '\n') {
        parallelOut('\r', 0);
    }
    uint8_t control;
    while (!(ioIn(0x379, sizeof(uint8_t)) & 0x80)) {
    }
    ioOut(0x378, U32(data), sizeof(uint8_t));

    control = ioIn(0x37A, sizeof(uint8_t));
    ioOut(0x37A, control | 1, sizeof(uint8_t));
    ioOut(0x37A, control, sizeof(uint8_t));
    while (!(ioIn(0x379, sizeof(uint8_t)) & 0x80)) {
    }
}

int32_t parallelIn(void *data, uint32_t dataLength) {
    // todo
    return 0;
}

int32_t main() { createFunction("writeChar", (void *)parallelOut); }
