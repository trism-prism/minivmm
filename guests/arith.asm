%include "boot.inc"
    mov ax, 0x1234
    add ax, 0x4321              ; 0x5555
    call hex
    mov ax, 7
    mov cx, 6
    mul cx                      ; 0x002A
    call hex
    mov ax, 100
    xor dx, dx
    mov cx, 7
    div cx                      ; quotient 0x000E, remainder 0x0002
    push dx
    call hex
    pop ax
    call hex
    mov ax, 0xF0F0
    shl ax, 4                   ; 0x0F00
    xor ax, 0xAAAA              ; 0xA5AA
    call hex
    exit 0

hex:                            ; print ax as 4 hex digits and a newline
    mov cx, 4
.digit:
    rol ax, 4
    push ax
    and al, 0x0F
    add al, '0'
    cmp al, '9'
    jbe .out
    add al, 'A' - '9' - 1
.out:
    mov dx, 0x3F8
    out dx, al
    pop ax
    loop .digit
    mov al, 10
    out dx, al
    ret
bootsector_end
