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
    for (uint32_t i = 0; i < 10; i++) {
        printf("restData: %x %x %x %x\n", response->restData[4*i], response->restData[4*i+1],
                                                response->restData[4*i+2], response->restData[4*i+3]);
    }
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
    return 0;
}

int32_t main() {
    createFunction("register", (void *)registerDevice);
    return 0;
}