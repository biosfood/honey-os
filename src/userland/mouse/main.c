#include <hlib.h>

volatile int32_t x = 0, y = 0;
volatile uint32_t updateEvent;
volatile bool buttons[5];

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

void updateButton(uint8_t button, bool newState) {
    if (button > 5 || button == 0) {
        // hid button 0 means no button is pressed
        // TODO: set all buttons to not pressed when button = 0?
        return;
    }
    buttons[button-1] = newState;
    fireEvent(updateEvent, 0);
}


bool initialized = false, updated = true;

void onUpdate() { updated = true; }

void initialize() {
    initialized = true;
    createFunction("moveAbsolute", (void *)moveAbsolute);
    createFunction("moveRelative", (void *)moveRelative);
    createFunction("updateButton", (void *)updateButton);
    updateEvent = createEvent("update");
}

int32_t main() {
    if (!initialized) { initialize(); }
    if (!checkFocus(0, 0)) {
        return 0;
    }
    uint32_t serviceId = getServiceId();
    printf("service: %i, event: %i\n", serviceId, updateEvent);
    subscribeEvent(serviceId, updateEvent, (void *)onUpdate);
    while (1) {
        if (updated) {
            printf("mouse info: x: %i, y: %i, buttons: %i %i %i         \r", x, y, buttons[0], buttons[1], buttons[2]);
            updated = false;
        }
        sleep(200);
    }
}
