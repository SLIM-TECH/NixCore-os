[BITS 16]
[ORG 0x7C00]

KERNEL_OFFSET equ 0x1000
KERNEL_SECTORS equ 150

start:
    cli
    mov [boot_drive], dl
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov ah, 0x00
    mov al, 0x03
    int 0x10

    mov si, boot_msg
    call print_string

    mov si, a20_msg
    call print_string
    call enable_a20

    call load_kernel

    mov si, pm_msg
    call print_string

    cli                    ; Disable interrupts before protected mode
    call switch_to_pm

enable_a20:
    pusha
    call .wait_input
    mov al, 0xAD
    out 0x64, al
    call .wait_input
    mov al, 0xD0
    out 0x64, al
    call .wait_output
    in al, 0x60
    push ax
    call .wait_input
    mov al, 0xD1
    out 0x64, al
    call .wait_input
    pop ax
    or al, 2
    out 0x60, al
    call .wait_input
    mov al, 0xAE
    out 0x64, al
    call .wait_input
    popa
    ret
.wait_input:
    in al, 0x64
    test al, 2
    jnz .wait_input
    ret
.wait_output:
    in al, 0x64
    test al, 1
    jz .wait_output
    ret

print_string:
    pusha
.loop:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp .loop
.done:
    popa
    ret

load_kernel:
    mov si, loading_msg
    call print_string

    mov ax, KERNEL_OFFSET
    mov es, ax
    xor bx, bx

    mov si, dap
    mov word [dap_sector], 1
    mov word [dap_count], 150

    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13
    jc .try_chs

    mov si, kernel_loaded_msg
    call print_string
    ret

.try_chs:
    mov ax, KERNEL_OFFSET
    mov es, ax
    xor bx, bx

    mov cx, 2
    mov byte [head_num], 0

.chs_loop:
    mov ah, 0x02
    mov al, 1
    mov ch, 0
    mov dh, [head_num]
    mov dl, [boot_drive]
    int 0x13
    jc .error

    add bx, 512
    inc cl

    cmp cl, 19
    jl .chs_loop

    mov cl, 1
    inc byte [head_num]
    cmp byte [head_num], 16
    jl .chs_loop

    mov si, kernel_loaded_msg
    call print_string
    ret

.error:
    mov si, disk_error_msg
    call print_string
    cli
    hlt
    jmp $

dap:
    db 0x10
    db 0
dap_count:
    dw 0
    dw 0
    dw KERNEL_OFFSET
dap_sector:
    dd 1
    dd 0

switch_to_pm:
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp CODE_SEG:init_pm

[BITS 32]
init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov esp, 0x90000
    mov ebp, 0x90000

    mov dword [0xB8000], 0x0F4F0F4B

    jmp CODE_SEG:KERNEL_OFFSET

gdt_start:
    dq 0x0000000000000000
gdt_code:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00
gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

[BITS 16]
boot_drive db 0
sector_num dw 0
head_num db 0

boot_msg db 'NixCore boot...',13,10,0
a20_msg db 'A20...',13,10,0
loading_msg db 'Loading kernel...',13,10,0
kernel_loaded_msg db 'Kernel OK',13,10,0
pm_msg db 'PM switch...',13,10,0
disk_error_msg db 'Disk error!',13,10,0

times 510-($-$$) db 0
dw 0xAA55
