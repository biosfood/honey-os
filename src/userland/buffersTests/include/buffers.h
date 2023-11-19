#ifndef BUFFERS_H
#define BUFFERS_H

#include <hlib.h>

#define COMMA ,
#define SEMICOLON ;
#define NOTHING 

typedef enum ReadTypes {
    Inline,
    InlineLength,
    FixedLength,
    ReadLength,
    ElementsInline,
    ReadElementts,
} ReadTypes;

#define DATA_TYPES(X, S) \
    X(INTEGER) S \
    X(NIL) S \
    X(BOOLEAN) S \
    X(FLOAT) S \
    X(STRING) S \
    X(BINARY) S \
    X(ARRAY) S \
    X(MAP) S \
    X(EXTENSION) S \
    X(UNUSED)

#define FORMATS(X, S) \
    X(POSITIVE_FIXINT, INTEGER, 0x00, 0x7F, Inline, 0x7F) S \
    X(FIXMAP, MAP, 0x80, 0x8F, InlineLength, 0x0F) S \
    X(FIXARRAY, ARRAY, 0x90, 0x9F, InlineLength, 0x0F) S \
    X(FIXSTR, STRING, 0xa0, 0xBF, InlineLength, 0x1F) S \
    X(NIL, NIL, 0xC0, 0xC0, Inline, 0x00) S \
    X(UNUSED, UNUSED, 0xC1, 0xC1, Inline, 0x00) S \
    X(FIXBOOL, BOOLEAN, 0xC2, 0xC3, Inline, 0x01) S \
    X(BIN8, BINARY, 0xC4, 0xC4, ReadLength, 1) S \
    X(BIN16, BINARY, 0xC5, 0xC5, ReadLength, 2) S \
    X(BIN32, BINARY, 0xc6, 0xC6, ReadLength, 4) S \
    X(EXT8, EXTENSION, 0xC7, 0xC7, ReadLength, 1) S \
    X(EXT16, EXTENSION, 0xC8, 0xC8, ReadLength, 2) S \
    X(EXT32, EXTENSION, 0xC9, 0xC9, ReadLength, 4) S \
    X(FLOAT32, FLOAT, 0xCA, 0xCA, FixedLength, 4) S \
    X(FLOAT64, FLOAT, 0xCB, 0xCB, FixedLength, 8) S \
    X(UINT8, INTEGER, 0xCC, 0xCC, FixedLength, 1) S \
    X(UINT16, INTEGER, 0xCD, 0xCD, FixedLength, 2) S \
    X(UINT32, INTEGER, 0xCE, 0xCE, FixedLength, 4) S \
    X(UINT64, INTEGER, 0xCF, 0xCF, FixedLength, 8) S \
    X(INT8, INTEGER, 0xD0, 0xD0, FixedLength, 1) S \
    X(INT16, INTEGER, 0xD1, 0xD1, FixedLength, 2) S \
    X(INT32, INTEGER, 0xD2, 0xD2, FixedLength, 4) S \
    X(INT64, INTEGER, 0xD3, 0xD3, FixedLength, 8) S \
    X(FIXEXT1, EXTENSION, 0xD4, 0xD4, FixedLength, 1) S \
    X(FIXEXT2, EXTENSION, 0xD5, 0xD5, FixedLength, 2) S \
    X(FIXEXT4, EXTENSION, 0xD6, 0xD6, FixedLength, 4) S \
    X(FIXEXT8, EXTENSION, 0xD7, 0xD7, FixedLength, 8) S \
    X(FIXEXT16, EXTENSION, 0xD8, 0xD8, FixedLength, 16) S \
    X(STR8, STRING, 0xD9, 0xD9, ReadLength, 1) S \
    X(STR16, STRING, 0xDA, 0xDA, ReadLength, 2) S \
    X(STR32, STRING, 0xDB, 0xDB, ReadLength, 4) S \
    X(ARRAY16, ARRAY, 0xDC, 0xDC, ReadLength, 2) S \
    X(ARRAY32, ARRAY, 0xDD, 0xDD, ReadLength, 4) S \
    X(MAP16, MAP, 0xDE, 0xDE, ReadLength, 2) S \
    X(MAP32, MAP, 0xDF, 0xDF, ReadLength, 4) S \
    X(NEGATIVE_FIXINT, INTEGER, 0xE0, 0xFF, Inline, 0x1F)

// define enums
#define ENUM_X(name) TYPE_##name
typedef enum DataTypes {
    DATA_TYPES(ENUM_X, COMMA)
} DataTypes;

#define ENUM_FORMATS_X(name, dataType, min, max, readType, readTypeParameter) FORMAT_##name
typedef enum Formats {
    FORMATS(ENUM_FORMATS_X, COMMA)
} Formats;

typedef struct FormatInfo {
    char *name;
    DataTypes dataType;
    ReadTypes readType;
    uint32_t readTypeParameter;
} FormatInfo;

extern Formats FirstByteToFormat[256];
extern FormatInfo formatInfo[];

#endif // BUFFERS_H