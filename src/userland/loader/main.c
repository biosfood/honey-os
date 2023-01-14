#define ALLOC_MAIN

#include <hlib.h>
#include <stdint.h>

int32_t main() {
    loadFromInitrd("ioManager");
    printf("HONEY-OS - made by Lukas Eisenhauer");
    printf("finished loading all the essential modules");
    loadFromInitrd("crashTest");
    uint32_t id = loadFromInitrdUninitialized("shell");
    requestName("ioManager", "setForeground", id, 0);
    requestName("shell", "main", 0, 0);
}
