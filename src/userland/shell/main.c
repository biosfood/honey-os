#define ALLOC_MAIN

#include <hlib.h>
#include <stdint.h>

char inputBuffer[256];
uint8_t inputBufferPosition;
bool printInput = true;

void onNewLine() {
    printf("\nInput: %s\n> ", inputBuffer);
    inputBufferPosition = 0;
    inputBuffer[inputBufferPosition] = '\0';
}

void onKeyInput(uint32_t keycode, uint32_t stringId) {
    switch (keycode) {
    case '\b':
        if (inputBufferPosition) {
            inputBufferPosition--;
            inputBuffer[inputBufferPosition] = 0;
            printf("\b");
        }
        break;
    case '\n':
        onNewLine();
        break;
    default:
        if (printInput) {
            printf("%c", keycode);
        }
        inputBuffer[inputBufferPosition] = (char)keycode;
        inputBufferPosition++;
        inputBuffer[inputBufferPosition] = '\0';
        break;
    }
}

int32_t main() {
    createFunction("onKey", (void *)onKeyInput);
    printf("HONEY-OS - made by Lukas Eisenhauer\n> ");
}
