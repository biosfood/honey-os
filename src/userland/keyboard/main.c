#define ALLOC_MAIN
#include <hlib.h>
#include <keycodes.h>

#define KEY_STRUCT(_name, _id, _normal, _modified, _modifiers) \
    { .key = _name, .normal = _normal, .modified = _modified, .modifierKeys = _modifiers },

uint32_t MODIFIERS_SHIFT[] = { 225, 229, 0 };

KeyInfo keyInfos[] = { KEYS(KEY_STRUCT) };

// maximum number of simultaniously pressed keys
#define MAX_PRESSED 10

REQUEST(doKeyCallback, "ioManager", "keyCallback");

// will be incremented every time a key is pressed.
volatile uint32_t repeatingThreadId = 0;
uint32_t repeatingKey = 0;

uint32_t pressedKeys[] = { 0 };

void keyUp(uint32_t keycode) {
    // 'normal' keyboards additionaly restart the key repeat for the key that was pressed before a key was let go of
    if (keycode == repeatingKey) {
        // only cancel current repeating key if it was the key that was released
        repeatingThreadId++;
    }
    for (uint32_t i = 0; i < MAX_PRESSED; i++) {
        if (pressedKeys[i] == keycode) {
            pressedKeys[i] = 0;
            break;
        }
    }
}

void sendPress(uint32_t keycode) {
    if (keycode >= sizeof(keyInfos) / sizeof(KeyInfo)) {
        return;
    }
    KeyInfo *info = &keyInfos[keycode];
    char *data = info->normal;
    for (uint32_t i = 0; i < MAX_PRESSED; i++) {
        if (!pressedKeys[i]) {
            continue;
        }
        for (uint32_t j = 0; info->modifierKeys[j]; j++) {
            if (pressedKeys[i] == info->modifierKeys[j]) {
                data = info->modified;
                break;
            }
        }
        if (data == info->modified) {
            break;
        }
    }
    for (uint32_t i = 0; data[i]; i++) {
        doKeyCallback(data[i], 0);
    }
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
    for (uint32_t i = 0; i < MAX_PRESSED; i++) {
        if (!pressedKeys[i]) {
            pressedKeys[i] = keycode;
            break;
        }
    }
    if (keycode >= sizeof(keyInfos) / sizeof(KeyInfo)) {
        return;
    }
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
