#include <storage.h>
#include <hlib.h>

void setup(uint32_t in, uint32_t out, uint32_t serviceName, uint32_t serviceId) {
    StorageDevice *device = malloc(sizeof(StorageDevice));
    device->serviceId = serviceId;
    device->getType = getFunction(serviceId, "type");
    printf("in: %x, out: %x\n", in, out);
}

int32_t main() {
    createFunction("setup", (void *) setup);
}