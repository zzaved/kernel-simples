#ifndef VGA_H
#define VGA_H

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEM ((unsigned char*)0xB8000)

static unsigned short vga_cursor = 0;
static unsigned char  vga_attr = 0x07; /* light grey on black */

static void vga_clear() {
    for (unsigned int i=0; i<VGA_WIDTH*VGA_HEIGHT; ++i) {
        VGA_MEM[i*2]   = ' ';
        VGA_MEM[i*2+1] = vga_attr;
    }
    vga_cursor = 0;
}

static void vga_putc(char c) {
    if (c == '\n') {
        vga_cursor = (vga_cursor / VGA_WIDTH + 1) * VGA_WIDTH;
    } else if (c == '\r') {
        vga_cursor = (vga_cursor / VGA_WIDTH) * VGA_WIDTH;
    } else if (c == '\t') {
        vga_cursor += 4 - (vga_cursor % 4);
    } else {
        VGA_MEM[vga_cursor*2] = (unsigned char)c;
        VGA_MEM[vga_cursor*2+1] = vga_attr;
        vga_cursor++;
    }
    if (vga_cursor >= VGA_WIDTH * VGA_HEIGHT) {
        /* scroll up */
        for (unsigned int row=1; row<VGA_HEIGHT; ++row) {
            for (unsigned int col=0; col<VGA_WIDTH; ++col) {
                unsigned int dst = (row-1)*VGA_WIDTH + col;
                unsigned int src = row*VGA_WIDTH + col;
                VGA_MEM[dst*2]   = VGA_MEM[src*2];
                VGA_MEM[dst*2+1] = VGA_MEM[src*2+1];
            }
        }
        /* clear last line */
        for (unsigned int col=0; col<VGA_WIDTH; ++col) {
            unsigned int idx = (VGA_HEIGHT-1)*VGA_WIDTH + col;
            VGA_MEM[idx*2]   = ' ';
            VGA_MEM[idx*2+1] = vga_attr;
        }
        vga_cursor = (VGA_HEIGHT-1)*VGA_WIDTH;
    }
}

static void vga_print(const char* s) {
    while (*s) vga_putc(*s++);
}

static void vga_println(const char* s) {
    vga_print(s);
    vga_putc('\n');
}

static void vga_set_attr(unsigned char attr) { vga_attr = attr; }

#endif
