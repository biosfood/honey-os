#include <hlib.h>
#include <stdbool.h>
#include <stdint.h>

int32_t main() {
    uint32_t *videoSource = requestMemory(1, NULL, PTR(0xB8000));
    *videoSource = 0x07690748;
    return 0;
}
