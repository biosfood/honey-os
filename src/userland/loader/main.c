#define ALLOC_MAIN

#include <hlib.h>
#include <stdint.h>

int32_t main() {
    loadFromInitrd("ioManager");
    printf("HONEY-OS by Lukas Eisenhauer");
    printf("finished loading all the essential modules");
    loadFromInitrd("crashTest");
    return 0;
}
