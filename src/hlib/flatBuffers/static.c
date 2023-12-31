#include <hlib.h>
#include "flatBuffers.h"

// see https://github.com/msgpack/msgpack/blob/master/spec.md

#define FORMATS_STRUCTS_X(_name, _dataType, _min, _max, _readType, _readTypeParameter) \
    { .name = #_name, .dataType = TYPE_##_dataType, .readType = _readType, .readTypeParameter = _readTypeParameter, .min = _min, .max = _max }

FormatInfo formatInfo[] = {
    FORMATS(FORMATS_STRUCTS_X, COMMA)
};

Formats FirstByteToFormat[256];

void fillSpots(uint16_t from, uint16_t to, Formats value) {
    for (uint16_t i = from; i <= to; i++) {
        FirstByteToFormat[i] = value;
    }
}

#define FILL_SPOTS_X(name, dataType, min, max, readType, readTypeParameter) fillSpots(min, max, FORMAT_##name);
void initializeFlatBuffers() {
    FORMATS(FILL_SPOTS_X, NOTHING);
}