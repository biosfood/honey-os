#include <hlib.h>
#include <scisi.h>

ListElement *devices = NULL;

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
    request(device->serviceId, device->outFunction, device->out, 0);
    request(device->serviceId, device->inFunction, device->in, 0);
    return 0;

}

int32_t main() {
    createFunction("register", (void *)registerDevice);
    return 0;
}