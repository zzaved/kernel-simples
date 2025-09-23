/* kernel.c — IDT/PIC/Teclado + VGA + jogo */
typedef unsigned int   u32;
typedef unsigned short u16;
typedef unsigned char  u8;

#include "vga.h"
#include "keyboard_map.h"

/* Declarações vindas do assembly */
extern u8  read_port(u16 port);
extern void write_port(u16 port, u8 data);
extern void load_idt(u32* idt_ptr);

/* ------------ IDT ------------- */
#define IDT_SIZE 256

struct IDT_entry{
    u16 offset_low;
    u16 selector;
    u8  zero;
    u8  type_attr;
    u16 offset_high;
} __attribute__((packed));

static struct IDT_entry IDT[IDT_SIZE];

static void set_idt_entry(int num, u32 base, u16 sel, u8 flags) {
    IDT[num].offset_low = base & 0xFFFF;
    IDT[num].selector   = sel;
    IDT[num].zero       = 0;
    IDT[num].type_attr  = flags;   /* 0x8E = present | DPL=0 | 32-bit interrupt gate */
    IDT[num].offset_high= (base >> 16) & 0xFFFF;
}

extern void keyboard_handler(void);

/* PIC ports */
#define PIC1        0x20
#define PIC2        0xA0
#define PIC1_DATA   (PIC1+1)
#define PIC2_DATA   (PIC2+1)

static void pic_remap(void) {
    /* ICW1 */
    write_port(PIC1, 0x11);
    write_port(PIC2, 0x11);
    /* ICW2 offsets */
    write_port(PIC1_DATA, 0x20); /* IRQ0..7 -> 0x20..0x27 */
    write_port(PIC2_DATA, 0x28); /* IRQ8..15 -> 0x28..0x2F */
    /* ICW3 */
    write_port(PIC1_DATA, 0x00);
    write_port(PIC2_DATA, 0x00);
    /* ICW4 */
    write_port(PIC1_DATA, 0x01);
    write_port(PIC2_DATA, 0x01);
    /* Mascara tudo inicialmente */
    write_port(PIC1_DATA, 0xFF);
    write_port(PIC2_DATA, 0xFF);
}

static void idt_init(void) {
    /* handler teclado em 0x21 */
    set_idt_entry(0x21, (u32)keyboard_handler, 0x08, 0x8E);

    /* ponteiro IDT para lidt: limite (bytes-1) + base */
    u32 idt_addr = (u32)IDT;
    u32 idt_ptr[2];
    idt_ptr[0] = (sizeof(struct IDT_entry) * IDT_SIZE) | ((idt_addr & 0xFFFF) << 16);
    idt_ptr[1] = idt_addr >> 16;
    load_idt(idt_ptr);
}

/* Habilitar apenas IRQ1 (teclado) */
static void kb_enable_irq(void) {
    /* 0xFD (1111 1101) — ativa somente IRQ1 */
    write_port(PIC1_DATA, 0xFD);
}

/* ------------ Teclado ISR ------------- */
#define KEYBOARD_DATA_PORT   0x60
#define KEYBOARD_STATUS_PORT 0x64

static volatile u8 last_key = 0;
static volatile int key_ready = 0;
static volatile unsigned int rng_seed = 2463534242u; /* seed inicial */

void keyboard_handler_main(void) {
    /* EOI para o PIC */
    write_port(PIC1, 0x20);

    u8 status = read_port(KEYBOARD_STATUS_PORT);
    if (status & 0x01) {
        u8 sc = read_port(KEYBOARD_DATA_PORT);
        /* ignora breakcodes (>= 0x80) */
        if (sc < 128) {
            u8 ch = keyboard_map[sc];
            if (ch != 0) {
                last_key = ch;
                key_ready = 1;
                rng_seed ^= sc;         /* mistura scancode no seed */
                rng_seed ^= (rng_seed << 13);
                rng_seed ^= (rng_seed >> 17);
                rng_seed ^= (rng_seed << 5);
            }
        }
    }
}

/* ------------ PRNG / Helpers ------------- */
static unsigned int lcg_rand(void) {
    rng_seed = (1103515245u * rng_seed + 12345u);
    return rng_seed;
}

/* ------------ Jogo ------------- */
enum GameState { GS_WELCOME, GS_CHOOSE, GS_RESULT };

static enum GameState gs = GS_WELCOME;
static int player_choice = 0; /* 1..5 */
static const char* bixos[5] = { "Leao", "Aguia", "Macaco", "Cobra", "Tubarao" };

static void render_header() {
    vga_set_attr(0x0F); /* branco */
    vga_println("=== Jogo do Bixo do Pablito ===");
    vga_set_attr(0x07);
}

static void render_welcome() {
    vga_clear();
    render_header();
    vga_println("");
    vga_println("Escolha seu bixo (1-5) e pressione a tecla correspondente:");
    for (int i=0;i<5;i++){
        vga_print("  ");
        char idx = '1'+i;
        vga_putc(idx);
        vga_print(") ");
        vga_println(bixos[i]);
    }
    vga_println("");
    vga_println("Depois pressione 's' para sortear. 'r' reinicia. ESC limpa.");
    gs = GS_CHOOSE;
}

static void render_choice() {
    vga_print("Voce escolheu: ");
    if (player_choice>=1 && player_choice<=5) vga_println(bixos[player_choice-1]);
    else vga_println("?");
    vga_println("Pressione 's' para sortear...");
}

static void render_result(int result_idx) {
    vga_print("Bixeiro sorteou: ");
    vga_println(bixos[result_idx-1]);
    if (result_idx == player_choice) {
        vga_set_attr(0x0A);
        vga_println("Parabens! Voce acertou!");
    } else {
        vga_set_attr(0x0C);
        vga_println("Nao foi dessa vez. :(");
    }
    vga_set_attr(0x07);
    vga_println("Pressione 'r' para jogar de novo.");
    gs = GS_RESULT;
}

static void handle_key(char ch) {
    if (ch == 27) { /* ESC */
        vga_clear();
        render_header();
        return;
    }
    if (gs == GS_CHOOSE) {
        if (ch >= '1' && ch <= '5') {
            player_choice = (int)(ch - '0');
            render_choice();
            return;
        }
        if (ch == 's') {
            if (player_choice < 1 || player_choice > 5) {
                vga_println("Escolha primeiro entre 1..5.");
                return;
            }
            int result = (int)(lcg_rand() % 5u) + 1;
            render_result(result);
            return;
        }
    } else if (gs == GS_RESULT) {
        if (ch == 'r') {
            player_choice = 0;
            render_welcome();
            return;
        }
    } else if (gs == GS_WELCOME) {
        render_welcome();
    }
}

/* ------------ kmain ------------- */
void kmain(void) {
    vga_clear();
    render_header();
    vga_println("Inicializando IDT/PIC/Teclado...");
    pic_remap();
    idt_init();
    kb_enable_irq();
    vga_println("Pronto!");
    vga_println("");
    render_welcome();

    /* Loop principal: consome teclas colocadas pela ISR */
    for(;;){
        if (key_ready) {
            char ch = (char)last_key;
            key_ready = 0;
            handle_key(ch);
        }
        /* opcionalmente poderíamos hlt aqui e usar outra IRQ p/ acordar */
    }
}
