#define ALLOC_MAIN

#include <hlib.h>
#include <stdint.h>

uint32_t mainService, mainOut, globalService, globalOut;

uint32_t focusService, focusServiceKeyHandler;
uint32_t currentKeyConsumerService, currentKeyConsumerFunction;

void writeString(char *string) {
    for (uint32_t i = 0; string[i]; i++) {
        request(globalService, globalOut, string[i], 0);
    }
}

void logMain(uint32_t stringId) {
    static char buffer[100];
    static bool lock = false;
    while (lock) {
        syscall(-1, 0, 0, 0, 0);
    }
    lock = true;
    readString(stringId, buffer);
    for (uint32_t i = 0; buffer[i]; i++) {
        request(mainService, mainOut, buffer[i], 0);
    }
    lock = false;
}

void handleLog(uint32_t stringId, uint32_t unused, uint32_t caller,
               uint32_t callerId) {
    if (stackContains(focusService)) {
        logMain(stringId);
        return;
    }
    static bool lock = false;
    static char buffer[100];
    while (lock) {
        syscall(-1, 0, 0, 0, 0);
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

uint32_t keyEvent, newLineEvent;

char inputBuffer[256];
uint32_t inputBufferPosition;
bool printInput = true;

void onNewLine() {
    uint32_t stringId = insertString(inputBuffer);
    fireEvent(newLineEvent, stringId);
    inputBufferPosition = 0;
    inputBuffer[inputBufferPosition] = '\0';
}

void handleKey(uint32_t keycode, uint32_t stringId) {
    if (!focusServiceKeyHandler) {
        focusServiceKeyHandler = getFunction(focusService, "onKey");
    }
    if (focusServiceKeyHandler) {
        request(focusService, focusServiceKeyHandler, keycode, stringId);
    }
    fireEvent(keyEvent, keycode);
    if (!printInput) {
        return;
    }
    switch (keycode) {
    case '\b':
        if (!inputBufferPosition) {
            return;
        }
        inputBufferPosition--;
        inputBuffer[inputBufferPosition] = 0;
        break;
    case '\n':
        onNewLine();
        break;
    default:
        inputBuffer[inputBufferPosition] = (char)keycode;
        inputBufferPosition++;
        inputBuffer[inputBufferPosition] = '\0';
        break;
    }
    request(mainService, mainOut, keycode, 0);
}

uint32_t getsImplementation() {
    printInput = true;
    inputBufferPosition = 0;
    inputBuffer[inputBufferPosition] = '\0';
    uint32_t result = await(ioManager, newLineEvent);
    printInput = false;
    return result;
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
    keyEvent = createEvent("keyPress");
    newLineEvent = createEvent("newLine");
    createFunction("gets", (void *)getsImplementation);
}
