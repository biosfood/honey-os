#define ALLOC_MAIN
#include <hlib.h>
#include <stdint.h>

enum {
    MODIFIER_LEFT_SHIFT = 0x01 << 0,
    MODIFIER_RIGHT_SHIFT = 0x01 << 1,
    MODIFIER_LEFT_CONTROL = 0x01 << 2,
    MODIFIER_RIGHT_CONTROL = 0x01 << 3
};

const char modifierScancodes[] = {0x2A, 0x36, 0x1D, 0x9D};

unsigned char keycodes[128] = {
    0,   27,   '1',  '2', '3',  '4', '5', '6', '7', '8', '9', '0', '-',
    '=', '\b', '\t', 'q', 'w',  'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    '[', ']',  '\n', 0,   'a',  's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
    ';', '\'', '`',  0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',',
    '.', '/',  0,    '*', 0,    ' ', 0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   '-', 0,   0,   0,
    '+', 0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0};

const char *altKeycodes[128] = {
    0,      0, 0, 0, 0, 0, 0, 0,      0,      0, 0, 0,      0, 0,      0, 0,
    0,      0, 0, 0, 0, 0, 0, 0,      0,      0, 0, 0,      0, 0,      0, 0,
    0,      0, 0, 0, 0, 0, 0, 0,      0,      0, 0, 0,      0, 0,      0, 0,
    0,      0, 0, 0, 0, 0, 0, 0,      0,      0, 0, 0,      0, 0,      0, 0,
    0,      0, 0, 0, 0, 0, 0, "\e[H", "\e[A", 0, 0, "\e[D", 0, "\e[C", 0, 0,
    "\e[B", 0, 0, 0, 0, 0, 0, 0,      0,      0, 0, 0,      0, 0,      0, 0,
    0,      0, 0, 0, 0, 0, 0, 0,      0,      0, 0, 0,      0, 0,      0, 0,
    0,      0, 0, 0, 0, 0, 0, 0,      0,      0, 0, 0,      0, 0,      0, 0};

const char capitalKeycodes[128] = {
    0,   27,   '!',  '@', '#', '$', '%', '^', '&', '*', '(', ')', '_',
    '+', '\b', '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
    '{', '}',  '\n', 0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
    ':', '\"', '~',  0,   '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<',
    '>', '?',  0,    '*', 0,   ' ', 0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   '-', 0,   0,   0,
    '+', 0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0};

uint8_t modifiers = 0;

uint8_t getScancode() {
    int_fast16_t scancode = -1;
    for (uint16_t i = 0; i < 1000; i++) {
        if ((ioIn(0x64, 1) & 1) == 0) {
            continue;
        }
        scancode = ioIn(0x60, 1);
        break;
    }
    return scancode;
}

void onKey() {
    uint8_t scancode = getScancode();
    if (scancode == 0xE0) {
        scancode = getScancode();
        if (scancode & 0x80) {
            return;
        }
        printf("key press %s detected\n", altKeycodes[scancode]);
        return;
    }
    if (scancode & 0x80) {
        scancode = scancode & 0x7F;
        for (uint8_t i = 0; i < sizeof(modifierScancodes); i++) {
            if (scancode == modifierScancodes[i]) {
                modifiers = modifiers & (0xFF ^ 0x01 << i);
            }
        }
        return;
    }
    for (uint8_t i = 0; i < sizeof(modifierScancodes); i++) {
        if (scancode == modifierScancodes[i]) {
            modifiers = modifiers | 0x01 << i;
            return;
        }
    }
    char data = 0;
    if (modifiers & MODIFIER_LEFT_SHIFT || modifiers & MODIFIER_RIGHT_SHIFT) {
        data = capitalKeycodes[scancode];
    } else {
        data = keycodes[scancode];
    }
    printf("key press %c detected\n", data);
}

int32_t main() {
    uint32_t service = getService("pic");
    uint32_t event = getEvent(service, "irq1");
    subscribeEvent(service, event, onKey);
    printf("keyboard handler installed\n");
}
