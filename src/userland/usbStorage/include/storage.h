#ifndef STORAGE_H
#define STORAGE_H

#include <hlib.h>

typedef struct {
    uint32_t serviceId;
    uint32_t deviceId;
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

#endif // STORAGE_H