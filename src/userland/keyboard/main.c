#include <hlib.h>
#include <stdint.h>

void onKey(void *data, uint32_t dataSize) {
    uint32_t keyCode = ioIn(0x60, 1);
    log("key press detected");
}

int32_t main() {
    uint32_t service = getService("pic");
    uint32_t event = getEvent(service, "irq1");
    subscribeEvent(service, event, onKey);
    log("keyboard handler installed");
}
