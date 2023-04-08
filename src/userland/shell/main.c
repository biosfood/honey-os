#define ALLOC_MAIN

#include <hlib.h>
#include <stdint.h>

char buffer[256];

int32_t main() {
    printf("HONEY-OS - made by Lukas Eisenhauer - shell\n");
    while (1) {
        printf("> ");
        gets(buffer);
        if (!*buffer) {
            continue;
        }
        uint32_t service = loadFromInitrdUninitialized(buffer);
        if (service) {
            request(service, 0, 0, 0);
        } else {
            printf("%s: command not found\n", buffer);
        }
    }
}
