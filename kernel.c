/*
 * kernel.c - Jogo do Bixo do Pablito Kernel
 */

#include "keyboard_map.h"

/* Hardware text mode color constants. */
enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define IDT_SIZE 256
#define INTERRUPT_GATE 0x8e
#define KERNEL_CODE_SEGMENT_OFFSET 0x08

#define ENTER_KEY_CODE 0x1C

extern unsigned char keyboard_map[128];
extern void keyboard_handler(void);
extern char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);
extern void load_idt(unsigned long *idt_ptr);

/* Current cursor location */
unsigned int current_loc = 0;
/* Video pointer to text memory */
char *vidptr = (char*)0xb8000;

/* Game state variables */
int game_state = 0; // 0 = menu, 1 = playing, 2 = result
int player_choice = -1;
int winning_animal = -1;
int score = 0;
int rounds = 0;

/* IDT entry structure */
struct IDT_entry {
	unsigned short int offset_lowerbits;
	unsigned short int selector;
	unsigned char zero;
	unsigned char type_attr;
	unsigned short int offset_higherbits;
};

struct IDT_entry IDT[IDT_SIZE];

/* Simple pseudo-random number generator */
unsigned long rand_seed = 1;

unsigned int simple_rand(void) {
    rand_seed = rand_seed * 1103515245 + 12345;
    return (rand_seed / 65536) % 32768;
}

void init_idt(void) {
	unsigned long keyboard_address;
	unsigned long idt_address;
	unsigned long idt_ptr[2];

	/* populate IDT entry of keyboard's interrupt */
	keyboard_address = (unsigned long)keyboard_handler;
	IDT[0x21].offset_lowerbits = keyboard_address & 0xffff;
	IDT[0x21].selector = KERNEL_CODE_SEGMENT_OFFSET;
	IDT[0x21].zero = 0;
	IDT[0x21].type_attr = INTERRUPT_GATE;
	IDT[0x21].offset_higherbits = (keyboard_address & 0xffff0000) >> 16;

	/* Ports
	 * PIC1	PIC2
	 * 0x20	0xA0
	 * 0x21	0xA1
	 */

	/* ICW1 - begin initialization */
	write_port(0x20, 0x11);
	write_port(0xA0, 0x11);

	/* ICW2 - remap offset address of IDT */
	write_port(0x21, 0x20);
	write_port(0xA1, 0x28);

	/* ICW3 - setup cascading */
	write_port(0x21, 0x00);
	write_port(0xA1, 0x00);

	/* ICW4 - environment info */
	write_port(0x21, 0x01);
	write_port(0xA1, 0x01);

	/* Initialization finished - mask interrupts */
	write_port(0x21, 0xff);
	write_port(0xA1, 0xff);

	/* fill the IDT descriptor */
	idt_address = (unsigned long)IDT;
	idt_ptr[0] = (sizeof(struct IDT_entry) * IDT_SIZE) + ((idt_address & 0xffff) << 16);
	idt_ptr[1] = idt_address >> 16;

	load_idt(idt_ptr);
}

void kb_init(void) {
	/* 0xFD is 11111101 - enables only keyboard interrupt (IRQ1) */
	write_port(0x21, 0xFD);
}

void kprint(const char *str) {
	unsigned int i = 0;
	while (str[i] != '\0') {
		vidptr[current_loc++] = str[i++];
		vidptr[current_loc++] = 0x07;
	}
}

void kprint_newline(void) {
	unsigned int line_size = 80 * 2;
	current_loc = current_loc + (line_size - current_loc % line_size);
}

void kprint_colored(const char *str, unsigned char color) {
	unsigned int i = 0;
	while (str[i] != '\0') {
		vidptr[current_loc++] = str[i++];
		vidptr[current_loc++] = color;
	}
}

void clear_screen(void) {
	unsigned int i = 0;
	while (i < 80 * 25 * 2) {
		vidptr[i++] = ' ';
		vidptr[i++] = 0x07;
	}
	current_loc = 0;
}

void print_number(int num) {
	if (num == 0) {
		vidptr[current_loc++] = '0';
		vidptr[current_loc++] = 0x07;
		return;
	}
	
	char str[12];
	int i = 0;
	int is_negative = 0;
	
	if (num < 0) {
		is_negative = 1;
		num = -num;
	}
	
	while (num > 0) {
		str[i++] = (num % 10) + '0';
		num /= 10;
	}
	
	if (is_negative) {
		str[i++] = '-';
	}
	
	// Reverse string
	for (int j = i - 1; j >= 0; j--) {
		vidptr[current_loc++] = str[j];
		vidptr[current_loc++] = 0x07;
	}
}

void show_menu(void) {
	clear_screen();
	kprint_colored("=== JOGO DO BIXO DO PABLITO ===\n", 0x0E); // Yellow
	kprint_newline();
	kprint_newline();
	
	kprint("Escolha seu bixo da sorte:\n");
	kprint_newline();
	kprint_colored("1 - Leao", 0x0C); // Light red
	kprint_newline();
	kprint_colored("2 - Macaco", 0x0A); // Light green
	kprint_newline();
	kprint_colored("3 - Elefante", 0x09); // Light blue
	kprint_newline();
	kprint_colored("4 - Cobra", 0x0D); // Light magenta
	kprint_newline();
	kprint_colored("5 - Aguia", 0x0B); // Light cyan
	kprint_newline();
	kprint_newline();
	
	kprint("Pontuacao: ");
	print_number(score);
	kprint(" | Rodadas: ");
	print_number(rounds);
	kprint_newline();
	kprint_newline();
	
	kprint("Digite o numero do seu bixo: ");
}

void show_drawing(void) {
	clear_screen();
	kprint_colored("O bixeiro esta sorteando o bixo do dia...", 0x0F);
	kprint_newline();
	kprint_newline();
	
	// Simple animation
	for (int i = 0; i < 5; i++) {
		kprint(".");
		// Simple delay
		for (volatile int j = 0; j < 1000000; j++) {}
	}
	
	// Generate winning number
	winning_animal = (simple_rand() % 5) + 1;
}

void show_result(void) {
	clear_screen();
	
	const char* animals[] = {"", "Leao", "Macaco", "Elefante", "Cobra", "Aguia"};
	
	kprint("Sua escolha: ");
	kprint(animals[player_choice]);
	kprint_newline();
	kprint("Bixo sorteado: ");
	kprint_colored(animals[winning_animal], 0x0E);
	kprint_newline();
	kprint_newline();
	
	if (player_choice == winning_animal) {
		kprint_colored("PARABENS! VOCE ACERTOU!", 0x0A); // Green
		score += 10;
	} else {
		kprint_colored("Que pena! Tente novamente!", 0x0C); // Red
		if (score > 0) score -= 2;
	}
	
	rounds++;
	kprint_newline();
	kprint_newline();
	
	kprint("Nova pontuacao: ");
	print_number(score);
	kprint_newline();
	kprint_newline();
	
	kprint("Pressione ENTER para jogar novamente ou ESC para sair");
}

void keyboard_handler_main(void) {
	unsigned char status;
	char keycode;

	/* write EOI */
	write_port(0x20, 0x20);

	status = read_port(KEYBOARD_STATUS_PORT);
	if (status & 0x01) {
		keycode = read_port(KEYBOARD_DATA_PORT);
		if (keycode < 0) return;

		// Handle different game states
		if (game_state == 0) { // Menu state
			if (keycode >= 0x02 && keycode <= 0x06) { // Keys 1-5
				player_choice = keycode - 0x01; // Convert to 1-5
				game_state = 1;
				show_drawing();
				game_state = 2;
				show_result();
			}
		} else if (game_state == 2) { // Result state
			if (keycode == ENTER_KEY_CODE) { // Enter key
				game_state = 0;
				show_menu();
			} else if (keycode == 0x01) { // ESC key
				clear_screen();
				kprint_colored("Obrigado por jogar o Jogo do Bixo do Pablito!", 0x0F);
				kprint_newline();
				kprint("Pontuacao final: ");
				print_number(score);
				kprint(" em ");
				print_number(rounds);
				kprint(" rodadas");
			}
		}
	}
}

void kmain(void) {
	clear_screen();
	
	init_idt();
	kb_init();
	
	// Initialize random seed with a simple method
	rand_seed = 12345;
	
	show_menu();
	
	while(1) {
		// Kernel main loop - interrupts handle everything
	}
}