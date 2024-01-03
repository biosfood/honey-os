#ifndef PCI_H
#define PCI_H

#include <stdint.h>

typedef struct {
    uint32_t id;
    uint8_t bus, device, function;
    uint8_t class, subclass;
    uint16_t configuration;
    uint16_t deviceId, vendorId, programmingInterface;
    uint32_t bar[6];
} PciDevice;

#endif
