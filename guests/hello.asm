%include "boot.inc"
    puts msg
    exit 0
msg: db "hello", 10, 0
bootsector_end
