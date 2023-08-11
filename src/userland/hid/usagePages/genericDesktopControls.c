#include <hid.h>

// https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf
// section 4, table 6, page 26

REQUEST(moveRelative, "mouse", "moveRelative");
REQUEST(updateButtons, "mouse", "updateButtons");

void handleX(int32_t dx) {
    moveRelative(dx, 0);
}

void handleY(int32_t dy) {
    moveRelative(0, dy);
}

Usage genericDesktopControlsUsages[] = {
    {
        .id = 0x30,
        .handle = handleX,
        .name = "X",
    }, {
        .id = 0x31,
        .handle = handleY,
        .name = "Y",
    }, {
        .id = -1, // end tag
    }
};
