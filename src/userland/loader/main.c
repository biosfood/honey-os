#include <hlib.h>
#include <stdint.h>

int32_t main() {
    loadFromInitrd("log");
    log("hello world");
    log("honey os is alive :)");
    loadFromInitrd("pic");
    return 0;
}
