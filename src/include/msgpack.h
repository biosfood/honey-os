#ifndef HLIB_BUFFERS_H
#define HLIB_BUFFERS_H

#include <stdint.h>
#include "../hlib/malloc.h"

#define COMMA ,

#define EMPTY(...)
#define DEFER(...) __VA_ARGS__ EMPTY()
#define OBSTRUCT(...) __VA_ARGS__ DEFER(EMPTY)()

#define EXPAND3(...) __VA_ARGS__
#define EXPAND2(...) EXPAND3(EXPAND3(EXPAND3(EXPAND3(__VA_ARGS__))))
#define EXPAND1(...) EXPAND2(EXPAND2(EXPAND2(EXPAND2(__VA_ARGS__))))
#define EXPAND(...) EXPAND1(EXPAND1(EXPAND1(EXPAND1(__VA_ARGS__))))

#define STRING_Length(x) msgPackStringLength(strlen(x))

#define ONE(...) 1

#define ARRAY_Length_id() ARRAY_Length
#define _INTEGER_LENGTH(x, type, ...) msgPackIntegerLength(x, type)
#define INTEGER_Length(x, ...) _INTEGER_LENGTH(x, ##__VA_ARGS__ , Unsigned)
#define INTEGER_Length_id() INTEGER_Length
#define STRING_Length_id() STRING_Length
#define MAP_Length_id() MAP_Length

#define LIST_Length(list, type, name, contents) msgPackArrayLength(listCount(list)) + ({ uint32_t len = 0; foreach (list, type, name, { len += contents(LENGTH, +); }) len; })
#define LIST_Length_id() LIST_Length

#define ARRAY_Length(contents) msgPackArrayLength(contents(ONE, +)) + contents(LENGTH, +)
#define MAP_Length(contents) msgPackMapLength(allocationData, contents(ONE, +)) + contents(LENGTH, +)

#define LENGTH(type, ...) DEFER(type##_Length_id)()(__VA_ARGS__)

#define _INTEGER_WRITE(x, type, ...) buffer = msgPackIntegerWrite(buffer, x, type);
#define INTEGER_Write(x, ...) _INTEGER_WRITE(x, ##__VA_ARGS__ , Unsigned)
#define STRING_Write(x) buffer = msgPackStringWrite(buffer, x);

#define INTEGER_Write_id() INTEGER_Write
#define ARRAY_Write_id() ARRAY_Write
#define STRING_Write_id() STRING_Write
#define MAP_Write_id() MAP_Write

#define LIST_Write_id() LIST_Write
#define LIST_Write(list, type, name, contents) buffer = msgPackArrayWrite(buffer, listCount(list)); foreach (list, type, name, { contents(WRITE, NOTHING); })

#define ARRAY_Write(contents) buffer = msgPackArrayWrite(buffer, contents(ONE, +)); contents(WRITE, NOTHING)
#define MAP_Write(contents) buffer = msgPackMapWrite(buffer, contents(ONE, +)); contents(WRITE, NOTHING)

#define WRITE(type, ...) DEFER(type##_Write_id)()(__VA_ARGS__)

#define CONTENTS contents (LENGTH, +)

#define CREATE(name, definition) \
    uint32_t name##Length = EXPAND(definition(LENGTH)); \
    void *name = malloc(name##Length); \
    { \
        void *buffer = name; \
        EXPAND(definition(WRITE)) \
    }

// helper macros to make accessing data in a buffer easier
// all of these check if the type is correct and throw an error if not
#define _AS_INT(data, catchError) \
    ({ \
        uint8_t *buffer = (uint8_t *) data; \
        uint8_t type = FirstByteToFormat[*buffer]; \
        if (formatInfo[type].dataType != TYPE_INTEGER) catchError \
        msgPackReadInt(allocationData, buffer); \
    })

#define _AS_UINT(data, catchError, catchNegative) \
    ({ \
        uint8_t *buffer = (uint8_t *) data; \
        uint8_t type = FirstByteToFormat[*buffer]; \
        if (formatInfo[type].dataType != TYPE_INTEGER) catchError \
        intmax_t asInt = msgPackReadInt(allocationData, data); \
        if (asInt < 0) catchNegative \
        (uint32_t) asInt; \
    })

#define _AS_STRING(data, catchError) \
    ({ \
        uint8_t *buffer = (uint8_t *) data; \
        uint8_t type = FirstByteToFormat[*buffer]; \
        if (formatInfo[type].dataType != TYPE_STRING) catchError \
        msgPackReadStr(allocationData, buffer); \
    })

#define AS_INT(data, retval, ...) \
    _AS_INT(data, ##__VA_ARGS__, { printf("AS_INT: cannot convert '" #data "' to an integer\n"); return retval; })

#define AS_UINT(data, retval, ...) \
    _AS_UINT(data, ##__VA_ARGS__, { printf("AS_UINT: cannot convert '" #data "' to an integer\n"); return retval; }, { printf("AS_UINT: '" #data "' is negative!\n"); asInt = 0; })

#define AS_STRING(data, retval, ...) \
    _AS_STRING(data, ##__VA_ARGS__, { printf("AS_STRING: cannot convert '" #data "' to a string\n"); return retval; })

#define ARRAY_LOOP(data, retval, elementName, action) \
    { \
        uint8_t *buffer = (uint8_t *) data; \
        uint8_t type = FirstByteToFormat[*buffer]; \
        if (formatInfo[type].dataType != TYPE_ARRAY) { \
            printf("ARRAY_LOOP: cannot convert '" #data "' to an array\n"); \
            return retval; \
        } \
        void *elementName; \
        uint32_t maxElement = msgPackReadArraySize(allocationData, data, &elementName); \
        elementName = data + 1; \
        for (uint32_t i = 0; i < maxElement; i++) { \
            (action); \
            elementName = msgPackSeek(allocationData, elementName); \
        } \
    }

#define GET_FROM_INT(data, value, retval) \
    ({ \
        uint8_t *buffer = (uint8_t *)data; \
        uint8_t type = FirstByteToFormat[*buffer]; \
        if (formatInfo[type].dataType != TYPE_MAP) { \
            printf("GET_FROM_INT: cannot convert '" #data "' to a map, got %s\n", formatInfo[type].name); \
            return retval; \
        } \
        msgPackMapGetFromInt(allocationData, data, value); \
    })

#define GET_FROM_STRING(data, value, retval) \
    ({ \
        uint8_t *buffer = (uint8_t *)data; \
        uint8_t type = FirstByteToFormat[*buffer]; \
        if (formatInfo[type].dataType != TYPE_MAP) { \
            printf("GET_FROM_STRING: cannot convert '" #data "' to a map, got %s\n", formatInfo[type].name); \
            return retval; \
        } \
        msgPackMapGetFromString(allocationData, data, value); \
    })


typedef enum ReadTypes {
    Inline,
    InlineLength,
    FixedLength,
    ReadLength,
    ElementsInline,
    ReadElements,
} ReadTypes;

typedef enum IntegerType {
    Signed,
    Unsigned
} IntegerType;

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
    X(POSITIVE_FIXINT, INTEGER, 0x00, 0x7F, Inline, -7) S \
    X(FIXMAP, MAP, 0x80, 0x8F, ElementsInline, -4) S \
    X(FIXARRAY, ARRAY, 0x90, 0x9F, ElementsInline, -4) S \
    X(FIXSTR, STRING, 0xA0, 0xBF, InlineLength, -5) S \
    X(NIL, NIL, 0xC0, 0xC0, Inline, 0x00) S \
    X(UNUSED, UNUSED, 0xC1, 0xC1, Inline, 0x00) S \
    X(FIXBOOL, BOOLEAN, 0xC2, 0xC3, Inline, -1) S \
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
    X(ARRAY16, ARRAY, 0xDC, 0xDC, ReadElements, 2) S \
    X(ARRAY32, ARRAY, 0xDD, 0xDD, ReadElements, 4) S \
    X(MAP16, MAP, 0xDE, 0xDE, ReadElements, 2) S \
    X(MAP32, MAP, 0xDF, 0xDF, ReadElements, 4) S \
    X(NEGATIVE_FIXINT, INTEGER, 0xE0, 0xFF, Inline, -5)

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
    uint8_t min, max;
} FormatInfo;

extern Formats FirstByteToFormat[256];
extern FormatInfo formatInfo[];

typedef char * STRING;
typedef uint32_t INT;

extern Formats FirstByteToFormat[256];
extern FormatInfo formatInfo[];

#define _GET(type, name, retval, ...) type name = AS_##type(GET_FROM_STRING(data, #name, retval), retval);
#define GET(type, name, ...) _GET(type, name, ##__VA_ARGS__, -1)

// query functions
extern void *msgPackMapGetFromString(AllocationData allocationData, void *data, char *searchValue);
extern void *msgPackMapGEtFromInt(AllocationData allocationData, void *data, intmax_t searchValue);

// length functions
extern uint32_t msgPackMapLength(AllocationData allocationData, uint32_t elementCount);
extern uint32_t msgPackArrayLength(uint32_t elementCount);
extern uint32_t msgPackIntegerLength(int32_t value, IntegerType integerType);
extern uint32_t msgPackStringLength(uint32_t strlength);

// write functions
extern void *msgPackArrayWrite(void *buffer, uint32_t elementCount);
extern void *msgPackMapWrite(void *buffer, uint32_t elementCount);
extern void *msgPackIntegerWrite(void *buffer, int32_t x, IntegerType type);
extern void *msgPackStringWrite(void *buffer, char *string);

// read functions
extern uintmax_t msgPackReadArraySize(AllocationData allocationData, void *data, void **firstElement);
extern uintmax_t msgPackReadMapSize(AllocationData allocationData, void *data, void **firstElement);
extern intmax_t msgPackReadInt(AllocationData allocationData, void *data);
extern uintmax_t msgPackReadUint(AllocationData allocationData, void *data);
extern char *msgPackReadStr(AllocationData allocationData, void *data);

// util / debugging functions
extern void *_msgPackDump(AllocationData allocationData, uint8_t *data, uint32_t indent);
#define msgPackDump(data) _msgPackDump(allocationData, data, 0)

extern void initializeFlatBuffers();
extern void *msgPackSeek(AllocationData allocationData, void *data);

#endif