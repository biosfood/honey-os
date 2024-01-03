#include <hlib.h>

REQUEST(pciDump, "lspci", "dumpAll");

uint32_t showDevice(void *data) {
    GET(STRING, className);
    GET(INT, class);
    GET(INT, id);
    GET(INT, subclass);
    GET(INT, programmingInterface);

    printf("id: %i, class: \"%s\" (%i) / %i / %i\n", id, className, class, subclass, programmingInterface);

    free(className);
    return 0;
}

int32_t main() {
    uintptr_t dataPhysical = pciDump(1, 0);
    void *data = requestMemory(1, NULL, PTR(dataPhysical));
    printf("pci data:\n");
    ARRAY_LOOP(data, 0, device, showDevice(device));
}