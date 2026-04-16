#include "../include/gdt.h"

// Simple GDT initialization - all in assembly
extern void gdt_init_simple(void) asm("_gdt_init_simple");

void gdt_init(void) {
    // Just call the simple assembly version
    gdt_init_simple();
}
