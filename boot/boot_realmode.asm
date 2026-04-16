[BITS 16]
[ORG 0x7C00]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    ; Clear screen
    mov ah, 0x00
    mov al, 0x03
    int 0x10

    ; Print boot message
    mov si, boot_msg
    call print_string

    ; Print system info
    mov si, realmode_msg
    call print_string

    ; Simple shell loop
shell_loop:
    mov si, prompt
    call print_string

    ; Wait for key
    xor ah, ah
    int 0x16

    ; Echo character
    mov ah, 0x0E
    int 0x10

    ; Check for commands
    cmp al, 'h'
    je cmd_help
    cmp al, 'r'
    je cmd_reboot
    cmp al, 13  ; Enter
    je newline

    jmp shell_loop

cmd_help:
    call newline
    mov si, help_msg
    call print_string
    jmp shell_loop

cmd_reboot:
    call newline
    mov si, reboot_msg
    call print_string
    jmp 0xFFFF:0x0000

newline:
    mov ah, 0x0E
    mov al, 13
    int 0x10
    mov al, 10
    int 0x10
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

boot_msg db 'NixCore Real Mode Fallback',13,10,0
realmode_msg db 'Running in 16-bit Real Mode (Safe Mode)',13,10,13,10,0
prompt db '> ',0
help_msg db 13,10,'Commands: h=help, r=reboot',13,10,0
reboot_msg db 'Rebooting...',13,10,0

times 510-($-$$) db 0
dw 0xAA55
