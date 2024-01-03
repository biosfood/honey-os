#include "flatBuffers.h"

uint32_t msgPackStringLength(uint32_t strlength) {
    if ((strlength & 0x1F) == strlength) {
        // fixstr
        return 1 + strlength;
    }
    if ((strlength & 0xFF) == strlength) {
        // str8
        return 2 + strlength;
    }
    if ((strlength & 0xFFFF) == strlength) {
        // str16
        return 3 + strlength;
    }
    // str32
    return 5 + strlength;
}

void *msgPackStringWrite(void *buffer, char *string) {
    uint32_t length = strlen(string);
    uint8_t *bufferByte = buffer;
    if ((length & 0x1F) == length) {
        *bufferByte = formatInfo[FORMAT_FIXSTR].min + length;
        buffer++;
    } else if ((length & 0xFF) == length) {
        *bufferByte = formatInfo[FORMAT_STR8].min;
        *(uint8_t *)(buffer + 1) = (uint8_t) length;
        buffer += 2;
    } else if ((length & 0xFFFF) == length) {
        *bufferByte = formatInfo[FORMAT_STR16].min;
        *(uint16_t *)(buffer + 1) = (uint16_t) length;
        buffer += 3;
    } else {
        *bufferByte = formatInfo[FORMAT_STR32].min;
        *(uint32_t *)(buffer + 1) = (uint32_t) length;
        buffer += 5;
    }
    memcpy(string, buffer, length);
    return buffer + length;
}


char *msgPackReadStr(AllocationData allocationData, void *data) {
    uint8_t *buffer = (uint8_t *) data;
    uint8_t format = FirstByteToFormat[*buffer];
    FormatInfo *info = &formatInfo[format];
    if (info->dataType != TYPE_STRING) {
        printf("readString: cannot convert %s to string\n", info->name);
        return NULL;
    }
    uint8_t offset;
    if (format == FORMAT_FIXSTR) {
        offset = 1;
    } else if (format == FORMAT_STR8) {
        offset = 2;
    } else if (format == FORMAT_STR16) {
        offset = 3;
    } else if (format == FORMAT_STR32) {
        offset = 5;
    }
    uint32_t length = msgPackReadLength(allocationData, data, info->readTypeParameter);
    char *str = malloc(length + 1);
    memcpy(data + offset, str, length);
    str[length] = 0;
    return str;
}

void *msgPackMapGetFromString(AllocationData allocationData, void *data, char *searchValue) {
    uint8_t *buffer = data;
    uint8_t format = FirstByteToFormat[*buffer];
    FormatInfo *info = &formatInfo[format];
    if (info->dataType != TYPE_MAP) {
        printf("mapGetFromString cannot convert %s to a map\n", info->name);
        return 0;
    }
    uint8_t *element;
    uint32_t pairCount = msgPackReadMapSize(allocationData, data, (void *)&element);
    for (uintmax_t i = 0; i < pairCount; i++) {
        if (formatInfo[FirstByteToFormat[*element]].dataType != TYPE_STRING) {
            element = msgPackSeek(allocationData, element);
            element = msgPackSeek(allocationData, element);
        }
        char *key = msgPackReadStr(allocationData, element);
        bool equal = true;
        for (uint32_t i = 0; searchValue[i]; i++) {
            if (key[i] != searchValue[i]) {
                equal = false;
                break;
            }
        }
        free(key);
        if (equal) {
            return msgPackSeek(allocationData, element);
        }
        element = msgPackSeek(allocationData, element);
        element = msgPackSeek(allocationData, element);
    }
    printf("mapGetFromString: key '%s' not found!\n", searchValue);
    // TODO: return something sensible here / throw an actual exception
    return NULL;
}