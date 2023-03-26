#define ALLOC_MAIN

#include <hlib.h>
#include <stdint.h>

int32_t main() {
    loadFromInitrd("ioManager");
    printf("HONEY-OS - made by Lukas Eisenhauer\n");
    printf("finished loading all the essential modules\n");
    loadFromInitrd("lspci");
    uint32_t id = loadFromInitrdUninitialized("shell");
    requestName("ioManager", "setForeground", id, 0);
    request(id, 0, 0, 0);
}
