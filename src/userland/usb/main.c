#define ALLOC_MAIN
#include <hlib.h>

#include "usb.h"

int32_t main() {
    uint32_t pciService = getService("lspci");
    uint32_t function = getFunction(pciService, "getDeviceClass");
    uint32_t i = 0, class = 0;
    while ((class = request(pciService, function, i, 0))) {
        printf("device %i has class 0x%x\n", i, class);
        i++;
    }
}
