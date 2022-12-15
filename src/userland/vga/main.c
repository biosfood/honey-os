#include <color.h>
#include <hlib.h>
#include <stdbool.h>
#include <stdint.h>

uint16_t *videoSource;
uint32_t offset = 0;

#define WIDTH 80
#define HEIGHT 25

void writeChar(char character, char colorCode) {
    videoSource[offset] = ((uint16_t)colorCode << 8) | character;
    offset++;
}

void write(char c) {
    switch (c) {
    case '\r':
        offset = (offset / WIDTH) * WIDTH;
        return;
    case '\n':
        offset += WIDTH;
        return;
    }
    writeChar(c, COLOR(white, black));
}

int32_t main() {
    videoSource = requestMemory(2, NULL, PTR(0xB8000));
    uint32_t thisService = getServiceId();
    uint32_t functionId = createFunction("writeVGA", (void *)write);
    requestName("log", "registerOut", thisService, functionId);
    return 0;
}
