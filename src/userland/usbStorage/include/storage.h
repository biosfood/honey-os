#ifndef STORAGE_H
#define STORAGE_H

#include <hlib.h>

typedef struct {
    uint32_t serviceId;
    uint32_t deviceId;
    uint32_t getType;
} StorageDevice;

#endif // STORAGE_H