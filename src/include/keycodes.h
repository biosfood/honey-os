#ifndef KEYCODES_H
#define KEYCODES_H

#include <stdint.h>

// modeled after
// https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf,
// page 53, section 10, Table 12: Keyboard / Keypad page
// made to densely fill an array
#define KEYS(f)                           \
    f(KEY_NONE,            0, "",  "", MODIFIERS_SHIFT) \
    f(KEY_ERROR,           1, "",  "", MODIFIERS_SHIFT) \
    f(KEY_POST_FAIL,       2, "",  "", MODIFIERS_SHIFT) \
    f(KEY_ERROR_UNDEFINED, 3, "",  "", MODIFIERS_SHIFT) \
    f(KEY_A         ,   4, "a", "A", MODIFIERS_SHIFT) \
    f(KEY_B         ,   5, "b", "B", MODIFIERS_SHIFT) \
    f(KEY_C         ,   6, "c", "C", MODIFIERS_SHIFT) \
    f(KEY_D         ,   7, "d", "D", MODIFIERS_SHIFT) \
    f(KEY_E         ,   8, "e", "E", MODIFIERS_SHIFT) \
    f(KEY_F         ,   9, "f", "F", MODIFIERS_SHIFT) \
    f(KEY_G         ,  10, "g", "G", MODIFIERS_SHIFT) \
    f(KEY_H         ,  11, "h", "H", MODIFIERS_SHIFT) \
    f(KEY_I         ,  12, "i", "I", MODIFIERS_SHIFT) \
    f(KEY_J         ,  13, "j", "J", MODIFIERS_SHIFT) \
    f(KEY_K         ,  14, "k", "K", MODIFIERS_SHIFT) \
    f(KEY_L         ,  15, "l", "L", MODIFIERS_SHIFT) \
    f(KEY_M         ,  16, "m", "M", MODIFIERS_SHIFT) \
    f(KEY_N         ,  17, "n", "N", MODIFIERS_SHIFT) \
    f(KEY_O         ,  18, "o", "O", MODIFIERS_SHIFT) \
    f(KEY_P         ,  19, "p", "P", MODIFIERS_SHIFT) \
    f(KEY_Q         ,  20, "q", "Q", MODIFIERS_SHIFT) \
    f(KEY_R         ,  21, "r", "R", MODIFIERS_SHIFT) \
    f(KEY_S         ,  22, "s", "S", MODIFIERS_SHIFT) \
    f(KEY_T         ,  23, "t", "T", MODIFIERS_SHIFT) \
    f(KEY_U         ,  24, "u", "U", MODIFIERS_SHIFT) \
    f(KEY_V         ,  25, "v", "V", MODIFIERS_SHIFT) \
    f(KEY_W         ,  26, "w", "W", MODIFIERS_SHIFT) \
    f(KEY_X         ,  27, "x", "X", MODIFIERS_SHIFT) \
    f(KEY_Y         ,  28, "y", "Y", MODIFIERS_SHIFT) \
    f(KEY_Z         ,  29, "z", "Z", MODIFIERS_SHIFT) \
    f(KEY_1         ,  30, "1", "!", MODIFIERS_SHIFT) \
    f(KEY_2         ,  31, "2", "@", MODIFIERS_SHIFT) \
    f(KEY_3         ,  32, "3", "#", MODIFIERS_SHIFT) \
    f(KEY_4         ,  33, "4", "$", MODIFIERS_SHIFT) \
    f(KEY_5         ,  34, "5", "%", MODIFIERS_SHIFT) \
    f(KEY_6         ,  35, "6", "^", MODIFIERS_SHIFT) \
    f(KEY_7         ,  36, "7", "&", MODIFIERS_SHIFT) \
    f(KEY_8         ,  37, "8", "*", MODIFIERS_SHIFT) \
    f(KEY_9         ,  38, "9", "(", MODIFIERS_SHIFT) \
    f(KEY_0         ,  39, "0", ")", MODIFIERS_SHIFT)

#define KEY_ENUM(name, id, normal, modified, modifiers) name = id,

typedef enum {
    KEYS(KEY_ENUM)
} Key;

typedef struct {
    Key key;
    char *normal;
    char *modified;
    uint32_t *modifierKeys;
} KeyInfo;

#endif
