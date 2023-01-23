#include "tar.h"
#include <memory.h>
#include <util.h>

uint32_t readOctal(char *string) {
    uint32_t result = 0;
    while (*string) {
        result += *string - '0';
        result <<= 3;
        string++;
    }
    return result;
}

void *findTarFile(void *fileData, uint32_t tarFileSize, char *fileName) {
    void *currentPosition = fileData;
    while (currentPosition <= fileData + tarFileSize) {
        TarFileHeader *header = currentPosition;
        uint32_t fileSize = readOctal(header->fileSize);
        if (stringEquals(header->fileName, fileName)) {
            return currentPosition + 512;
        }
        currentPosition += 512;
    }
    return NULL;
}
