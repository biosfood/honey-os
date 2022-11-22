#ifndef COLOR_H
#define COLOR_H

#define COLOR(foreground, background) (foreground | background << 4)

enum {
    black = 0,
    blue = 1,
    green = 2,
    cyan = 3,
    red = 4,
    magenta = 5,
    brown = 6,
    lightGray = 7,
    darkGray = 8,
    lightBlue = 9,
    lightGreen = 10,
    lightCyan = 11,
    lightRed = 12,
    lightMagenta = 13,
    yellow = 14,
    white = 15,
} color;

#endif
