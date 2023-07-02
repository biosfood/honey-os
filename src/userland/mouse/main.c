#define ALLOC_MAIN
#include <hlib.h>

volatile int32_t x = 0, y = 0;
volatile bool initialized = false;
volatile uint32_t updateEvent, buttons;

REQUEST(checkFocus, "ioManager", "checkFocus");

void moveAbsolute(int32_t newX, int32_t newY) {
    x = newX;
    y = newY;
    fireEvent(updateEvent, 0);
}

void moveRelative(int32_t dX, int32_t dY) {
    x += dX;
    y += dY;
    fireEvent(updateEvent, 0);
}

void updateButtons(uint8_t newButtons) {
    buttons = newButtons;
    fireEvent(updateEvent, 0);
}

void initialize() {
    initialized = true;
    createFunction("moveAbsolute", (void *)moveAbsolute);
    createFunction("moveRelative", (void *)moveRelative);
    createFunction("updateButtons", (void *)updateButtons);
    updateEvent = createEvent("update");
}

int32_t main() {
    if (!initialized) { initialize(); }
    if (!checkFocus(0, 0)) {
        return;
    }
    uint32_t serviceId = getServiceId();
    printf("service: %i, event: %i\n", serviceId, updateEvent);
    while (1) {
        await(serviceId, updateEvent);
        printf("mouse info: buttons: %i, x: %i, y: %i         \r", buttons, x, y);
    }
}
