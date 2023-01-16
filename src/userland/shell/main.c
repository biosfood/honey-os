#define ALLOC_MAIN

#include <hlib.h>
#include <stdint.h>

char buffer[256];

int32_t main() {
    printf("HONEY-OS - made by Lukas Eisenhauer - shell\n");
    while (1) {
        printf("> ");
        gets(buffer);
        loadFromInitrd(buffer);
    }
}
