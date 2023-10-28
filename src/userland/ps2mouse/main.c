#include <hlib.h>

REQUEST(read, "ps2", "read");
REQUEST(flush, "ps2", "flush");
REQUEST(moveRelative, "mouse", "moveRelative")
REQUEST(updateButton, "mouse", "updateButton")

uint32_t deviceTypes[16];
uint8_t deviceBuffer[16][5];
uint8_t deviceBufferPositions[16];

typedef union {
    uint8_t byte;
    struct {
        uint8_t button1: 1;
        uint8_t button2: 1;
        uint8_t button3: 1;
        uint8_t one: 1;
        uint8_t xSign: 1;
        uint8_t ySign: 1;
        uint8_t xOverflow: 1;
        uint8_t yOverflow: 1;
    } __attribute__((packed)) data;
} Status;

void onMove(uint8_t device) {
    deviceBuffer[device][deviceBufferPositions[device]] = read(0, 0);
    deviceBufferPositions[device]++;
    if (deviceBufferPositions[device] == 3) {
        deviceBufferPositions[device] = 0;
        Status status = {.byte = deviceBuffer[device][0]};
        updateButton(1, status.data.button1);
        updateButton(2, status.data.button2);
        updateButton(3, status.data.button3);

        int32_t dx = (int32_t) deviceBuffer[device][2] - (status.data.xSign << 8);
        int32_t dy = (int32_t) deviceBuffer[device][1] - (status.data.ySign << 8);
        moveRelative(dx, dy);
    }
}

void doRegister(uint32_t deviceType, uint32_t event) {
    subscribeEvent(getService("pic"), event, (void *) onMove);
    deviceTypes[event] = deviceType;
}

int32_t main() {
    createFunction("register", (void *)doRegister);
}
