%include "boot.inc"
%assign N 200000
    mov dx, 0x3F8
    mov al, 'x'
    mov ecx, N
.loop:
    out dx, al
    dec ecx
    jnz .loop
    exit 0
bootsector_end
