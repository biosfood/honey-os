#include "hid.h"

// https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf
// page 14, section 3, table 1: Usage Page Summary
UsagePage usagePages[] = {
    {
        .name = "Undefined",
    }, {
        .name = "Generic Desktop Controls",
    }, {
        .name = "Simulation Controls",
    }, {
        .name = "VR Controls",
    }, {
        .name = "Sport Controls",
    }, {
        .name = "Game Controls",
    }, {
        .name = "Generic Device Controls",
    }, {
        .name = "Keyboard/Keypad",
    }, {
        .name = "LEDs",
    }, {
        .name = "Button",
    }, {
        .name = "Ordinal",
    }, {
        .name = "Telephony",
    }, {
        .name = "Consumer",
    }, {
        .name = "Digitizer",
    }, {
        .name = "PID Page",
    },
};

UsagePage *getUsagePage(uint32_t id) {
    if (id >= sizeof(usagePages) / sizeof(usagePages[0])) {
        return &usagePages[0];
    }
    return &usagePages[id];
}
