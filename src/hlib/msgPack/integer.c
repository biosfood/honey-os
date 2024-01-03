#include "msgPack.h"

uint32_t msgPackIntegerLength(int32_t value, IntegerType integerType) {
    if ((value & 0x7F) == value || ((~value) & 0x1F) == ~value) {
        // fixint
        return 1;
    }
    value = ABS(value);
    if ((value & 0xFF) == value) {
        // int8
        return 2;
    }
    if ((value & 0xFFFF) == value) {
        // int16
        return 3;
    }
    if ((value & 0xFFFFFFFF) == value) {
        // int32
        return 5;
    }
    // int64
    return 9;
}

void *msgPackIntegerWrite(void *buffer, int32_t x, IntegerType type) {
    uint8_t *bufferByte = buffer;
    if (x < 0 && type != Signed) {
        // printf("integerWrite: %i is negative but type is Unsigned!\n", x);
        // should never occur when using the correct macros
        // as a failsave, treat it as a uint32_t
        *bufferByte = formatInfo[FORMAT_UINT32].min;
        *(uint32_t *)(buffer + 1) = (uint32_t) x;
        return buffer + 5;
    }
    if ((x & 0x7F) == x) {
        *bufferByte = (uint8_t)x;
        // fixint
        return buffer + 1;
    }
    if (((~x) & 0x1F) == ~x) {
        *bufferByte = (int8_t)x;
        // negative fixint
        return buffer + 1;
    }
    if ((uint8_t)x == x) {
        *bufferByte = formatInfo[FORMAT_UINT8].min;
        *(uint8_t *)(buffer + 1) = (uint8_t) x;
        return buffer + 2;
    }
    if ((int8_t) x == x) {
        *bufferByte = formatInfo[FORMAT_INT8].min;
        *(int8_t *)(buffer + 1) = (int8_t) x;
        return buffer + 2;
    }
    if ((uint16_t)x == x) {
        *bufferByte = formatInfo[FORMAT_UINT16].min;
        *(uint16_t *)(buffer + 1) = (uint16_t) x;
        return buffer + 3;
    }
    if ((int16_t) x == x) {
        *bufferByte = formatInfo[FORMAT_INT16].min;
        *(int16_t *)(buffer + 1) = (int16_t) x;
        return buffer + 3;
    }
    if ((uint32_t)x == x) {
        *bufferByte = formatInfo[FORMAT_UINT32].min;
        *(uint32_t *)(buffer + 1) = (uint32_t) x;
        return buffer + 5;
    }
    if ((int32_t) x == x) {
        *bufferByte = formatInfo[FORMAT_INT32].min;
        *(int32_t *)(buffer + 1) = (int32_t) x;
        return buffer + 5;
    }
    // TODO: 64 bit numbers
    return buffer;
}

intmax_t msgPackReadInt(AllocationData allocationData, void *data) {
    uint8_t *buffer = (uint8_t *) data;
    uint8_t format = FirstByteToFormat[*buffer];
    FormatInfo *info = &formatInfo[format];
    if (info->dataType != TYPE_INTEGER) {
        printf("readInt: cannot convert %s to int\n", info->name);
        return 0;
    }
    if (format < FORMAT_INT8) {
        // definietly working with a uint
        if (format == FORMAT_POSITIVE_FIXINT) {
            return *((uint8_t *)data) & ((1 << (-info->readTypeParameter)) - 1);
        }
        if (format == FORMAT_UINT8) {
            return *((uint8_t *)(data + 1));
        }
        if (format == FORMAT_UINT16) {
            return *((uint16_t *)(data + 1));
        }
        if (format == FORMAT_UINT32) {
            return *((uint32_t *)(data + 1));
        }
        goto fail;
    }
    if (format == FORMAT_NEGATIVE_FIXINT) {
        return (intmax_t) (int8_t) (*buffer);
    }
    if (format == FORMAT_INT8) {
        return (intmax_t) *((int8_t *)data);
    }
    if (format == FORMAT_INT16) {
        return (intmax_t) *((int16_t *)(data + 1));
    }
    if (format == FORMAT_INT32) {
        return (intmax_t) *((int32_t *)(data + 1));
    }
fail:
    // TODO: 64-bit numbers
    printf("readUint: cannot read %s\n", info->name);
    return 0;
}

uintmax_t msgPackReadUint(AllocationData allocationData, void *data) {
    intmax_t asInt = msgPackReadInt(allocationData, data);
    if (asInt < 0) {
        printf("readUint: value %i is negative\n", asInt);
        return 0;
    }
    return asInt;
}

void *msgPackMapGetFromInt(AllocationData allocationData, void *data, uintmax_t searchValue) {
    // need a reference to allocationData for printing
    uint8_t *buffer = data;
    uint8_t format = FirstByteToFormat[*buffer];
    FormatInfo *info = &formatInfo[format];
    if (info->dataType != TYPE_MAP) {
        printf("mapGetFromInt cannot convert %s to a map\n", info->name);
        return 0;
    }
    uint8_t *element;
    uint32_t pairCount = msgPackReadMapSize(allocationData, data, (void *)&element);
    for (uintmax_t i = 0; i < pairCount; i++) {
        if (formatInfo[FirstByteToFormat[*element]].dataType != TYPE_INTEGER) {
            element = msgPackSeek(allocationData,  element);
            element = msgPackSeek(allocationData, element);
        }
        if (msgPackReadInt(allocationData, element) == searchValue) {
            return msgPackSeek(allocationData, element);
        }
        element = msgPackSeek(allocationData, element);
        element = msgPackSeek(allocationData, element);
    }
    printf("mapGetFromInt: key %i not found!\n", searchValue);
    // TODO: return something sensible here / throw an actual exception
    return NULL;
}