#include <hlib.h>
#include <stdint.h>

int32_t main() {
    loadFromInitrd("log");
    loadFromInitrd("parallel");
    log("hello world");
    log("honey os is alive :)");
    loadFromInitrd("pic");
    loadFromInitrd("keyboard");
    return 0;
}
