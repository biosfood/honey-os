#include <hlib.h>
#include <stdint.h>

uint32_t outService, outProvider;

void writeParallel(uint8_t data) {
    request(outService, outProvider, PTR(data), 0);
}

void handleLog(void *data, uint32_t dataLength) {
    char *string = data, dump;
    for (uint32_t i = 0; i < dataLength; i++) {
        writeParallel(string[i]);
    }
    writeParallel('\r');
    writeParallel('\n');
}

void registerOut(uint32_t *service, uint32_t provider) {
    outService = *service;
    outProvider = provider;
}

int32_t main() {
    installServiceProvider("log", (void *)handleLog);
    installServiceProvider("registerOut", (void *)registerOut);
    return 0;
}
