#include <hlib.h>
#include <buffers.h>

// see https://github.com/msgpack/msgpack/blob/master/spec.md

#define FORMATS_STRUCTS_X(_name, _dataType, _min, _max, _readType, _readTypeParameter) \
    { .name = #_name, .dataType = TYPE_##_dataType, .readType = _readType, .readTypeParameter = _readTypeParameter }

FormatInfo formatInfo[] = {
    FORMATS(FORMATS_STRUCTS_X, COMMA)
};

Formats FirstByteToFormat[256];

uint32_t readLength(void *data, uint32_t size) {
    switch (size) {
    case 1:
        return *((uint8_t *)data);
    case 2:
        return *((uint16_t *)data);
    case 4:
        return *((uint32_t *)data);
    // TODO: 64-bit numbers!
    }
    printf("cannot read length of size %i!\n", size);
    return 0;
}

int32_t readInteger(uint8_t *data, Formats format, FormatInfo *info) {
    if (format == FORMAT_NEGATIVE_FIXINT) {
        return ((int8_t) (*data | ~info->readTypeParameter));
    }
    if (format == FORMAT_POSITIVE_FIXINT ||
        format == FORMAT_UINT8 ||
        format == FORMAT_UINT16 ||
        format == FORMAT_UINT32 ||
        format == FORMAT_UINT64) {

        return *(uint32_t *)data;
    }
    if (data[info->readTypeParameter-1] & 0x80) {
        for (uint8_t i = info->readTypeParameter; i < 8; i++) {
            data[i] = 0xFF;
        }
    }
    return *(int32_t *)data;
}

void *dumpPack(uint8_t *data, uint32_t indent) {
    uint8_t firstByte = data[0];
    Formats format = FirstByteToFormat[firstByte];
    FormatInfo *info = &formatInfo[format];
    uint32_t bytesToRead = 1;
    uint32_t dataOffset = 0, dataSize = 0;
    switch (info->readType) {
    case Inline:
        break;
    case InlineLength:
        bytesToRead += firstByte & info->readTypeParameter;
        dataOffset = 1;
        dataSize = firstByte & info->readTypeParameter;
        break;
    case FixedLength:
        bytesToRead += info->readTypeParameter;
        dataOffset = 1;
        dataSize = info->readTypeParameter;
        break;
    case ReadLength:
        uint32_t length = readLength(data + 1, info->readTypeParameter);
        bytesToRead += info->readTypeParameter + length;
        dataOffset = 1 + info->readTypeParameter;
        dataSize = length;
        break;
    case ElementsInline:
        dataOffset = 1;
        break;
    case ReadElements:
        bytesToRead += info->readTypeParameter;
        dataOffset = 1;
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
    printf("%s%s: ", indentData, hexData);
    void *buffer = malloc(MAX(bytesToRead + 1, 8));
    switch (info->readType) {
    case Inline:
    case ElementsInline:
        *((uint8_t *)buffer) = firstByte & info->readTypeParameter; break;
    case InlineLength:
    case FixedLength:
    case ReadLength:
    case ReadElements:
        memcpy(data + dataOffset, buffer, dataSize); break;
    default: break;
    }
    uint32_t size;
    void *next = data + bytesToRead;
    switch (info->dataType) {
    case TYPE_NIL:
        printf("NIL"); break;
    case TYPE_INTEGER:
        printf("int(%i)", readInteger(buffer, format, info)); break;
    case TYPE_BOOLEAN:
        printf("bool(%s)", readInteger(buffer, format, info) ? "true" : "false"); break;
    // can't even print a float yet...
    case TYPE_STRING:
        ((uint8_t *)buffer)[bytesToRead] = 0;
        printf("str(\"%s\")", buffer); break;
    case TYPE_ARRAY:
        size = readInteger(buffer, format, info);
        printf("array(%i) (%s)\n", size, info->name);
        for (uint32_t i = 0; i < size; i++) {
            next = dumpPack(next, indent + 2);
        }
        return next;
    case TYPE_MAP:
        size = readInteger(buffer, format, info);
        printf("map(%i) (%s)\n", size, info->name);
        for (uint32_t i = 0; i < size; i++) {
            next = dumpPack(next, indent + 1);
            next = dumpPack(next, indent + 2);
        }
        return next;

    default:
        printf("unknown"); break;
    }
    printf(" (%s)\n", info->name);
    free(hexData);
    free(indentData);
    free(buffer);
    return next;
}

uint8_t demo1[] = {0x92, 1, 5}; // fixarray
uint8_t demo2[] = {0xDE, 2, 0, 0xA2, 'h', 'i', 1, 0xA2, ';', ')', 0xC3 }; // map16
uint8_t demo3[] = {0xC3}; // true
uint8_t demo4[] = {0xA2, 'h', 'i'}; // fixstring
uint8_t demo5[] = {0xD9, 5, 'w', 'o', 'r', 'l', 'd'}; // str8

void fillSpots(uint16_t from, uint16_t to, Formats value) {
    for (uint16_t i = from; i <= to; i++) {
        FirstByteToFormat[i] = value;
    }
}

#define FILL_SPOTS_X(name, dataType, min, max, readType, readTypeParameter) fillSpots(min, max, FORMAT_##name);
void initialize() {
    FORMATS(FILL_SPOTS_X, NOTHING);
}

int32_t main() {
    static bool intitialized = false;
    if (!intitialized) {
        intitialized = true;
        initialize();
    }
    printf("dumping test data...\n");
    dumpPack(demo1, 0);
    dumpPack(demo2, 0);
    dumpPack(demo3, 0);
    dumpPack(demo4, 0);
    dumpPack(demo5, 0);
}