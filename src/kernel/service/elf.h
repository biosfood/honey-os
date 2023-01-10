#ifndef ELF_H
#define ELF_H

#include <stdint.h>

typedef struct {
    uint32_t magic;
    uint8_t is64BitFile;
    uint8_t endianness;
    uint8_t elfHeaderVersion;
    uint8_t osABI;
    uint32_t unused0, unused1;
    uint16_t flags;
    uint16_t instructionSet;
    uint32_t elfVersion;
    uint32_t entryPosition;
    uint32_t programHeaderTablePosition;
    uint32_t sectionHeaderTablePosition;
    uint32_t architectureFlags;
    uint16_t headerSize;
    uint16_t programHeaderEntrySize;
    uint16_t programHeaderEntryCount;
    uint16_t sectionHeaderEntrySize;
    uint16_t sectionHeaderEntryCount;
    uint16_t sectionNamesHeaderTableIndex;
} ElfHeader;

typedef struct {
    uint16_t segmentType;
    uint32_t dataOffset;
    uint32_t virtualAddress;
    uint32_t undefined;
    uint32_t segmentFileSize;
    uint32_t segmentMemorySize;
    uint32_t flags;
    uint32_t alignment;
} ProgramHeader;

typedef struct {
    uint32_t stringTableNameIndex;
    uint32_t type;
    uint32_t flags;
    uint32_t virtualAddress;
    uint32_t offset;
    uint32_t size;
    uint32_t link;
    uint32_t info;
    uint32_t alignment;
    uint32_t entrySize;
} SectionHeader;

typedef struct {
    uint32_t name;
    uint32_t value;
    uint32_t size;
    uint8_t info;
    uint8_t other;
    uint16_t sectionHeaderIndex;
} SymbolEntry;

#endif
