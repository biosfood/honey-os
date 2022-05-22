OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS {
    . = 1M;

    .boot : {
        *(.boot)
    }
    bootEnd = .;

    . +=                         0xFFB00000;
    .text : AT(ADDR(.text) -     0xFFB00000) {
        *(.text)
    }

    .rodata :AT (ADDR(.rodata) - 0xFFB00000) {
        *(.rodata)
    }

    .data :AT(ADDR(.data) -      0xFFB00000) {
         *(.data)
    }

    .bss :AT(ADDR(.bss) -        0xFFB00000) {
        *(.bss)
    }
}
