#include <hlib.h>
#include <stdint.h>

typedef union {
    uint8_t byte;
    struct {
        uint8_t outputBufferStatus:1;
        uint8_t inputBufferStatus: 1;
        uint8_t sytemFlag: 1;
        uint8_t commandData: 1;
        uint8_t unused: 2;
        uint8_t timeoutError: 1;
        uint8_t parityError: 1;
    } __attribute__((packed)) data;
    } Status;

Status readStatus() {
    Status result = {.byte = ioIn(0x64, 1)};
    return result;
}

void waitForRead() {
    uint32_t timeout = 100000;
    while (!readStatus().data.outputBufferStatus) {
        if (--timeout == 0) {
            printf("PS/2 read timeout\n");
            return;
        }
    }
}

uint8_t read(uint8_t device) {
    waitForRead();
    return ioIn(0x60, 1);
}

int32_t main() {
    createFunction("read", (void *)read);
    loadFromInitrd("ps2kb");
    return 0;
}
