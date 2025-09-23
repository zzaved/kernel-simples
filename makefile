# Makefile for Jogo do Bixo do Pablito Kernel

# Compiler and assembler
CC = gcc
ASM = nasm
LD = ld

# Flags
CFLAGS = -m32 -c -ffreestanding -fno-stack-protector -fno-builtin
ASMFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T link.ld

# Target files
KERNEL = kernel
KERNEL_ASM = kernel.asm
KERNEL_C = kernel.c
LINKER_SCRIPT = link.ld

# Object files
KERNEL_ASM_OBJ = kasm.o
KERNEL_C_OBJ = kc.o

# Default target
all: $(KERNEL)

# Compile assembly
$(KERNEL_ASM_OBJ): $(KERNEL_ASM)
	$(ASM) $(ASMFLAGS) $< -o $@

# Compile C
$(KERNEL_C_OBJ): $(KERNEL_C) keyboard_map.h
	$(CC) $(CFLAGS) $< -o $@

# Link everything
$(KERNEL): $(KERNEL_ASM_OBJ) $(KERNEL_C_OBJ) $(LINKER_SCRIPT)
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_ASM_OBJ) $(KERNEL_C_OBJ)

# Run with QEMU
run: $(KERNEL)
	qemu-system-i386 -kernel $(KERNEL)

# Clean build files
clean:
	rm -f *.o $(KERNEL)

# Install to /boot (requires sudo)
install: $(KERNEL)
	sudo cp $(KERNEL) /boot/kernel-bixo

.PHONY: all run clean install