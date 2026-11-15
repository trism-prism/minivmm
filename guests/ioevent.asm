%include "boot.inc"
    mov dx, 0xE9
    xor al, al
    out dx, al
    out dx, al
    out dx, al
    exit 0
bootsector_end
