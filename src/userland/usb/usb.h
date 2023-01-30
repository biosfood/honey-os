#ifndef USB_H
#define USB_H

#include <hlib.h>

#define OFFSET(ptr, off) (((void *)ptr) + off)

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

typedef struct {
    uint32_t usbCommand, usbStatus;
    uint64_t pageSize;
    uint32_t deviceNotificationControl, commandRingControl;
    uint64_t deviceContextArrayOffset;
    uint32_t configure;
} XHCIOperationalRegisters;

typedef struct {
    uint32_t statusControl, powerManagement, linkInfo, reserved;
} XHCIPortRegister;

#endif
