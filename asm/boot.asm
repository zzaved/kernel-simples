; boot.asm — ponto de entrada + cabeçalho Multiboot
BITS 32
SECTION .text
    align 4
    dd 0x1BADB002        ; magic
    dd 0x00              ; flags
    dd -(0x1BADB002+0x00); checksum

global start
extern kmain

start:
    cli
    mov esp, stack_top
    call kmain
.hang:
    hlt
    jmp .hang

SECTION .bss
    align 16
stack_bottom:
    resb 8192            ; 8 KiB
stack_top:
