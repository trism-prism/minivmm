%include "boot.inc"
    lidt [empty]                ; limit 0: any interrupt faults, then double and triple faults
    int3
empty: times 6 db 0
bootsector_end
