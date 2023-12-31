#include <hlib.h>
#include <stdint.h>

int32_t main() {
    initializeFlatBuffers();
    loadFromInitrd("ioManager");
    printf("HONEY-OS - made by Lukas Eisenhauer\n");
    printf("finished loading all the essential modules\n");
    loadFromInitrd("lspci");
    loadFromInitrd("pic");
    loadFromInitrd("pit");
    loadFromInitrd("keyboard");
    loadFromInitrd("mouse");
    loadFromInitrd("usb");
    uint32_t id = loadFromInitrdUninitialized("shell");
    requestName("ioManager", "setForeground", id, 0);
    request(id, 0, 0, 0);
}
