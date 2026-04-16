[BITS 32]

section .data
align 16

; Simple GDT like in Linux and OSDev tutorials
gdt_start:
    ; Null descriptor
    dq 0x0000000000000000

gdt_code:
    ; Code segment: base=0, limit=0xFFFFF, access=0x9A, flags=0xCF
    dw 0xFFFF       ; Limit (bits 0-15)
    dw 0x0000       ; Base (bits 0-15)
    db 0x00         ; Base (bits 16-23)
    db 0x9A         ; Access byte
    db 0xCF         ; Flags + Limit (bits 16-19)
    db 0x00         ; Base (bits 24-31)

gdt_data:
    ; Data segment: base=0, limit=0xFFFFF, access=0x92, flags=0xCF
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92
    db 0xCF
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; Size
    dd gdt_start                 ; Address

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

section .text

global _gdt_init_simple

_gdt_init_simple:
    cli
    lgdt [gdt_descriptor]

    ; Reload data segments
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Far jump to reload CS
    jmp CODE_SEG:.reload_cs

.reload_cs:
    ret
