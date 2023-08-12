#include <hid.h>

extern Usage genericDesktopControlsUsages[];
extern Usage buttonUsages[];

// https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf
// page 14, section 3, table 1: Usage Page Summary
UsagePage usagePages[] = {
    {
        .name = "Undefined",
        .usages = NULL,
    }, {
        .name = "Generic Desktop Controls",
        .usages = genericDesktopControlsUsages,
    }, {
        .name = "Simulation Controls",
        .usages = NULL,
    }, {
        .name = "VR Controls",
        .usages = NULL,
    }, {
        .name = "Sport Controls",
        .usages = NULL,
    }, {
        .name = "Game Controls",
        .usages = NULL,
    }, {
        .name = "Generic Device Controls",
        .usages = NULL,
    }, {
        .name = "Keyboard/Keypad",
        .usages = NULL,
    }, {
        .name = "LEDs",
        .usages = NULL,
    }, {
        .name = "Button",
        .usages = buttonUsages,
    }, {
        .name = "Ordinal",
        .usages = NULL,
    }, {
        .name = "Telephony",
        .usages = NULL,
    }, {
        .name = "Consumer",
        .usages = NULL,
    }, {
        .name = "Digitizer",
        .usages = NULL,
    }, {
        .name = "PID Page",
        .usages = NULL,
    },
};

void initializeUsages(UsagePage *usagePage) {
    if (!usagePage->usages) {
        return;
    }
    uint32_t i = 0;
    for (; usagePage->usages[i].id != -1; i++) {
        usagePage->usages[i].usagePage = usagePage;
    }
    usagePage->usageCount = i;
}

void initializeUsagePages() {
    for (uint32_t i = 0; i < sizeof(usagePages) / sizeof(usagePages[0]); i++) {
        usagePages[i].id = i;
        initializeUsages(&usagePages[i]);
    }
}

UsagePage *getUsagePage(uint32_t id) {
    if (id >= sizeof(usagePages) / sizeof(usagePages[0])) {
        return &usagePages[0];
    }
    return &usagePages[id];
}

Usage *getUsage(UsagePage *usagePage, uint32_t id) {
    if (!usagePage || !usagePage->usages) {
        return NULL;
    }
    for (uint32_t i = 0; i < usagePage->usageCount; i++) {
        if (usagePage->usages[i].id == id) {
            return &usagePage->usages[i];
        }
    }
    return NULL;
}
