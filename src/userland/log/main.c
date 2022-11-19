#include <hlib.h>
#include <stdint.h>

uint32_t outService, outProvider;

void writeParallel(uint8_t data) { request(outService, outProvider, data, 0); }

char buffer[100];

void handleLog(uint32_t stringId, uint32_t unused) {
    readString(stringId, buffer);
    uint32_t length = strlen(buffer);
    for (uint32_t i = 0; i < length; i++) {
        writeParallel(buffer[i]);
    }
    writeParallel('\r');
    writeParallel('\n');
}

void registerOut(uintptr_t service, uintptr_t provider) {
    outService = service;
    outProvider = provider;
}

int32_t main() {
    installServiceProvider("log", (void *)handleLog);
    installServiceProvider("registerOut", (void *)registerOut);
    return 0;
}
