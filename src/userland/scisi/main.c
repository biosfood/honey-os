#include <hlib.h>
#include <scisi.h>

ListElement *devices = NULL;

void doInquiry(ScisiDevice *device) {
    InquiryCommand *command = malloc(sizeof(InquiryCommand));
    command->size = sizeof(InquiryCommand) - 2*sizeof(uint32_t);
    command->transferSize = 5;
    command->operationCode = 0x12;
    command->pageCode = 0;
    command->evpd = 0;
    command->allocationLengthHigh = 0;
    command->allocationLengthLow = 5;
    command->control = 0;
    command->size = sizeof(InquiryCommand) - sizeof(uint32_t);
    
    request(device->serviceId, device->outFunction, device->out, U32(getPhysicalAddress(command)));
    InquiryResponse *response = malloc(sizeof(InquiryResponse));
    response->size = sizeof(InquiryResponse) - sizeof(uint32_t);
    request(device->serviceId, device->inFunction, device->in, U32(getPhysicalAddress(response)));
    command->allocationLengthLow = 5 + response->additionalLength;
    command->transferSize = command->allocationLengthLow;
    
    request(device->serviceId, device->outFunction, device->out, U32(getPhysicalAddress(command)));
    request(device->serviceId, device->inFunction, device->in, U32(getPhysicalAddress(response)));
    free(command);
    free(response);
}

void readSize(ScisiDevice *device) {
    ReadCapacity10Command *command = malloc(sizeof(ReadCapacity10Command));
    command->size = sizeof(ReadCapacity10Command) - 2*sizeof(uint32_t);
    command->transferSize = sizeof(ReadCapacity10Response) - sizeof(uint32_t);
    command->operationCode = 0x25;
    command->control = 0;

    request(device->serviceId, device->outFunction, device->out, U32(getPhysicalAddress(command)));

    ReadCapacity10Response *response = malloc(sizeof(ReadCapacity10Response));
    response->size = sizeof(ReadCapacity10Response) - sizeof(uint32_t);
    request(device->serviceId, device->inFunction, device->in, U32(getPhysicalAddress(response)));
    device->blockSize = response->blockSize[0] << 24 | response->blockSize[1] << 16 | response->blockSize[2] << 8 | response->blockSize[3];
    uint32_t maxLba = response->lastLBA[0] << 24 | response->lastLBA[1] << 16 | response->lastLBA[2] << 8 | response->lastLBA[3];
    printf("max block address: %x, block size: %x, capacity: %i = 0x%x\n", maxLba, device->blockSize, maxLba * device->blockSize, maxLba * device->blockSize);   
    free(command);
    free(response);
}

void *read(ScisiDevice *device, uint32_t address, uint16_t size) {
    if (size == 0) {
        return malloc(1);
    }
    uint32_t blockCount = (size-1) / device->blockSize + 1;
    Read10Command *command = malloc(sizeof(Read10Command));
    command->size = sizeof(Read10Command) - 2*sizeof(uint32_t);
    command->transferSize = size;
    command->operationCode = 0x28;
    command->lba[0] = (uint8_t) (address >> 24);
    command->lba[1] = (uint8_t) (address >> 16);
    command->lba[2] = (uint8_t) (address >> 8);
    command->lba[3] = (uint8_t) (address);
    command->transferLength[0] = (uint8_t) (blockCount >> 8);
    command->transferLength[1] = (uint8_t) (blockCount);

    command->control = 0;

    uint32_t *data = malloc(size + sizeof(uint32_t));
    *data = size;
    
    request(device->serviceId, device->outFunction, device->out, U32(getPhysicalAddress(command)));
    request(device->serviceId, device->inFunction, device->in, U32(getPhysicalAddress(data)));
    free(command);
    return data + 1; // todo: ensure free will work on this
}

void *doRead(uint32_t deviceSize, uint32_t blockId) {
    uint32_t deviceId = deviceSize & 0xFFFF;
    ScisiDevice *device = listGet(devices, deviceId);
    void *result = read(device, blockId, deviceSize >> 16);
    printf("reading %i bytes from position %i from device %i: %x\n", deviceSize >> 16, blockId, deviceId, getPhysicalAddress(result));
    return getPhysicalAddress(result);
}

REQUEST(registerMBR, "mbr", "register");

int32_t registerDevice(uint32_t in, uint32_t out, uint32_t serviceName, uint32_t serviceId) {
    ScisiDevice *device = malloc(sizeof(ScisiDevice));
    device->serviceId = serviceId;
    device->id = listCount(devices);
    device->in = in;
    device->out = out;
    device->inFunction = getFunction(serviceId, "scisi_in");
    device->outFunction = getFunction(serviceId, "scisi_out");
    device->id = listCount(devices);
    listAdd(&devices, device);
    printf("registering a new SCISI device (in port: %x, out port: %x)\n", in, out);
    doInquiry(device);
    readSize(device);
    uint8_t *buffer = read(device, 0, 512);
    if (buffer[510] == 0x55 && buffer[511] == 0xAA) {
        printf("device %i is bootable!\n", device->id);
    }
    free(buffer - 4);
    registerMBR(device->id, 0);
    return 0;
}

int32_t main() {
    createFunction("register", (void *)registerDevice);
    createFunction("mbr_read", (void *)doRead);
    return 0;
}