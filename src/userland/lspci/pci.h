#ifndef PIC_H
#define PCI_H

#include <stdint.h>

typedef struct {
    uint8_t bus, device, function;
    uint8_t class, subclass;
    uint16_t deviceId, vendorId, programmingInterface;
    uint32_t bar0, bar1, bar2, bar3, bar4, bar5;
} PciDevice;

#endif
