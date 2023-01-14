#define ALLOC_MAIN

#include <hlib.h>
#include <stdint.h>

bool lock = false;
char buffer[100];

uint32_t mainService, mainOut, globalService, globalOut;

uint32_t focusService;
uint32_t currentKeyConsumerService, currentKeyConsumerFunction;

void writeString(char *string) {
    for (uint32_t i = 0; string[i]; i++) {
        request(globalService, globalOut, string[i], 0);
    }
}

void handleLog(uint32_t stringId, uint32_t unused, uint32_t caller,
               uint32_t callerId) {
    while (lock) {
        syscall(-1, 0, 0, 0, 0);
    }
    lock = true;
    if (callerId == focusService) {
        readString(stringId, buffer);
        for (uint32_t i = 0; buffer[i]; i++) {
            request(mainService, mainOut, buffer[i], 0);
        }
    }
    writeString("[ ");
    readString(caller, buffer);
    writeString(buffer);
    for (int32_t i = 10 - strlen(buffer); i > 0; i--) {
        request(globalService, globalOut, ' ', 0);
    }
    writeString(" ] ");
    readString(stringId, buffer);
    writeString(buffer);
    writeString("\r\n");
    lock = false;
}

extern uint32_t logService, logFunction;

int32_t main() {
    logService = getServiceId();
    logFunction = createFunction("", (void *)handleLog);
    mainService = loadFromInitrd("vga");
    mainOut = getFunction(mainService, "writeChar");
    globalService = loadFromInitrd("parallel");
    globalOut = getFunction(mainService, "writeChar");
    loadFromInitrd("log");
    loadFromInitrd("pic");
    loadFromInitrd("keyboard");
}
