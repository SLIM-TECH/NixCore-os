#include "../include/interrupts.h"
#include "../include/io.h"
#include "../include/kernel.h"
#include "../include/memory.h"

static interrupt_handler_t interrupt_handlers[256];

struct idt_entry {
    uint16_t base_lo;
    uint16_t sel;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_hi;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_entry idt_entries[256];
static struct idt_ptr   idt_ptr;

static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].base_lo = base & 0xFFFF;
    idt_entries[num].base_hi = (base >> 16) & 0xFFFF;
    idt_entries[num].sel = sel;
    idt_entries[num].always0 = 0;
    idt_entries[num].flags = flags;
}

extern void __isr0(void);
extern void __isr1(void);
extern void __isr2(void);
extern void __isr3(void);
extern void __isr4(void);
extern void __isr5(void);
extern void __isr6(void);
extern void __isr7(void);
extern void __isr8(void);
extern void __isr9(void);
extern void __isr10(void);
extern void __isr11(void);
extern void __isr12(void);
extern void __isr13(void);
extern void __isr14(void);
extern void __isr15(void);
extern void __isr16(void);
extern void __isr17(void);
extern void __isr18(void);
extern void __isr19(void);
extern void __isr20(void);
extern void __isr21(void);
extern void __isr22(void);
extern void __isr23(void);
extern void __isr24(void);
extern void __isr25(void);
extern void __isr26(void);
extern void __isr27(void);
extern void __isr28(void);
extern void __isr29(void);
extern void __isr30(void);
extern void __isr31(void);

extern void __irq0(void);
extern void __irq1(void);
extern void __irq2(void);
extern void __irq3(void);
extern void __irq4(void);
extern void __irq5(void);
extern void __irq6(void);
extern void __irq7(void);
extern void __irq8(void);
extern void __irq9(void);
extern void __irq10(void);
extern void __irq11(void);
extern void __irq12(void);
extern void __irq13(void);
extern void __irq14(void);
extern void __irq15(void);

static void pic_remap(void) {
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x00);
    outb(0xA1, 0x00);
}

void interrupts_init() {
    idt_ptr.limit = sizeof(struct idt_entry) * 256 - 1;
    idt_ptr.base  = (uint32_t)&idt_entries;

    memset(&idt_entries, 0, sizeof(struct idt_entry) * 256);
    memset(&interrupt_handlers, 0, sizeof(interrupt_handler_t) * 256);

    pic_remap();

    idt_set_gate(0, (uint32_t)__isr0, 0x08, 0x8E);
    idt_set_gate(1, (uint32_t)__isr1, 0x08, 0x8E);
    idt_set_gate(2, (uint32_t)__isr2, 0x08, 0x8E);
    idt_set_gate(3, (uint32_t)__isr3, 0x08, 0x8E);
    idt_set_gate(4, (uint32_t)__isr4, 0x08, 0x8E);
    idt_set_gate(5, (uint32_t)__isr5, 0x08, 0x8E);
    idt_set_gate(6, (uint32_t)__isr6, 0x08, 0x8E);
    idt_set_gate(7, (uint32_t)__isr7, 0x08, 0x8E);
    idt_set_gate(8, (uint32_t)__isr8, 0x08, 0x8E);
    idt_set_gate(9, (uint32_t)__isr9, 0x08, 0x8E);
    idt_set_gate(10, (uint32_t)__isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)__isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)__isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)__isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)__isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint32_t)__isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint32_t)__isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint32_t)__isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)__isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint32_t)__isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint32_t)__isr20, 0x08, 0x8E);
    idt_set_gate(21, (uint32_t)__isr21, 0x08, 0x8E);
    idt_set_gate(22, (uint32_t)__isr22, 0x08, 0x8E);
    idt_set_gate(23, (uint32_t)__isr23, 0x08, 0x8E);
    idt_set_gate(24, (uint32_t)__isr24, 0x08, 0x8E);
    idt_set_gate(25, (uint32_t)__isr25, 0x08, 0x8E);
    idt_set_gate(26, (uint32_t)__isr26, 0x08, 0x8E);
    idt_set_gate(27, (uint32_t)__isr27, 0x08, 0x8E);
    idt_set_gate(28, (uint32_t)__isr28, 0x08, 0x8E);
    idt_set_gate(29, (uint32_t)__isr29, 0x08, 0x8E);
    idt_set_gate(30, (uint32_t)__isr30, 0x08, 0x8E);
    idt_set_gate(31, (uint32_t)__isr31, 0x08, 0x8E);

    idt_set_gate(32, (uint32_t)__irq0, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)__irq1, 0x08, 0x8E);
    idt_set_gate(34, (uint32_t)__irq2, 0x08, 0x8E);
    idt_set_gate(35, (uint32_t)__irq3, 0x08, 0x8E);
    idt_set_gate(36, (uint32_t)__irq4, 0x08, 0x8E);
    idt_set_gate(37, (uint32_t)__irq5, 0x08, 0x8E);
    idt_set_gate(38, (uint32_t)__irq6, 0x08, 0x8E);
    idt_set_gate(39, (uint32_t)__irq7, 0x08, 0x8E);
    idt_set_gate(40, (uint32_t)__irq8, 0x08, 0x8E);
    idt_set_gate(41, (uint32_t)__irq9, 0x08, 0x8E);
    idt_set_gate(42, (uint32_t)__irq10, 0x08, 0x8E);
    idt_set_gate(43, (uint32_t)__irq11, 0x08, 0x8E);
    idt_set_gate(44, (uint32_t)__irq12, 0x08, 0x8E);
    idt_set_gate(45, (uint32_t)__irq13, 0x08, 0x8E);
    idt_set_gate(46, (uint32_t)__irq14, 0x08, 0x8E);
    idt_set_gate(47, (uint32_t)__irq15, 0x08, 0x8E);

    asm volatile("lidt %0" : : "m"(idt_ptr));
}

void register_interrupt_handler(uint8_t n, interrupt_handler_t handler) {
    interrupt_handlers[n] = handler;
}

void isr_handler(registers_t regs) {
    if (interrupt_handlers[regs.int_no]) {
        interrupt_handlers[regs.int_no](regs);
    }
}

void irq_handler(registers_t regs) {
    if (regs.int_no >= 40) {
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);

    if (interrupt_handlers[regs.int_no]) {
        interrupt_handlers[regs.int_no](regs);
    }
}
