# Micro Kernel — "Jogo do Bixo do Pablito" (x86, QEMU)

Um micro kernel em x86 que:
- Boota via Multiboot (compatível com GRUB / `qemu -kernel`)
- Escreve na tela via memória de vídeo (0xB8000)
- Configura IDT, remapeia o PIC e trata interrupção do teclado (IRQ1 → int 0x21)
- Implementa um joguinho: escolha um dos 5 **bixos** (1–5) e pressione `s` para sortear. O kernel gera um resultado pseudoaleatório e informa vitória/derrota. `r` reinicia.

> Desenvolvido para rodar no **QEMU** sem depender de hardware real.

## Requisitos (macOS/Apple Silicon)

- **QEMU**: `brew install qemu`
- **Docker** (usaremos toolchain via container x86): `Docker Desktop` + `buildx`

> Usamos Docker `linux/amd64` para garantir toolchain 32-bit. Isso funciona em Apple Silicon via emulação.

## Rodando (sem ISO, usando Multiboot diretamente)

```bash
# 1) Build da toolchain/ambiente
docker buildx build --platform linux/amd64 -t pablito-kernel:dev -f Dockerfile .

# 2) Compilar
docker run --rm -v "$PWD:/src" -w /src --platform linux/amd64 pablito-kernel:dev make clean all

# 3) Executar no QEMU
qemu-system-i386 -kernel build/kernel
```

> Dica: se quiser uma janela menor: `-vga std -display default,show-cursor=on`

## Controles do jogo

- Na tela inicial, pressione **1..5** para escolher seu *bixo*.
- Pressione **s** para *sortear*.
- O kernel mostrará o resultado e se você ganhou.
- Pressione **r** para reiniciar o jogo.
- `ESC` limpa a tela.

O gerador pseudoaleatório usa um **seed** atualizado com os *scancodes* das teclas digitadas.

## Estrutura

```
.
├── Dockerfile
├── Makefile
├── link.ld
├── asm/
│   ├── boot.asm         # ponto de entrada + cabeçalho multiboot
│   └── isr.asm          # in/out de portas + wrapper da ISR de teclado + load_idt
├── src/
│   ├── kernel.c         # kmain + VGA, IDT/PIC, teclado, jogo
│   ├── keyboard_map.h   # mapa de scancodes → chars
│   └── vga.h            # helpers VGA
└── README.md
```

## Como funciona (resumo técnico)

- **boot.asm** define o cabeçalho **Multiboot** (0x1BADB002) nos primeiros 8KB, exporta o símbolo `start`, desabilita interrupções (`cli`), define `esp`, chama `kmain` e dá `hlt`.
- **link.ld** fixa a base do binário em **0x0010_0000** (1 MiB), alinhado com a prática comum de kernels x86.
- **isr.asm** implementa `read_port`, `write_port`, `load_idt`, e o *stub* `keyboard_handler` que chama `keyboard_handler_main` (em C) e retorna com `iretd`.
- **kernel.c**:
  - Implementa **IDT** e remapeia o **PIC** (ICW1..4) para que IRQ0..15 virem **0x20..0x2F**.
  - Habilita somente **IRQ1** (teclado) na máscara do PIC.
  - Configura a entrada **IDT[0x21]** para o handler do teclado.
  - Escreve diretamente em **0xB8000** (tela modo texto 80x25, 2 bytes por célula).
  - Loop do jogo: estado, input do usuário via ISR, render simples.
  - PRNG linear congruente com *seed* vindo dos scancodes.