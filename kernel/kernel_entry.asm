[BITS 32]

section .bss
align 16
stack_bottom:
resb 16384
stack_top:

section .text
global _start
extern __kernel_main

_start:
    cli

    ; Disable PIC completely
    mov al, 0xFF
    out 0x21, al    ; Mask all IRQs on master PIC
    out 0xA1, al    ; Mask all IRQs on slave PIC

    mov esp, stack_top
    push 0
    popf
    cli

    ; Use absolute address to avoid linker relocation bug
    mov eax, __kernel_main
    call eax

    cli
.hang:
    hlt
    jmp .hang
