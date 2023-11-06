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

void writeBulk(uint32_t stringId) {
    uint32_t length = getStringLength(stringId);
    char *buffer = malloc(length);
    readString(stringId, buffer);
    for (uint32_t i = 0; i < length; i++) {
        parallelOut(buffer[i], 0);
    }
}

int32_t parallelIn(void *data, uint32_t dataLength) {
    // todo
    return 0;
}

int32_t main() {
    createFunction("writeChar", (void *)parallelOut);
    createFunction("write_bulk", (void *)writeBulk);
}
