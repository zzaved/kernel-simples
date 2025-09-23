TARGET     := build/kernel
ASM_SOURCES:= asm/boot.asm asm/isr.asm
C_SOURCES  := src/kernel.c
OBJ        := build/kasm.o build/isr.o build/kernel.o

NASM       := nasm
NASMFLAGS  := -f elf32

CC         := gcc
CFLAGS     := -m32 -ffreestanding -fno-pic -fno-builtin -fno-stack-protector -nostdlib -nostartfiles -Wall -Wextra -O2
# -ffreestanding: sem stdlib, ambiente freestanding
# -fno-stack-protector/-nostdlib: sem dependências do userland

LD         := ld
LDFLAGS    := -m elf_i386 -T link.ld -nostdlib

all: $(TARGET)

build:
	mkdir -p build

$(TARGET): build $(OBJ) link.ld
	$(LD) $(LDFLAGS) -o $(TARGET) $(OBJ)

build/kasm.o: asm/boot.asm | build
	$(NASM) $(NASMFLAGS) $< -o $@

build/isr.o: asm/isr.asm | build
	$(NASM) $(NASMFLAGS) $< -o $@

build/kernel.o: src/kernel.c src/keyboard_map.h src/vga.h | build
	$(CC) $(CFLAGS) -c src/kernel.c -o $@

clean:
	rm -rf build

.PHONY: all clean
