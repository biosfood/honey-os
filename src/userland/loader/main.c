#define ALLOC_MAIN

#include <hlib.h>
#include <stdint.h>

int32_t main() {
    loadFromInitrd("log");
    loadFromInitrd("vga");
    loadFromInitrd("parallel");
    log("hello world");
    log("honey os is alive :)");
    loadFromInitrd("pic");
    loadFromInitrd("keyboard");
    malloc(10);
    log("finished loading essential modules");
    return 0;
}
