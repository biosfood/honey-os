#include <hlib.h>
#include <buffers.h>

// see https://github.com/msgpack/msgpack/blob/master/spec.md

#define FORMATS_STRUCTS_X(_name, _dataType, _min, _max, _readType, _readTypeParameter) \
    { .name = #_name, .dataType = TYPE_##_dataType, .readType = _readType, .readTypeParameter = _readTypeParameter, .min = _min, .max = _max }

FormatInfo formatInfo[] = {
    FORMATS(FORMATS_STRUCTS_X, COMMA)
};

Formats FirstByteToFormat[256];

uintmax_t msgPackReadLength(void *data, int8_t size) {
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

void *msgPackDump(uint8_t *data, uint32_t indent) {
    FormatInfo *info = &formatInfo[FirstByteToFormat[data[0]]];
    uint32_t bytesToRead = 1;
    uint32_t dataOffset = 0, dataSize = 0;
    uintmax_t length = msgPackReadLength(data, info->readTypeParameter);
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
        printf("%s%s: %s(%i)\n", indentData, hexData, info->name, msgPackReadInt(data)); break;
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
            next = msgPackDump(next, indent + 2);
        }
        break;
    case TYPE_MAP:
        printf("%s%s: %s(%i)\n", indentData, hexData, info->name, length);
        for (uint32_t i = 0; i < length; i++) {
            next = msgPackDump(next, indent + 1);
            next = msgPackDump(next, indent + 2);
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

void fillSpots(uint16_t from, uint16_t to, Formats value) {
    for (uint16_t i = from; i <= to; i++) {
        FirstByteToFormat[i] = value;
    }
}

#define FILL_SPOTS_X(name, dataType, min, max, readType, readTypeParameter) fillSpots(min, max, FORMAT_##name);
void initialize() {
    FORMATS(FILL_SPOTS_X, NOTHING);
}

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

uint32_t msgPackMapLength(uint32_t elementCount) {
    if (elementCount % 2) {
        printf("map: bad element count %i\n", elementCount);
        return 0;
    }
    return msgPackArrayLength(elementCount / 2);
}

void *msgPackMapWrite(void *buffer, uint32_t elementCount) {
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

// for reading values from a buffer: malloc is very slow, so only use it sparingly, when reutrning a value.
intmax_t msgPackReadInt(void *data) {
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

uint32_t msgPackReadUint(void *data) {
    intmax_t asInt = msgPackReadInt(data);
    if (asInt < 0) {
        printf("readUint: value %i is negative\n", asInt);
        return 0;
    }
    return asInt;
}

char *msgPackReadStr(void *data) {
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
    uint32_t size = msgPackReadLength(data, info->readTypeParameter);
    char *str = malloc(size + 1);
    memcpy(data + offset, str, size);
    str[size] = 0;
    return str;
}

uintmax_t msgPackReadArraySize(void *data, void **firstElement) {
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
    case FORMAT_ARRAY16:
        *firstElement = data + 3;
    case FORMAT_ARRAY32:
        *firstElement = data + 5;
    }
    return msgPackReadLength(data, info->readTypeParameter);
}

uintmax_t msgPackReadMapSize(void *data, void **firstElement) {
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
    return msgPackReadLength(data, info->readTypeParameter);
}

void *msgPackSeek(void *data) {
    uint8_t *buffer = (uint8_t *) data; 
    uint8_t format = FirstByteToFormat[*buffer];
    FormatInfo *info = &formatInfo[format];
    uint32_t length = msgPackReadLength(data, info->readTypeParameter);
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
            data = msgPackSeek(data);
        }
        return data;
    }
    // should never happen
    printf("seek: cannot read %s\n", info->name);
    return NULL;
}

void *msgPackMapGetFromInt(void *data, uintmax_t searchValue) {
    uint8_t *buffer = data;
    uint8_t format = FirstByteToFormat[*buffer];
    FormatInfo *info = &formatInfo[format];
    if (info->dataType != TYPE_MAP) {
        printf("mapGetFromInt cannot convert %s to a map\n", info->name);
        return 0;
    }
    uint8_t *element;
    uint32_t pairCount = msgPackReadMapSize(data, (void *)&element);
    for (uintmax_t i = 0; i < pairCount; i++) {
        if (formatInfo[FirstByteToFormat[*element]].dataType != TYPE_INTEGER) {
            element = msgPackSeek(element);
            element = msgPackSeek(element);
        }
        if (msgPackReadInt(element) == searchValue) {
            return msgPackSeek(element);
        }
        element = msgPackSeek(element);
        element = msgPackSeek(element);
    }
    printf("mapGetFromInt: key %i not found!\n", searchValue);
    // TODO: return something sensible here / throw an actual exception
    return NULL;
}

void *msgPackMapGetFromString(void *data, char *searchValue) {
    uint8_t *buffer = data;
    uint8_t format = FirstByteToFormat[*buffer];
    FormatInfo *info = &formatInfo[format];
    if (info->dataType != TYPE_MAP) {
        printf("mapGetFromString cannot convert %s to a map\n", info->name);
        return 0;
    }
    uint8_t *element;
    uint32_t pairCount = msgPackReadMapSize(data, (void *)&element);
    for (uintmax_t i = 0; i < pairCount; i++) {
        if (formatInfo[FirstByteToFormat[*element]].dataType != TYPE_STRING) {
            element = msgPackSeek(element);
            element = msgPackSeek(element);
        }
        char *key = msgPackReadStr(element);
        bool equal = true;
        for (uint32_t i = 0; searchValue[i]; i++) {
            if (key[i] != searchValue[i]) {
                equal = false;
                break;
            }
        }
        free(key);
        if (equal) {
            return msgPackSeek(element);
        }
        element = msgPackSeek(element);
        element = msgPackSeek(element);
    }
    printf("mapGetFromString: key '%s' not found!\n", searchValue);
    // TODO: return something sensible here / throw an actual exception
    return NULL;
}

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
     X(STRING, "world") S \
    X(INTEGER, 2) S \
     X(STRING, "Number 2") S \
    X(STRING, "number") S \
     X(INTEGER, 1)

#define SAMPLE_3(X) \
    X(MAP, SAMPLE_3_MAP_CONTENTS)

uint32_t testFunction(void *data) {
    GET(STRING, hello);
    GET(INT, number);

    printf("parameters: hello=%s, number=%i\n", hello, number);
    free(hello);
    return 0;
}

int32_t main() {
    static bool intitialized = false;
    if (!intitialized) {
        intitialized = true;
        initialize();
    }
    CREATE(test, SAMPLE_3);
    msgPackDump(test, 0);

    testFunction(test);

    free(test);
}