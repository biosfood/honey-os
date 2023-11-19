#include <hlib.h>
#include <buffers.h>

// see https://github.com/msgpack/msgpack/blob/master/spec.md

#define FORMATS_STRUCTS_X(_name, _dataType, _min, _max, _readType, _readTypeParameter) \
    { .name = #_name, .dataType = TYPE_##_dataType, .readType = _readType, .readTypeParameter = _readTypeParameter }

FormatInfo formatInfo[] = {
    FORMATS(FORMATS_STRUCTS_X, COMMA)
};

Formats FirstByteToFormat[256];

void dumpPack(uint8_t *data, uint32_t indent) {
    Formats format = FirstByteToFormat[data[0]];
    FormatInfo *info = &formatInfo[format];
    printf("%x: %s\n", data[0], info->name);
}

uint8_t demoPack[] = {1}; // fixint 1

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