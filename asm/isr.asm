; isr.asm — wrappers, portas e load_idt
BITS 32
SECTION .text
global read_port
global write_port
global load_idt
global keyboard_handler
extern keyboard_handler_main

read_port:
    mov edx, [esp + 4]
    in  al, dx
    movzx eax, al
    ret

write_port:
    mov edx, [esp + 4]
    mov eax, [esp + 8]
    mov al, al           ; baixo de eax já está em al
    out dx, al
    ret

; void load_idt(unsigned long* idt_ptr);
load_idt:
    mov edx, [esp + 4]
    lidt [edx]
    sti
    ret

; Stub da ISR do teclado → chama C e retorna com iretd
keyboard_handler:
    pusha
    cld
    call keyboard_handler_main
    popa
    iretd
