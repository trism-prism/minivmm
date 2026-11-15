; Shared guest boilerplate. Every guest is a 512-byte boot sector so that
; QEMU (BIOS loads sector 0 at 0x7C00, needs 0x55AA at offset 510) and
; minivmm (loads the file at 0x7C00) run the identical binary.
bits 16
org 0x7C00

%macro puts 1                   ; print a NUL-terminated string to COM1
    mov si, %1
%%next:
    lodsb
    test al, al
    jz %%done
    mov dx, 0x3F8
    out dx, al
    jmp %%next
%%done:
%endmacro

%macro exit 1                   ; end the run with status %1 (port 0xF4)
    mov al, %1
    out 0xF4, al
    hlt                         ; not reached; stops a runaway if the exit is ignored
%endmacro

%macro bootsector_end 0
    times 510-($-$$) db 0
    dw 0xAA55
%endmacro
