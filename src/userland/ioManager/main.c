#include <hlib.h>
#include <stdint.h>

uint32_t mainService, mainOut, globalService, globalOut, globalBulkOut;

uint32_t focusService, focusServiceKeyHandler;
uint32_t currentKeyConsumerService, currentKeyConsumerFunction;

void writeString(char *string) {
    if (globalBulkOut) {
        uint32_t stringId = insertString(string);
        request(globalService, globalBulkOut, stringId, 0);
        discardString(stringId);
        return;
    }
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

bool doCheckFocus() {
    return stackContains(focusService);
}

void handleLog(uint32_t stringId, uint32_t unused, uint32_t caller,
               uint32_t callerId) {
    free(malloc(1));
    if (stackContains(focusService)) {
        logMain(stringId);
        return;
    }

    char *buffer = malloc(getStringLength(stringId));
    readString(stringId, buffer);
    char *result = asprintf("[            ] %s ", buffer);
    uint32_t resultLength = strlen(result);
    if (result[resultLength - 2] != '\n') {
        result[resultLength - 1] = '\n';
    } else {
        result[resultLength - 1] = 0;
    }
    uint32_t callerNameLength = getStringLength(caller);
    char *callerName = malloc(callerNameLength + 2);
    readString(caller, callerName);
    for (uint32_t i = 0; i < callerNameLength; i++) {
        result[i + 2] = callerName[i];
    }
    
    static bool lock = false;
    while (lock) {
        syscall(-1, 0, 0, 0, 0);
    }
    lock = true;

    writeString(result);
    
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
    createFunction("keyCallback", (void *)handleKey);
    createFunction("checkFocus", (void *)doCheckFocus);
    mainService = loadFromInitrd("vga");
    mainOut = getFunction(mainService, "writeChar");
    globalService = loadFromInitrd("parallel");
    globalOut = getFunction(globalService, "writeChar");
    globalBulkOut = getFunction(globalService, "write_bulk");
    loadFromInitrd("log");
    createFunction("setForeground", (void *)setForeground);
    keyEvent = createEvent("keyPress");
    newLineEvent = createEvent("newLine");
    createFunction("gets", (void *)getsImplementation);
}
