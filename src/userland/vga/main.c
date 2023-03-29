#include <color.h>
#include <cursor.h>
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
    setCursorOffset(offset);
}

void write(char c) {
    switch (c) {
    case '\r':
        offset = (offset / WIDTH) * WIDTH;
        setCursorOffset(offset);
        return;
    case '\n':
        offset = (offset / WIDTH + 1) * WIDTH;
        setCursorOffset(offset);
        return;
    case '\b':
        offset--;
        videoSource[offset] = ((uint16_t)COLOR(white, black) << 8) | ' ';
        setCursorOffset(offset);
        return;
    }
    writeChar(c, COLOR(white, black));
}

int32_t main() {
    videoSource = requestMemory(2, NULL, PTR(0xB8000));
    createFunction("writeChar", (void *)write);
    return 0;
}
