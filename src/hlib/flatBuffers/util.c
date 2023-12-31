#include "flatBuffers.h"

uintmax_t msgPackReadLength(AllocationData allocationData, void *data, int8_t size) {
    if (size < 0) {
        return (*(uint8_t *)(data)) & ((1 << (-size)) - 1);
    }
    switch (size) {
    case 1:
        return *((uint8_t *)(data + 1));
    case 2:
        return *((uint16_t *)(data + 1));
    case 4:
        return *((uint32_t *)(data + 1));
    // TODO: 64-bit numbers!
    }
    printf("cannot read length of size %i!\n", size);
    return 0;
}

void *_msgPackDump(AllocationData allocationData, uint8_t *data, uint32_t indent) {
    FormatInfo *info = &formatInfo[FirstByteToFormat[data[0]]];
    uint32_t bytesToRead = 1;
    uint32_t dataOffset = 0, dataSize = 0;
    uintmax_t length = msgPackReadLength(allocationData, data, info->readTypeParameter);
    switch (info->readType) {
    case Inline:
        break;
    case InlineLength:
        bytesToRead += length;
        dataOffset = 1;
        dataSize = length;
        break;
    case FixedLength:
        bytesToRead += info->readTypeParameter;
        dataOffset = 1;
        dataSize = info->readTypeParameter;
        break;
    case ReadLength:
        bytesToRead += info->readTypeParameter + length;
        dataOffset = 1 + info->readTypeParameter;
        dataSize = length;
        break;
    case ElementsInline:
        dataOffset = 1;
        break;
    case ReadElements:
        bytesToRead += info->readTypeParameter;
        dataOffset = 1 + info->readTypeParameter;
        dataSize = info->readTypeParameter;
    }
    char *hexData = malloc(3*bytesToRead);
    for (uint32_t i = 0; i < bytesToRead; i++) {
        sprintf(hexData + 3*i, "%x ", data[i]);
    }
    hexData[3*bytesToRead - 1] = 0;
    char *indentData = malloc(indent + 1);
    memset(indentData, ' ', indent);
    indentData[indent] = 0;
    void *next = data + bytesToRead;
    uint8_t *buffer;
    switch (info->dataType) {
    case TYPE_NIL:
        printf("%s%s: %s\n", indentData, hexData, info->name); break;
    case TYPE_INTEGER:
        printf("%s%s: %s(%i)\n", indentData, hexData, info->name, msgPackReadInt(allocationData, data)); break;
    case TYPE_BOOLEAN:
        printf("%s%s: %s(%s)\n", indentData, hexData, info->name, length ? "true" : "false"); break;
    // can't even print a float yet...
    case TYPE_STRING:
        buffer = malloc(length + 1);
        memcpy(data + dataOffset, buffer, length);
        buffer[length] = 0;
        printf("%s%s: %s(\"%s\")\n", indentData, hexData, info->name, buffer);
        free(buffer);
        break;
    case TYPE_ARRAY:
        printf("%s%s: %s(%i)\n", indentData, hexData, info->name, length);
        for (uint32_t i = 0; i < length; i++) {
            next = _msgPackDump(allocationData, next, indent + 2);
        }
        break;
    case TYPE_MAP:
        printf("%s%s: %s(%i)\n", indentData, hexData, info->name, length);
        for (uint32_t i = 0; i < length; i++) {
            next = _msgPackDump(allocationData, next, indent + 1);
            next = _msgPackDump(allocationData, next, indent + 2);
        }
        break;
    default:
        // this branch should actually be impossible to reach...
        printf("unknown\n"); break;
    }
    free(hexData);
    free(indentData);
    free(buffer);
    return next;
}

void *msgPackSeek(AllocationData allocationData, void *data) {
    uint8_t *buffer = (uint8_t *) data; 
    uint8_t format = FirstByteToFormat[*buffer];
    FormatInfo *info = &formatInfo[format];
    uint32_t length = msgPackReadLength(allocationData, data, info->readTypeParameter);
    if (info->dataType == TYPE_MAP) {
        length <<= 1;
    }
    switch (info->readType) {
    case Inline:
        return data + 1;
    case FixedLength:
        return data + 1 + info->readTypeParameter;
    case ReadLength:
        return data + 1 + info->readTypeParameter + length;
    case InlineLength:
        return data + 1 + length;
    case ElementsInline:
        data++;
        goto READ_ELEMENTS;
    case ReadElements:
        data += 1 + info->readTypeParameter;
    READ_ELEMENTS:
        for (uint8_t i = 0; i < length; i++) {
            data = msgPackSeek(allocationData, data);
        }
        return data;
    }
    // should never happen
    printf("seek: cannot read %s\n", info->name);
    return NULL;
}