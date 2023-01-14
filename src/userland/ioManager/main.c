#define ALLOC_MAIN

#include <hlib.h>
#include <stdint.h>

bool lock = false;
char buffer[100];

uint32_t mainService, mainOut, globalService, globalOut;

uint32_t focusService, focusServiceKeyHandler;
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
    if (callerId == focusService) {
        lock = true;
        readString(stringId, buffer);
        for (uint32_t i = 0; buffer[i]; i++) {
            request(mainService, mainOut, buffer[i], 0);
        }
        lock = false;
        return;
    }
    lock = true;
    writeString("[ ");
    readString(caller, buffer);
    writeString(buffer);
    for (int32_t i = 10 - strlen(buffer); i > 0; i--) {
        request(globalService, globalOut, ' ', 0);
    }
    writeString(" ] ");
    readString(stringId, buffer);
    writeString(buffer);
    if (buffer[strlen(buffer) - 1] != '\n') {
        writeString("\n");
    }
    lock = false;
}

extern uint32_t ioManager, logFunction, keyCallback;

void setForeground(uint32_t service) {
    focusService = service;
    focusServiceKeyHandler = 0;
}

void handleKey(uint32_t keycode, uint32_t stringId) {
    focusServiceKeyHandler = getFunction(focusService, "onKey");
    request(focusService, focusServiceKeyHandler, keycode, stringId);
}

int32_t main() {
    ioManager = getServiceId();
    logFunction = createFunction("", (void *)handleLog);
    keyCallback = createFunction("", (void *)handleKey);
    mainService = loadFromInitrd("vga");
    mainOut = getFunction(mainService, "writeChar");
    globalService = loadFromInitrd("parallel");
    globalOut = getFunction(globalService, "writeChar");
    loadFromInitrd("log");
    loadFromInitrd("pic");
    loadFromInitrd("keyboard");
    createFunction("setForeground", (void *)setForeground);
}
