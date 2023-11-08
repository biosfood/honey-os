#include <hlib.h>
#include <scisi.h>

ListElement *devices = NULL;

void doInquiry(ScisiDevice *device) {
    InquiryCommand *command = malloc(sizeof(InquiryCommand));
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
    
    request(device->serviceId, device->outFunction, device->out, U32(getPhysicalAddress(command)));
    request(device->serviceId, device->inFunction, device->in, U32(getPhysicalAddress(response)));
    
    
    printf("response: type: %x, removable: %i, version: %x, responseData: %x, additionalLength: %i\n",
            response->type, response->removable, response->version, response->responseData, response->additionalLength);
}

void readSize(ScisiDevice *device) {
    ReadCapacity10Command *command = malloc(sizeof(ReadCapacity10Command));
    command->size = sizeof(ReadCapacity10Command) - sizeof(uint32_t);
    command->operationCode = 0x25;
    command->control = 0;

    request(device->serviceId, device->outFunction, device->out, U32(getPhysicalAddress(command)));

    ReadCapacity10Response *response = malloc(sizeof(ReadCapacity10Response));
    response->size = sizeof(ReadCapacity10Response) - sizeof(uint32_t);
    request(device->serviceId, device->inFunction, device->in, U32(getPhysicalAddress(response)));
    device->blockSize = response->blockSize[0] << 24 | response->blockSize[1] << 16 | response->blockSize[2] << 8 | response->blockSize[3];
    uint32_t maxLba = response->lastLBA[0] << 24 | response->lastLBA[1] << 16 | response->lastLBA[2] << 8 | response->lastLBA[3];
    printf("max lba: %x, block size: %x\n", maxLba, device->blockSize);   
}

void *read(ScisiDevice *device, uint32_t address, uint16_t size) {
    if (size == 0) {
        return malloc(1);
    }
    Read10Command *command = malloc(sizeof(Read10Command));
    command->size = sizeof(Read10Command) - sizeof(uint32_t);
    command->operationCode = 0x28;
    command->lba[0] = (uint8_t) (address >> 24);
    command->lba[1] = (uint8_t) (address >> 16);
    command->lba[2] = (uint8_t) (address >> 8);
    command->lba[3] = (uint8_t) (address);
    uint32_t blockCount = (size-1) / device->blockSize + 1;
    command->transferLength[0] = (uint8_t) (blockCount >> 8);
    command->transferLength[1] = (uint8_t) (blockCount);

    command->control = 0;

    uint32_t *data = malloc(size + sizeof(uint32_t));
    *data = size;
    
    request(device->serviceId, device->outFunction, device->out, U32(getPhysicalAddress(command)));
    request(device->serviceId, device->inFunction, device->in, U32(getPhysicalAddress(data)));
    return data + 1;
}

int32_t registerDevice(uint32_t in, uint32_t out, uint32_t serviceName, uint32_t serviceId) {
    ScisiDevice *device = malloc(sizeof(ScisiDevice));
    device->serviceId = serviceId;
    device->id = listCount(devices);
    device->in = in;
    device->out = out;
    device->inFunction = getFunction(serviceId, "scisi_in");
    device->outFunction = getFunction(serviceId, "scisi_out");
    listAdd(&devices, device);
    printf("registering a new SCISI device (in: %x, out: %x)\n", in, out);
    doInquiry(device);
    readSize(device);
    uint8_t *buffer = read(device, 0, 512);
    for (uint32_t i = 0; i < 512 / 8; i++) {
        printf("%x %x %x %x %x %x %x %x",
                buffer[i * 8 + 0], buffer[i * 8 + 1], buffer[i * 8 + 2], buffer[i * 8 + 3],
                buffer[i * 8 + 4], buffer[i * 8 + 5], buffer[i * 8 + 6], buffer[i * 8 + 7]);
    }
    return 0;
}

int32_t main() {
    createFunction("register", (void *)registerDevice);
    return 0;
}