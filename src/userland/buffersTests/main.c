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

void dumpPack(uint8_t *data, uint32_t indent) {
    uint8_t firstByte = data[0];
    Formats format = FirstByteToFormat[firstByte];
    FormatInfo *info = &formatInfo[format];
    uint32_t bytesToRead = 1;
    uint32_t dataOffset = 0;
    switch (info->readType) {
    case Inline:
        break;
    case InlineLength:
        bytesToRead += firstByte & info->readTypeParameter;
        dataOffset = 1;
        break;
    case FixedLength:
        bytesToRead += info->readTypeParameter;
        dataOffset = 1;
        break;
    case ReadLength:
        uint32_t length = readLength(data + 1, info->readTypeParameter);
        bytesToRead += info->readTypeParameter + length;
        dataOffset = info->readTypeParameter;
        break;
    case ElementsInline:
        break;
    case ReadElements:
        bytesToRead += info->readTypeParameter;
        dataOffset = info->readTypeParameter;
    }

    char *hexData = malloc(3*bytesToRead);
    for (uint32_t i = 0; i < bytesToRead; i++) {
        sprintf(hexData + 3*i, "%x ", data[i]);
    }
    hexData[3*bytesToRead - 1] = 0;
    printf("%s: %s\n", hexData, info->name);
    free(hexData);
}

uint8_t demoPack[] = {0xD0, 1}; // fixint 1

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
    dumpPack(demoPack, 0);
}