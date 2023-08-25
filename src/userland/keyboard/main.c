#define ALLOC_MAIN

#include <hlib.h>

// maximum number of simultaniously pressed keys
#define MAX_PRESSED 10

REQUEST(doKeyCallback, "ioManager", "keyCallback");

// will be incremented every time a key is pressed.
volatile uint32_t repeatingThreadId = 0;
char repeatingKey = 0;

char pressedKeys[] = { 0 };

void keyUp(char keycode) {
    // 'normal' keyboards additionaly restart the key repeat for the key that was pressed before a key was let go of
    if (keycode == repeatingKey) {
        // only cancel current repeating key if it was the key that was released
        repeatingThreadId++;
    }
}

void sendPress(uint32_t keycode) {
    doKeyCallback(keycode, 0);
}

void keyRepeat(uint32_t keycode, uint32_t threadId) {
    sleep(500);
    while (threadId == repeatingThreadId) {
        sendPress(keycode);
        sleep(50);
    }
}

void keyDown(uint32_t keycode) {
    sendPress(keycode);
    repeatingKey = keycode;
    fork((void *)keyRepeat, PTR(keycode), PTR(++repeatingThreadId), 0);
}

void initialize() {
    createFunction("keyDown", (void *)keyDown);
    createFunction("keyUp", (void *)keyUp);
}

int32_t main() {
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        initialize();
        printf("keyboard driver set up\n");
    }
}
