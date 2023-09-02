OUTPUT_ARCH(i386)

SECTIONS {
    . = 0xFF000000;

    .hlib : {
        *(.text)
        *(.rodata)
        *(.data)
        *(.bss)
    }
}
