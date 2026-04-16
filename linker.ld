ENTRY(_start)

SECTIONS {
    . = 0x1000;

    .text : {
        *(.text)
    }

    .rodata ALIGN(16) : {
        *(.rodata)
    }

    .data ALIGN(16) : {
        *(.data)
    }

    .bss ALIGN(16) : {
        *(COMMON)
        *(.bss)
    }

    _kernel_end = .;
}
