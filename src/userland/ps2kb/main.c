#include <hlib.h>
#include <keycodes.h>
#include <stdint.h>

const char modifierScancodes[] = {0x2A, 0x36, 0x1D, 0x9D};

uint32_t keycodes[128] = { 
    0,   27,   KEY_1,  KEY_2, KEY_3,  KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0, KEY_MINUS,
    KEY_EQUALS, KEY_DELETE, KEY_TAB, KEY_Q, KEY_W,  KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I, KEY_O, KEY_P,
    KEY_BRACEOPEN, KEY_BRACECLOSE,  KEY_RETURN, 0,  KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L,
    KEY_SEMICOLON, KEY_APOSTROPHE, /*backtick*/0,  0, KEY_BACKSLASH, KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M, /*comma*/ 0,
    KEY_DOT, KEY_SLASH,  0, /* asterisk */0, 0, KEY_SPACEBAR, 0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   KEY_MINUS, 0,   0,   0,
    /*KEY_PLUS*/0, 0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0};

const char *altKeycodes[128] = {
    0,      0, 0, 0, 0, 0, 0, 0,      0,      0, 0, 0,      0, 0,      0, 0,
    0,      0, 0, 0, 0, 0, 0, 0,      0,      0, 0, 0,      0, 0,      0, 0,
    0,      0, 0, 0, 0, 0, 0, 0,      0,      0, 0, 0,      0, 0,      0, 0,
    0,      0, 0, 0, 0, 0, 0, 0,      0,      0, 0, 0,      0, 0,      0, 0,
    0,      0, 0, 0, 0, 0, 0, "\e[H", "\e[A", 0, 0, "\e[D", 0, "\e[C", 0, 0,
    "\e[B", 0, 0, 0, 0, 0, 0, 0,      0,      0, 0, 0,      0, 0,      0, 0,
    0,      0, 0, 0, 0, 0, 0, 0,      0,      0, 0, 0,      0, 0,      0, 0,
    0,      0, 0, 0, 0, 0, 0, 0,      0,      0, 0, 0,      0, 0,      0, 0};

REQUEST(keyDown, "keyboard", "keyDown");
REQUEST(keyUp, "keyboard", "keyUp");
REQUEST(read, "ps2", "read");

void onKey() {
    uint8_t scancode = read(0, 0);
    if (scancode == 0xE0) {
        scancode = read(0, 0);
        if (scancode & 0x80) {
            return;
        }
        // TODO: send alternate keycodes
        return;
    }
    if (!keycodes[scancode & 0x7F]) {
        return;
    }
    if (scancode & 0x80) {
        scancode = scancode & 0x7F;
        keyUp(keycodes[scancode], 0);
        return;
    } else {
        keyDown(keycodes[scancode], 0);
        keyUp(keycodes[scancode], 0);
    }
}

void doRegister(uint32_t deviceType, uint32_t event) {
    subscribeEvent(getService("pic"), event, onKey);
}

int32_t main() {
    createFunction("register", (void *)doRegister);
}