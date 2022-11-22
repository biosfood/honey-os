#include <color.h>
#include <hlib.h>
#include <stdbool.h>
#include <stdint.h>

char *string = "test";
uint16_t *videoSource;

void writeChar(char character, char colorCode) {
    *videoSource = ((uint16_t)colorCode << 8) | character;
    videoSource++;
}

int32_t main() {
    videoSource = requestMemory(1, NULL, PTR(0xB8000));
    for (uint32_t i = 0; string[i]; i++) {
        writeChar(string[i], COLOR(white, black));
    }
    return 0;
}
