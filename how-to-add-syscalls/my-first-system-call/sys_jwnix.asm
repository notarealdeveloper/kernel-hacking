;--------------------------------------------;
; Testing the syscall I added to the kernel! ;
;--------------------------------------------;

; Compile, run, and see what happened:
; sudo dmesg -C; nasmcompile sys_jwnix.asm; ./sys_jwnix; echo $?; dmesg

%define sys_exit        60
%define sys_jwnix_32    358
%define sys_jwnix_64    322

; variable macros
%define stdin           0
%define stdout          1
%define stderr          2
%define EXIT_SUCCESS    0

section .text
global _start
_start:

    ; call the 32 bit version
    mov     eax, sys_jwnix_32
    int 0x80

    ; call the 64 bit version
    mov     rax, sys_jwnix_64
    syscall

    call exit

exit:
    mov     rdi, rax       ; Get the exit code. It should be 69.
    mov     rax, sys_exit
    syscall
