%include "boot.inc"
    xor ax, ax
    mov es, ax
    cld
    mov ax, 0xA55A
    mov di, 0x1000
    mov cx, 0x3000              ; words: 0x1000..0x6FFF
    rep stosw
    mov di, 0x1000
    mov cx, 0x3000
    repe scasw
    jne bad
    puts ok
    exit 0
bad:
    puts no
    exit 1
ok: db "ok", 10, 0
no: db "bad", 10, 0
bootsector_end
