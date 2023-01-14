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
        offset = (offset / WIDTH + 1) * WIDTH;
        return;
    }
    writeChar(c, COLOR(white, black));
}

int32_t main() {
    videoSource = requestMemory(2, NULL, PTR(0xB8000));
    createFunction("writeChar", (void *)write);
    return 0;
}
