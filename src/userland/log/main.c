#include <hlib.h>
#include <stdint.h>

uint32_t services[10], functions[10], outputCount;

void writeChar(uint8_t data) {
    for (uint8_t i = 0; i < outputCount; i++) {
        request(services[i], functions[i], data, 0);
    }
}

char buffer[100];

void handleLog(uint32_t stringId, uint32_t unused) {
    readString(stringId, buffer);
    uint32_t length = strlen(buffer);
    for (uint32_t i = 0; i < length; i++) {
        writeChar(buffer[i]);
    }
    writeChar('\r');
    writeChar('\n');
}

void registerOut(uintptr_t service, uintptr_t provider) {
    services[outputCount] = service;
    functions[outputCount] = provider;
    outputCount++;
}

int32_t main() {
    installServiceProvider("log", (void *)handleLog);
    installServiceProvider("registerOut", (void *)registerOut);
    return 0;
}
