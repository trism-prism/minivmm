%include "boot.inc"
%assign N 1000000
    mov dx, 0xE9
    xor al, al
    mov ecx, N
.loop:
    out dx, al
    dec ecx
    jnz .loop
    exit 0
bootsector_end
