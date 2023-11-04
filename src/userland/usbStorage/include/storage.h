#ifndef STORAGE_H
#define STORAGE_H

#include <hlib.h>

typedef struct {
    uint32_t serviceId;
    uint32_t deviceId;
    uint32_t id;
    uint32_t inFunction, outFunction;
} StorageDevice;

typedef enum UsbStorageSubClass {
    SCISI = 0,
    RBC = 1,
    MMC_5 = 2,
    Obsolete1 = 3,
    UFI = 4,
    Obsolete2 = 5,
    SCISI_Transparent = 6,
    LSD_FS = 7,
    IEEE_1667 = 8,
    UnknownSubClass = 9
} UsbStorageSubClass;

typedef enum UsbStorageProtocol {
    CBI1 = 0,
    CBI2 = 1,
    Obsolete = 2,
    BulkOnly = 3,
    UAS = 4,
    UnknownProtocol = 5
} UsbStorageProtocol;

typedef struct {
    uint32_t size;
    uint32_t signature;
    uint32_t tag;
    uint32_t transferSize;
    union {
        uint8_t byte;
        struct {
            uint8_t reserved: 6;
            uint8_t obsolete: 1;
            uint8_t direction: 1;
        } __attribute__((packed)) values;
    } flags;
    uint8_t LUN;
    uint8_t length;
    uint8_t data[16];
} CommandBlockWrapper;

#endif // STORAGE_H