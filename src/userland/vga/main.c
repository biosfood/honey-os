#include <color.h>
#include <cursor.h>
#include <hlib.h>
#include <stdbool.h>
#include <stdint.h>

volatile uint16_t *videoSource;
uint32_t offset = 0;

#define WIDTH 80
#define HEIGHT 25

void writeChar(char character, char colorCode) {
    videoSource[offset] = ((uint16_t)colorCode << 8) | character;
    offset++;
    setCursorOffset(offset);
}

void scrollUp() {
    for (int y = 1; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            videoSource[x + (y - 1) * WIDTH] = videoSource[x + y * WIDTH];
        }
    }
    for (int x = 0; x < WIDTH; x++) {
        videoSource[x + (HEIGHT - 1) * WIDTH] = 0;
    }
}

void write(char c) {
    switch (c) {
    case '\r':
        offset = (offset / WIDTH) * WIDTH;
        setCursorOffset(offset);
        return;
    case '\n':
        offset = (offset / WIDTH + 1) * WIDTH;
        if (offset / WIDTH >= HEIGHT) {
            offset -= WIDTH;
            scrollUp();
        }
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

void writeBulk(uint32_t stringId) {
    uint32_t length = getStringLength(stringId);
    char *buffer = malloc(length);
    readString(stringId, buffer);
    for (uint32_t i = 0; i < length; i++) {
        write(buffer[i]);
    }
}


int32_t main() {
    videoSource = requestMemory(2, NULL, PTR(0xB8000));
    createFunction("writeChar", (void *)write);
    createFunction("writeBulk", (void *)writeBulk);
}
