#include <hlib.h>
#include <buffers.h>

REQUEST(pciDump, "lspci", "dumpAll");

int32_t main() {
    uintptr_t dataPhysical = pciDump(1, 0);
    void *data = requestMemory(1, NULL, PTR(dataPhysical));
    printf("pci data:\n");
    msgPackDump(data);
}