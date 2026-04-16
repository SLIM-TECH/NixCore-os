[BITS 16]
[ORG 0x7C00]

; Simple bootloader based on Linux kernel approach
; Loads kernel and switches to protected mode CORRECTLY

KERNEL_OFFSET equ 0x1000
KERNEL_SECTORS equ 200

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Save boot drive
    mov [boot_drive], dl

    ; Load kernel from disk
    mov si, loading_msg
    call print

    call load_kernel

    ; Enable A20 line
    call enable_a20

    ; Print PM message
    mov si, pm_msg
    call print

    ; Switch to protected mode
    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Far jump to 32-bit code
    jmp CODE_SEG:protected_mode_start

; Load kernel using BIOS
load_kernel:
    ; BIOS can only read 127 sectors at once, so read in chunks
    mov bx, KERNEL_OFFSET
    mov cx, 2           ; Start from sector 2
    mov dh, 0           ; Head 0
    mov dl, [boot_drive]

    ; Read first 127 sectors
    mov ah, 0x02
    mov al, 127
    int 0x13
    jc disk_error

    ; Read remaining 73 sectors (200 - 127 = 73)
    add bx, 127 * 512   ; Move buffer pointer
    add cl, 127         ; Next sector (will overflow to next track)
    mov ah, 0x02
    mov al, 73
    int 0x13
    jc disk_error

    ; Print success message
    mov si, kernel_loaded_msg
    call print

    ret

disk_error:
    mov si, error_msg
    call print
    cli
    hlt

enable_a20:
    in al, 0x92
    or al, 2
    out 0x92, al
    ret

print:
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

; GDT - Simple and correct like Linux
gdt_start:
    dq 0x0000000000000000  ; Null descriptor

gdt_code:
    dw 0xFFFF    ; Limit low
    dw 0x0000    ; Base low
    db 0x00      ; Base middle
    db 10011010b ; Access: present, ring 0, code, executable, readable
    db 11001111b ; Flags: 4KB granularity, 32-bit
    db 0x00      ; Base high

gdt_data:
    dw 0xFFFF    ; Limit low
    dw 0x0000    ; Base low
    db 0x00      ; Base middle
    db 10010010b ; Access: present, ring 0, data, writable
    db 11001111b ; Flags: 4KB granularity, 32-bit
    db 0x00      ; Base high

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

[BITS 32]
protected_mode_start:
    ; Set up segments
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Set up stack
    mov esp, 0x90000
    mov ebp, esp

    ; Jump to kernel
    jmp CODE_SEG:KERNEL_OFFSET

[BITS 16]
boot_drive db 0
loading_msg db 'Loading NixCore...',13,10,0
kernel_loaded_msg db 'Kernel loaded OK',13,10,0
pm_msg db 'Entering Protected Mode...',13,10,0
error_msg db 'Disk error!',13,10,0

times 510-($-$$) db 0
dw 0xAA55
