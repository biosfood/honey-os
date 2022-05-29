#include <memory.h>
#include <multiboot.h>
#include <util.h>

void *findInitrd(MultibootInformation *information, uint32_t *fileSize) {
    MultibootInformationTag *tag = (void *)information->tags;
    while (tag->size) {
        if (tag->type == MultibootModuleTagType &&
            stringEquals(((MultibootModuleTag *)tag)->commandLineParameters,
                         "initrd")) {
            MultibootModuleTag *module = (void *)tag;
            uint32_t moduleSize = module->moduleEnd - module->moduleStart;
            void *moduleLocation = kernelMapMultiplePhysicalPages(
                PTR(module->moduleStart), PAGE_COUNT(moduleSize));
            *fileSize = moduleSize;
            return moduleLocation;
        }
        tag = ((void *)tag) + ((tag->size + 0x07) & ~0x7);
    }
    return NULL;
}
