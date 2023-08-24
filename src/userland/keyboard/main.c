#define ALLOC_MAIN

#include <hlib.h>

// maximum number of simultaniously pressed keys
#define MAX_PRESSED 10

REQUEST(doKeyCallback, "ioManager", "keyCallback");

enum {
    MODIFIER_LEFT_SHIFT = 0,
    MODIFIER_RIGHT_SHIFT = 1,
    MODIFIER_LEFT_CONTROL = 2,
    MODIFIER_RIGHT_CONTROL = 3,
};

uint32_t modifiers = 0;

// will be incremented every time a key is pressed.
volatile uint32_t repeatingThreadId = 0;
char repeatingKey = 0;

char pressedKeys[] = { 0 };

void modifierUp(uint8_t modifier) {

}

void modifierDown(uint8_t modifier) {
    
}

void keyUp(char keycode) {
    // 'normal' keyboards additionaly restart the key repeat for the key that was pressed before a key was let go of
    if (keycode == repeatingKey) {
        // only cancel current repeating key if it was the key that was released
        repeatingThreadId++;
    }
}

void keyRepeat(char keycode, uint32_t threadId) {
    sleep(500);
    while (threadId == repeatingThreadId) {
        doKeyCallback(keycode, 0);
        sleep(50);
    }
}

void keyDown(char keycode) {
    doKeyCallback(keycode, 0);
    repeatingKey = keycode;
    fork((void *)keyRepeat, PTR(keycode), PTR(++repeatingThreadId), 0);
}

void initialize() {
    createFunction("keyDown", (void *)keyDown);
    createFunction("keyUp", (void *)keyUp);
    createFunction("modifierDown", (void *)modifierDown);
    createFunction("modifierUp", (void *)modifierUp);
}

int32_t main() {
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        initialize();
        printf("keyboard driver set up\n");
    }
}
