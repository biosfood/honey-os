OUTPUT_ARCH(i386)
ENTRY(main)

INPUT( ../../build/hlib.o)

SECTIONS {
    . = 0x80000000;
    .text : {
        *(.text)
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
         *(.data)
    }

    .bss : {
        *(.bss)
        . = ALIGN(4);
        allocationData = .;
        . += 4 * 12;
    }

    . = 0xFF000000;
    .hlib (NOLOAD) : {
        *(.hlib)
    }
}
