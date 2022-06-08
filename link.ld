OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS {
    . = 1M;

    .boot : {
        *(.boot)
    }

    . +=                                           0xFFB00000;
    .text : AT(ADDR(.text) -                       0xFFB00000) {
        *(.text)
    }

    . = ALIGN(4k);
    functionsStart = .;
    .sharedFunctions : AT(ADDR(.sharedFunctions) - 0xFFB00000) {
        *(.sharedFunctions)
    }
    . = ALIGN(4k);

    . = ALIGN(4k);
    .rodata :AT (ADDR(.rodata) - 0xFFB00000) {
        *(.rodata)
    }

    .data :AT(ADDR(.data) -      0xFFB00000) {
         *(.data)
    }

    .bss :AT(ADDR(.bss) -        0xFFB00000) {
        *(.bss)
        . = ALIGN(4M);
    }

    . = 0x500000;
    .kernelReserve :AT(0x500000) {
        . += 0x400000;
    }
}
