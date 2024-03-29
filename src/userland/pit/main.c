#include <hlib.h>

#define PIT_A 0x40
#define PIT_CONTROL 0x43

#define PIT_MASK 0xFF
#define PIT_SCALE 1193180
#define PIT_SET 0x36

#define CMD_BINARY 0x00

#define CMD_MODE0 0x00
#define CMD_MODE1 0x02
#define CMD_MODE2 0x04
#define CMD_MODE3 0x06
#define CMD_MODE4 0x08
#define CMD_MODE5 0x0a

#define CMD_RW_BOTH 0x30

#define CMD_COUNTER0 0x00
#define CMD_COUNTER2 0x80

uint32_t systemTime = 0;
uint32_t serviceId, timeEvent;
bool initialized = false;

void interruptHandler() {
    systemTime++;
    fireEventCode(timeEvent, systemTime, systemTime);
}

void doSleep(uint32_t millis) {
    uint32_t targetTime = systemTime + (millis-1) / 10 + 1;
    awaitCode(serviceId, timeEvent, targetTime);
}

int32_t main() {
    if (!initialized) {
        initialized = true;
        serviceId = getServiceId();
        // time event will have code and data equal to the current system time
        timeEvent = createEvent("time_update");
        createFunction("sleep", (void *)doSleep);

        uint32_t service = getService("pic");
        uint32_t event = getEvent(service, "irq0");
        subscribeEvent(service, event, interruptHandler);

        uint32_t hz = 100;
        int divisor = PIT_SCALE / hz;
        ioOut(PIT_CONTROL, CMD_BINARY | CMD_MODE3 | CMD_RW_BOTH | CMD_COUNTER0,
              1);
        ioOut(PIT_A, (uint8_t)divisor, 1);
        ioOut(PIT_A, (uint8_t)(divisor >> 8), 1);
        printf("timer handler installed\n");
    } else {
        printf("current uptime: %i.%is\n", systemTime / 100, systemTime % 100);
        printf("waiting one second to demonstrate sleeping\n");
        doSleep(1000);
    }
}
