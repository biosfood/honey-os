#include <hlib.h>
#include <stdint.h>

char buffer[256];

int32_t main() {
    printf("HONEY-OS - made by Lukas Eisenhauer - shell\n");
    while (1) {
        printf("> ");
        gets(buffer);
        if (!*buffer || *buffer == ' ') {
            continue;
        }
        uint32_t space_position = strlen(buffer);
        for (uint32_t i = 0; buffer[i]; i++) {
            if (buffer[i] == ' ') {
                space_position = i;
                break;
            }
        }
        buffer[space_position] = 0;
        char *command = buffer;
        char *arguments = buffer + space_position + 1;
        uint32_t service = getService(buffer);
        if (!service) {
            service = loadFromInitrdUninitialized(buffer);
        }
        if (service) {
            uint32_t function = getFunction(service, "terminal");
            if (function) {
                uint32_t args = insertString(arguments);
                request(service, function, args, 0);
            } else {
                request(service, 0, 0, 0);
            }
        } else {
            printf("%s: command not found\n", buffer);
        }
    }
}
