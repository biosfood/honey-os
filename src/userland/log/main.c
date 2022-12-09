#include <hlib.h>
#include <stdint.h>

uint32_t services[10], functions[10], outputCount;

void writeChar(uint8_t data) {
    for (uint8_t i = 0; i < outputCount; i++) {
        request(services[i], functions[i], data, 0);
    }
}

void writeString(char *string) {
    for (uint32_t i = 0; string[i]; i++) {
        writeChar(string[i]);
    }
}

void handleLog(uint32_t stringId, uint32_t unused, uint32_t caller) {
    char buffer[100];
    writeChar('[');
    writeChar(' ');
    readString(caller, buffer);
    writeString(buffer);
    writeChar(' ');
    writeChar(']');
    writeChar(' ');
    readString(stringId, buffer);
    writeString(buffer);
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
