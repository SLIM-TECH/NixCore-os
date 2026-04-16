#include "../include/gdt.h"

// Don't reload GDT - bootloader already set it up correctly
// Just keep it as is

void gdt_init(void) {
    // Do nothing - GDT is already loaded by bootloader
    // This prevents all the stack corruption bullshit
}
