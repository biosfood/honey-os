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

char pressedKeys[] = { 0 };

void modifierUp(uint8_t modifier) {

}

void modifierDown(uint8_t modifier) {
    
}

void keyUp(char keycode) {
}

void keyDown(char keycode) {
    doKeyCallback(keycode, 0);
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
