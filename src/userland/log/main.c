#include <hlib.h>
#include <stdint.h>

void writeParallel(uint8_t data) {
    uint8_t control;
    while (!(ioIn(0x379, sizeof(uint8_t)) & 0x80)) {
    }
    ioOut(0x378, data, sizeof(uint8_t));

    control = ioIn(0x37A, sizeof(uint8_t));
    ioOut(0x37A, control | 1, sizeof(uint8_t));
    ioOut(0x37A, control, sizeof(uint8_t));
    while (!(ioIn(0x379, sizeof(uint8_t)) & 0x80)) {
    }
}

void handleLog(void *data, uint32_t dataLength) {
    char *string = data, dump;
    for (uint32_t i = 0; i < dataLength; i++) {
        writeParallel(string[i]);
    }
    writeParallel('\r');
    writeParallel('\n');
}

int32_t main() {
    installServiceProvider("log", handleLog);
    char *message = "logger initialized";
    handleLog(message, strlen(message));
    return 0;
}
