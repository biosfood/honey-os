#include <hlib.h>

uint32_t event0, event1;

void parallelOut(void *data, uint32_t dataLength) {
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

int32_t main() {
    uint32_t provider = installServiceProvider("out", (void *)parallelOut);
    installServiceProvider("writeParallel", (void *)parallelOut);
    event0 = createEvent("in");
    event1 = createEvent("parallelIn");
    uint32_t thisService = getServiceId();
    requestName("log", "registerOut", &thisService, provider);
    return 0;
}
