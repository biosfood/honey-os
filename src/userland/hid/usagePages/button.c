#include <hid.h>

// https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf
// section 12, page 67

REQUEST(updateButton, "mouse", "updateButton");

void handleButton1(int32_t data) { updateButton(1, data); }
void handleButton2(int32_t data) { updateButton(2, data); }
void handleButton3(int32_t data) { updateButton(3, data); }
void handleButton4(int32_t data) { updateButton(4, data); }
void handleButton5(int32_t data) { updateButton(5, data); }

Usage buttonUsages[] = {
    {
        .id = 1,
        .handle = handleButton1,
        .name = "Buton 1",
    }, {
        .id = 2,
        .handle = handleButton2,
        .name = "Buton 2",
    }, {
        .id = 3,
        .handle = handleButton3,
        .name = "Buton 3",
    }, {
        .id = 4,
        .handle = handleButton4,
        .name = "Buton 4",
    }, {
        .id = 5,
        .handle = handleButton5,
        .name = "Buton 5",
    }, {
        .id = -1, // end tag
    }
};
