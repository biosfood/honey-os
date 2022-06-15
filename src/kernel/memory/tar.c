#include "tar.h"
#include <memory.h>
#include <util.h>

#define SECTOR_COUNT(size) (size ? (size - 1) / 512 + 1 : 0)

uint32_t readOctal(char *string) {
    uint32_t result = 0;
    while (*string) {
        result += *string - '0';
        result <<= 3;
        string++;
    }
    return result;
}

uint32_t i = 0;

void *findTarFile(void *fileData, uint32_t tarFileSize, char *fileName) {
    void *currentPosition = fileData;
    i++;
    while (currentPosition <= fileData + tarFileSize) {
        TarFileHeader *header = currentPosition;
        uint32_t fileSize = readOctal(header->fileSize);
        if (stringEquals(header->fileName, fileName)) {
            return currentPosition + 512;
        }
        // fixme!
        currentPosition += 512;
    }
    return NULL;
}
