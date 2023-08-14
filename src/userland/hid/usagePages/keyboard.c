#include <hid.h>

// https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf
// section 10, page 53, table 12

char keycodes[] = {
    0, 0, 0, 0, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '\n'
};

REQUEST(doKeyCallback, "ioManager", "keyCallback");

void handleKeyboard(uint32_t usage, int32_t data) {
    if (usage == 0) {
        return;
    }
    if (usage >= sizeof(keycodes) / sizeof(char)) {
        // doKeyCallback('E', 0);
    } else {
        doKeyCallback(keycodes[usage], 0);
    }
}
