#ifndef TAR_H
#define TAR_H

typedef struct {
    char fileName[100];
    char fileMode[8];
    char ownerUID[8];
    char groupUID[8];
    char fileSize[12];
    char lastModification[12];
    char checksum[8];
    char fileType;
    char linkTarget[100];
    char ustar[6];
    char ustarVersion[2];
    char ownerUserName[32];
    char ownerGroupName[32];
    char deviceMajor[8];
    char deviceMinor[8];
    char filenamePrefix[155];
} TarFileHeader;

#endif
