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

uint64_t getSize(ScisiDevice *device) {
    ReadCapacity10Command *command = malloc(sizeof(ReadCapacity10Command));
    command->size = sizeof(ReadCapacity10Command) - sizeof(uint32_t);
    command->operationCode = 0x25;
    command->control = 0;

    request(device->serviceId, device->outFunction, device->out, U32(getPhysicalAddress(command)));

    ReadCapacity10Response *response = malloc(sizeof(ReadCapacity10Response));
    response->size = sizeof(ReadCapacity10Response) - sizeof(uint32_t);
    request(device->serviceId, device->inFunction, device->in, U32(getPhysicalAddress(response)));
    uint32_t blockSize = response->blockSize[0] << 24 | response->blockSize[1] << 16 | response->blockSize[2] << 8 | response->blockSize[3];
    uint32_t maxLba = response->lastLBA[0] << 24 | response->lastLBA[1] << 16 | response->lastLBA[2] << 8 | response->lastLBA[3];
    printf("max lba: %x, block size: %x\n", maxLba, blockSize);   
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
    uint64_t size = getSize(device);
    return 0;
}

int32_t main() {
    createFunction("register", (void *)registerDevice);
    return 0;
}