#include <hlib.h>
#include <stdint.h>
#include "ps2.h"

const char* deviceTypeNames[] = {
    "Unknown PS2 Device",
    "AT Keyboard",
    "Standard PS2 Mouse",
    "Standard PS2 Mouse with Scroll Wheel",
    "Mouse with 5 Buttons",
    "IBM Thinkpad Keyboard",
    "NCD N97 Keyboard",
    "Keyboard 122 Keys",
    "Japanese G Keyboard",
    "Japanese P Keyboard",
    "Japanese A Keyboard",
    "QEMU keyboard",
};


Status readStatus() {
    Status result = {.byte = ioIn(COMMAND, 1)};
    return result;
}

uint8_t waitForRead() {
    uint32_t timeout = 100000;
    while (!readStatus().data.outputBufferStatus) {
        if (--timeout == 0) {
            printf("PS/2 read timeout\n");
            return 1;
        }
    }
    return 0;
}

void waitForWrite() {
    uint32_t timeout = 100000;
    while (readStatus().data.inputBufferStatus) {
        if (--timeout == 0) {
            printf("PS/2 write timeout\n");
            return;
        }
    }
}

uint8_t read(uint8_t device) {
    if (waitForRead() == 1) {
        return 0xFF;
    }
    return ioIn(DATA, 1);
}

void writeController(uint8_t command) {
    waitForWrite();
    ioOut(COMMAND, command, 1);
}

void writeDevice(uint8_t device, uint8_t data) {
    if (device == 1) {
        writeController(0xD4);
    }
    waitForWrite();
    ioOut(DATA, data, 1);
}

Configuration readConfiguration() {
    writeController(0x20);
    Configuration result = {.byte = read(0)};
    return result;
}

void writeConfiguration(uint8_t data) {
    writeController(0x60);
    writeDevice(0, data);
}

uint8_t checkPort(uint8_t device) {
    return read(0);
}

void flushOutputBuffer() {
    while (readStatus().data.outputBufferStatus) {
        ioIn(DATA, 1);
    }
}

DeviceType getDeviceType(uint8_t device) {
    // disable scanning
    writeDevice(device, 0xF5);
    // wait for ACK
    while (read(device) != 0xFA);
    // send identify command
    writeDevice(device, 0xF2);
    // wait for ACK
    while (read(device) != 0xFA);
    // read data
    uint8_t result1 = read(device);
    switch (result1) {
        case 0xFF: return ATKeyboard;
        case 0x00: return StandardPS2Mouse;
        case 0x03: return StandardPS2MouseWithScrollWheel;
        case 0x04: return MouseWith5Buttons;
    }
    if (result1 != 0xAB && result1 != 0xAC) {
        return UnknownPS2Device;
    }
    uint8_t result2 = read(device);
    switch (result2) {
        case 0x83: return QEMUKeyboard;
        case 0x84: return IBMThinkpadKeyboard;
        case 0x85: return NCDN97Keyboard;
        case 0x86: return Keyboard122Keys;
        case 0x90: return JapaneseGKeyboard;
        case 0x91: return JapanesePKeyboard;
        case 0x92: return JapaneseAKeyboard;
    }
    return UnknownPS2Device;
}

void initDevice(uint8_t device) {
    flushOutputBuffer();
    // test port
    writeController(device == 0 ? 0xAB : 0xA9);
    uint8_t deviceHealth = read(0);
    if (deviceHealth) {
        printf("device %i interface test failed: %i\n", device, deviceHealth);
        return;
    }
    // send enable command
    writeController(device == 0 ? 0xAE : 0xA8);
    read(device);
    flushOutputBuffer();
    // send reset command to device
    writeDevice(device, 0xFF);
    uint8_t ack = read(device);
    deviceHealth = read(device);
    if (ack != 0xFA || deviceHealth != 0xAA) {
        printf("device %i reset failed with response %x, %x\n", device, ack, deviceHealth);
        return;
    }

    DeviceType deviceType = getDeviceType(device);
    printf("device %i has type '%s'\n", device, deviceTypeNames[deviceType]);
}

int32_t main() {
    createFunction("read", (void *)read);

    // disable all devices
    writeController(0xAD);
    writeController(0xA7);
    
    flushOutputBuffer();

    // configure controller
    Configuration config = readConfiguration();
    config.data.firstInterruptEnabled = 0;
    config.data.secondInterruptEnabled = 0;
    config.data.firstPortTranslation = 0;
    writeConfiguration(config.byte); 
    config = readConfiguration();

    // perform self test
    writeController(0xAA);
    uint8_t result = read(0);
    if (result != 0x55) {
        printf("controller self test failed: %x\n", result);
        return -1;
    }

    initDevice(0);

    if (config.data.secondPortClock) {
        // enable second device
        writeController(0xA8);
        config = readConfiguration();
        if (config.data.secondPortClock) {
            printf("controller has support for 2 ports, but the second port isn't in use\n");
        } else {
            printf("controller has support for 2 ports, and the second port is in use\n");
            // shut down the second port again
            writeController(0xA7);

            initDevice(1);
        }
    }

    loadFromInitrd("ps2kb");
    return 0;
}