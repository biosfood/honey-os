#include <hid.h>

// https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf
// section 12, page 67

REQUEST(updateButton, "mouse", "updateButton");

void handleButton(uint32_t usage, int32_t data) {
    updateButton(usage, data);
}
