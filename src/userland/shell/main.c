#define ALLOC_MAIN

#include <hlib.h>
#include <stdint.h>

char inputBuffer[256];
uint8_t inputBufferPosition;

void onKeyInput(uint32_t keycode, uint32_t stringId) { printf("%c", keycode); }

int32_t main() {
    createFunction("onKey", (void *)onKeyInput);
    printf("HONEY-OS - made by Lukas Eisenhauer\n> ");
}
