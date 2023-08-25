#include <hid.h>

// https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf
// section 10, page 53, table 12

REQUEST(keyDown, "keyboard", "keyDown");
REQUEST(keyUp, "keyboard", "keyUp");

void handleKeyboard(uint32_t usage, int32_t data) {
    if (data) {
        keyDown(usage, 0);
    } else {
        keyUp(usage, 0);
    }
}
