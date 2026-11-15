%include "boot.inc"
%assign N 1000000
    mov ax, 0xA000
    mov ds, ax
    xor si, si
    mov ecx, N
.loop:
    mov al, [si]
    dec ecx
    jnz .loop
    exit 0
bootsector_end
