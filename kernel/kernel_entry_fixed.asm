[BITS 32]

section .text
global _start
extern __kernel_main

_start:
    ; We're already in protected mode with correct GDT from bootloader
    ; Just set up our own stack and go

    cli

    ; Disable PIC
    mov al, 0xFF
    out 0x21, al
    out 0xA1, al

    ; Set up kernel stack (don't touch bootloader stack yet)
    mov esp, kernel_stack_top
    mov ebp, esp

    ; Clear EFLAGS
    push 0
    popf

    ; Call kernel main
    call __kernel_main

    ; If kernel returns, halt
    cli
.hang:
    hlt
    jmp .hang

section .bss
align 16
kernel_stack_bottom:
    resb 16384  ; 16KB stack
kernel_stack_top:
