#ifndef PS2_H
#define PS2_H

#include <hlib.h>

#define COMMAND 0x64
#define DATA 0x60

#define PORTS 2

typedef union {
    uint8_t byte;
    struct {
        uint8_t outputBufferStatus:1;
        uint8_t inputBufferStatus: 1;
        uint8_t sytemFlag: 1;
        uint8_t commandData: 1;
        uint8_t unused: 2;
        uint8_t timeoutError: 1;
        uint8_t parityError: 1;
    } __attribute__((packed)) data;
    } Status;

typedef union {
    uint8_t byte;
    struct {
        uint8_t firstInterruptEnabled:1;
        uint8_t secondInterruptEnabled: 1;
        uint8_t sytemFlag: 1;
        uint8_t zero1: 1;
        uint8_t firstPortClock: 1;
        uint8_t secondPortClock: 1;
        uint8_t firstPortTranslation: 1;
        uint8_t zero2: 1;
    } __attribute__((packed)) data;
    } Configuration;

typedef enum {
    UnknownPS2Device,
    ATKeyboard,
    StandardPS2Mouse,
    StandardPS2MouseWithScrollWheel,
    MouseWith5Buttons,
    IBMThinkpadKeyboard,
    NCDN97Keyboard,
    Keyboard122Keys,
    JapaneseGKeyboard,
    JapanesePKeyboard,
    JapaneseAKeyboard,
    QEMUKeyboard,
} DeviceType;


#endif