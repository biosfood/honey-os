#include <cursor.h>
#include <hlib.h>

void setCursorOffset(uint16_t offset) {
    ioOut(0x3D4, 0x0F, 1);
    ioOut(0x3D5, (uint8_t)(offset & 0xFF), 1);
    ioOut(0x3D4, 0x0E, 1);
    ioOut(0x3D5, (uint8_t)((offset >> 8) & 0xFF), 1);
}
