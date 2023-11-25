#include <hlib.h>
#include <buffers.h>

// see https://github.com/msgpack/msgpack/blob/master/spec.md

#define FORMATS_STRUCTS_X(_name, _dataType, _min, _max, _readType, _readTypeParameter) \
    { .name = #_name, .dataType = TYPE_##_dataType, .readType = _readType, .readTypeParameter = _readTypeParameter, .min = _min, .max = _max }

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

void fillSpots(uint16_t from, uint16_t to, Formats value) {
    for (uint16_t i = from; i <= to; i++) {
        FirstByteToFormat[i] = value;
    }
}

#define FILL_SPOTS_X(name, dataType, min, max, readType, readTypeParameter) fillSpots(min, max, FORMAT_##name);
void initialize() {
    FORMATS(FILL_SPOTS_X, NOTHING);
}

uint32_t stringLength(uint32_t strlength) {
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

void *stringWrite(void *buffer, char *string) {
    uint32_t length = strlen(string);
    uint8_t *bufferByte = buffer;
    if ((length & 0x1F) == length) {
        *bufferByte = formatInfo[FORMAT_FIXSTR].min + (uint8_t) length;
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

uint32_t integerLength(int32_t value, IntegerType integerType) {
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

void *integerWrite(void *buffer, int32_t x, IntegerType type) {
    if (x < 0 && type != Signed) {
        printf("integerWrite: %i is negative but type is Unsigned!\n", x);
        return buffer;
    }
    uint8_t *bufferByte = buffer;
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

uint32_t arrayLength(uint32_t elementCount) {
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

void *arrayWrite(void *buffer, uint32_t elementCount) {
    uint8_t *bufferByte = buffer;
    if ((elementCount & formatInfo[FORMAT_FIXARRAY].readTypeParameter) == elementCount) {
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

uint32_t mapLength(uint32_t elementCount) {
    if (elementCount % 2) {
        printf("map: bad element count %i\n", elementCount);
        return 0;
    }
    return arrayLength(elementCount / 2);
}

void *mapWrite(void *buffer, uint32_t elementCount) {
    uint8_t *bufferByte = buffer;
    if (elementCount % 2) {
        printf("map: bad element count %i\n", elementCount);
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

#define SAMPLE_1(X) \
    X(INTEGER, 500, Signed)

#define SAMPLE_2_ARRAY_CONTENT(X, S) \
    X(INTEGER, 1) S \
    X(STRING, "hi") S \
    X(INTEGER, 500, Signed)

#define SAMPLE_2(X) \
    X(ARRAY, SAMPLE_2_ARRAY_CONTENT)

#define SAMPLE_3_MAP_CONTENTS(X, S) \
    X(INTEGER, 1) S \
    X(ARRAY, SAMPLE_2_ARRAY_CONTENT) S \
    X(STRING, "hello") S \
    X(STRING, "world")

#define SAMPLE_3(X) \
    X(MAP, SAMPLE_3_MAP_CONTENTS)

#define EXPECT(data, _dataType) \
    if (formatInfo[FirstByteToFormat[*((uint8_t *)data)]].dataType != TYPE_##_dataType) { \
        printf("failed EXPECT, expected %s, got %s\n", #_dataType, formatInfo[FirstByteToFormat[*((uint8_t *)data)]].name); \
    } else

int32_t main() {
    static bool intitialized = false;
    if (!intitialized) {
        intitialized = true;
        initialize();
    }
    printf("dumping test data...\n");
    CREATE(test, SAMPLE_3);
    EXPECT(test, MAP) {
        printf("test is a map\n");
    }
    EXPECT(test, STRING) {
        printf("test is a string\n");
    }
    printf("test 1 length: %i\n", testLength);
    dumpPack(test, 0);
    free(test);
}