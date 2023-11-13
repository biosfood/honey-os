#ifndef MBR_H
#define MBR_H

#include <hlib.h>

typedef struct {
    uint8_t active;
    uint8_t startCHS[3];
    uint8_t type;
    uint8_t endCHS[3];
    uint32_t lbaStart;
    uint32_t sectorCount;
} PartitionTableEntry;

typedef struct {
    PartitionTableEntry entries[4];
    uint8_t signature[2];
} ClassicMBR;

typedef struct {
    uint32_t id;
    uint32_t serviceId, deviceId, readFunktion, writeFunktion;
    ClassicMBR *mbr;
} MbrDevice;

#endif