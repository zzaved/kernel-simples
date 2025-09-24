# Micro Kernel

🎥 PARA ACESSAR O VÍDEO DE EXPLICAÇÃO DO PROJETO [CLIQUE AQUI](https://youtu.be/W4W3duIrCbU)

## Objetivo

Este projeto implementa um micro kernel em x86 que:

* É iniciado por um bootloader em Assembly compatível com Multiboot.
* Escreve diretamente na tela usando a memória de vídeo (0xB8000).
* Configura a IDT e o PIC para lidar com a interrupção de teclado (IRQ1).
* Contém um jogo interativo simples.

## Ambiente de Desenvolvimento: macOS vs Linux

Este sistema foi desenvolvido em um **MacBook Air com macOS (arquitetura ARM)**.
Por isso, utilizou-se **Docker** para fornecer um toolchain 32-bit compatível (`gcc -m32`, `nasm`, `ld`).

No entanto, em **Linux (x86\_64)** o processo é mais simples e direto, pois o compilador de 32 bits pode ser instalado nativamente.

## Execução no Linux

### Dependências

```bash
sudo apt-get update
sudo apt-get install build-essential gcc-multilib nasm qemu-system-x86
```

### Compilação

```bash
make clean all
```

### Execução

```bash
qemu-system-i386 -kernel build/kernel
```

## Execução no macOS (Apple Silicon ou Intel)

### Dependências

* Docker Desktop
* QEMU

```bash
brew install qemu
```

### Construção do ambiente com Docker

```bash
docker buildx build --platform linux/amd64 -t pablito-kernel:dev -f Dockerfile .
```

### Compilação

```bash
docker run --rm -v "$PWD:/src" -w /src --platform linux/amd64 pablito-kernel:dev make clean all
```

O kernel será gerado em:

```
build/kernel
```

### Execução

```bash
qemu-system-i386 -kernel build/kernel
```

## Como Jogar

1. Na tela inicial, o jogador deve escolher um bixo (1–5):

   ```
   1) Leao
   2) Aguia
   3) Macaco
   4) Cobra
   5) Tubarao
   ```

2. Após escolher, pressione `s` para o sorteio.

3. O kernel mostra o bixo sorteado e o resultado (vitória ou derrota).

4. Controles adicionais:

   * `r`: reinicia o jogo.
   * `ESC`: limpa a tela.
   * `m`: retorna ao menu principal.
   * `3`: encerra o kernel (loop infinito).

## Estrutura do Projeto

```
.
├── Dockerfile          # Ambiente de build (gcc -m32, nasm, ld)
├── Makefile            # Script de compilação
├── link.ld             # Linker script (kernel carregado em 0x100000)
├── asm/
│   ├── boot.asm        # Bootloader + cabeçalho Multiboot
│   └── isr.asm         # Funções em assembly (I/O, ISR teclado, load_idt)
├── src/
│   ├── kernel.c        # Lógica do kernel e do jogo
│   ├── vga.h           # Funções de escrita em 0xB8000
│   └── keyboard_map.h  # Tabela scancode → caractere
└── README.md
```

## Como Funciona o Kernel

1. **Bootloader (boot.asm)**

   * Define o cabeçalho Multiboot (0x1BADB002).
   * Configura a pilha (ESP).
   * Chama `kmain()` em C.

2. **Tela / VGA (vga.h)**

   * Escrita direta em `0xB8000`.
   * Cada caractere ocupa 2 bytes (caractere + atributo de cor).
   * Suporte a impressão, rolagem e cores.

3. **Interrupções**

   * PIC remapeado (IRQ0–7 → 0x20..0x27).
   * IDT configurada com handler no vetor `0x21` (teclado).
   * Handler do teclado lê scancode da porta `0x60` e converte em caractere.

4. **Jogo do Bixo do Pablito**

   * Estados: `MENU → RULES → CHOOSE → RESULT → EXIT`.
   * Jogador escolhe de 1 a 5.
   * Um gerador pseudoaleatório sorteia o resultado.
   * Mostra vitória ou derrota.
   * Mantém placar acumulado de vitórias/derrotas.
   * Permite reinício (`r`) ou retorno ao menu (`m`).