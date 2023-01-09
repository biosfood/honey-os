#define ALLOC_MAIN

#include <hlib.h>
#include <stdint.h>

int32_t main() {
    loadFromInitrd("log");
    loadFromInitrd("vga");
    loadFromInitrd("parallel");
    log("hello world! honey os is alive :)");
    loadFromInitrd("pic");
    loadFromInitrd("keyboard");
    printf("test print string: '%s', number: %i, hex: 0x%x", "hello world",
           1234, 0xB105F00D);
    log("finished loading essential modules");
    loadFromInitrd("crashTest");
    return 0;
}
