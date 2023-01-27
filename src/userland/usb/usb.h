#ifndef USB_H
#define USB_H

#include <hlib.h>

typedef struct {
    uint8_t capabilitiesSize;
    uint8_t reserved;
    uint16_t interfaceVersion;
    uint32_t structuralParameters[3];
    uint32_t capabilityParameters1;
    uint32_t doorbellOffset;
    uint32_t runtimeRegistersSpaceOffset;
    uint32_t capabilityParameters2;
} XHCICapabilities;

#endif
