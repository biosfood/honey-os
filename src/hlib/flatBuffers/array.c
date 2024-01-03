#include "flatBuffers.h"

uint32_t msgPackArrayLength(uint32_t elementCount) {
    if ((elementCount & formatInfo[FORMAT_FIXARRAY].readTypeParameter) == elementCount) {
        return 1;
    }
    if ((uint16_t)elementCount == elementCount) {
        return 3;
    }
    if ((uint32_t)elementCount == elementCount) {
        return 5;
    }
    // TODO: 64 bit numbers
    return 1;
}

void *msgPackArrayWrite(void *buffer, uint32_t elementCount) {
    uint8_t *bufferByte = buffer;
    if ((elementCount & 0xF) == elementCount) {
        *bufferByte = formatInfo[FORMAT_FIXARRAY].min + elementCount;
        return buffer + 1;
    }
    if ((uint16_t)elementCount == elementCount) {
        *bufferByte = formatInfo[FORMAT_ARRAY16].min;
        *(uint16_t *)(buffer + 1) = (uint16_t) elementCount;
        return buffer + 3;
    }
    if ((uint32_t)elementCount == elementCount) {
        *bufferByte = formatInfo[FORMAT_ARRAY32].min;
        *(uint32_t *)(buffer + 1) = (uint32_t) elementCount;
        return buffer + 5;
    }
    // TODO: 64 bit numbers
    return buffer;
}

uint32_t msgPackMapLength(AllocationData allocationData, uint32_t elementCount) {
    if (elementCount % 2) {
        printf("map: bad element count %i\n", elementCount);
        return 0;
    }
    return msgPackArrayLength(elementCount / 2);
}

void *msgPackMapWrite(void *buffer, uint32_t elementCount) {
    uint8_t *bufferByte = buffer;
    if (elementCount % 2) {
        // printf("map: bad element count %i\n", elementCount);
        // should never occur
        return buffer;
    }
    elementCount >>= 1;
    if ((elementCount & 0x0F) == elementCount) {
        *bufferByte = formatInfo[FORMAT_FIXMAP].min + elementCount;
        // fixmap
        return buffer + 1;
    }
    if ((elementCount & 0xFFFF) == elementCount) {
        *bufferByte = formatInfo[FORMAT_MAP16].min;
        *(uint16_t *)(buffer + 1) = (uint16_t) elementCount;
        return buffer + 3;
    }
    *bufferByte = formatInfo[FORMAT_MAP32].min;
    *(uint32_t *)(buffer + 1) = elementCount;
    return buffer + 5;
}

uintmax_t msgPackReadArraySize(AllocationData allocationData, void *data, void **firstElement) {
    uint8_t *buffer = (uint8_t *) data;
    uint8_t format = FirstByteToFormat[*buffer];
    FormatInfo *info = &formatInfo[format];
    if (info->dataType != TYPE_ARRAY) {
        printf("readArraySize: cannot convert %s to array\n", info->name);
        return 0;
    }
    switch (format) {
    case FORMAT_FIXARRAY:
        *firstElement = data + 1;
        break;
    case FORMAT_ARRAY16:
        *firstElement = data + 3;
        break;
    case FORMAT_ARRAY32:
        *firstElement = data + 5;
        break;
    }
    return msgPackReadLength(allocationData, data, info->readTypeParameter);
}

uintmax_t msgPackReadMapSize(AllocationData allocationData, void *data, void **firstElement) {
    uint8_t *buffer = data;
    uint8_t format = FirstByteToFormat[*buffer];
    FormatInfo *info = &formatInfo[format];
    if (info->dataType != TYPE_MAP) {
        printf("readMapSize: cannot convert %s to a map\n", info->name);
        return 0;
    }
    switch (format) {
    case FORMAT_FIXMAP:
        *firstElement = data + 1; break;
    case FORMAT_MAP16:
        *firstElement = data + 3; break;
    case FORMAT_MAP32:
        *firstElement = data + 5; break;
    }
    return msgPackReadLength(allocationData, data, info->readTypeParameter);
}