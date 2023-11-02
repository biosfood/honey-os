#include <hlib.h>

int32_t registerDevice(uint32_t in, uint32_t out, uint32_t serviceName, uint32_t serviceId) {
    printf("registering a new SCISI device...\n");
}

int32_t main() {
    createFunction("register", (void *)registerDevice);
    return 0;
}