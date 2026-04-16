#include "nvme.h"
#include "../kernel/mm/mm.h"
#include <stdint.h>
#include <stddef.h>

static NVME_REGS *nvme_regs = NULL;
static NVME_QUEUE admin_queue;
static NVME_QUEUE io_queue;

// PCI configuration space access
static uint32_t pci_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | 0x80000000);
    __asm__ volatile("outl %0, $0xCF8" :: "a"(address));
    uint32_t result;
    __asm__ volatile("inl $0xCFC, %0" : "=a"(result));
    return result;
}

static void pci_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | 0x80000000);
    __asm__ volatile("outl %0, $0xCF8" :: "a"(address));
    __asm__ volatile("outl %0, $0xCFC" :: "a"(value));
}

void nvme_init(void) {
    // Scan PCI for NVMe controller (Class 01, Subclass 08, Interface 02)
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint32_t vendor = pci_read(bus, slot, func, 0);
                if (vendor == 0xFFFFFFFF) continue;

                uint32_t class_code = pci_read(bus, slot, func, 0x08);
                uint8_t base_class = (class_code >> 24) & 0xFF;
                uint8_t sub_class = (class_code >> 16) & 0xFF;
                uint8_t prog_if = (class_code >> 8) & 0xFF;

                if (base_class == 0x01 && sub_class == 0x08 && prog_if == 0x02) {
                    // Found NVMe controller
                    uint32_t bar0_low = pci_read(bus, slot, func, 0x10);
                    uint32_t bar0_high = pci_read(bus, slot, func, 0x14);
                    uint64_t bar0 = ((uint64_t)bar0_high << 32) | (bar0_low & 0xFFFFFFF0);
                    nvme_regs = (NVME_REGS*)bar0;

                    // Enable bus mastering and memory space
                    uint32_t cmd = pci_read(bus, slot, func, 0x04);
                    cmd |= 0x06;
                    pci_write(bus, slot, func, 0x04, cmd);

                    goto found_nvme;
                }
            }
        }
    }
    return; // No NVMe controller found

found_nvme:
    // Disable controller
    nvme_regs->cc = 0;
    while (nvme_regs->csts & NVME_CSTS_RDY);

    // Allocate admin queues
    admin_queue.sq = (NVME_COMMAND*)kmalloc(64 * sizeof(NVME_COMMAND));
    admin_queue.cq = (NVME_COMPLETION*)kmalloc(64 * sizeof(NVME_COMPLETION));
    admin_queue.sq_size = 64;
    admin_queue.cq_size = 64;
    admin_queue.sq_tail = 0;
    admin_queue.cq_head = 0;
    admin_queue.phase = 1;
    memset(admin_queue.sq, 0, 64 * sizeof(NVME_COMMAND));
    memset(admin_queue.cq, 0, 64 * sizeof(NVME_COMPLETION));

    // Set admin queue attributes
    nvme_regs->aqa = ((admin_queue.cq_size - 1) << 16) | (admin_queue.sq_size - 1);
    nvme_regs->asq = (uint64_t)admin_queue.sq;
    nvme_regs->acq = (uint64_t)admin_queue.cq;

    // Enable controller
    nvme_regs->cc = NVME_CC_ENABLE | NVME_CC_IOSQES | NVME_CC_IOCQES;
    while (!(nvme_regs->csts & NVME_CSTS_RDY));

    // Allocate I/O queues
    io_queue.sq = (NVME_COMMAND*)kmalloc(64 * sizeof(NVME_COMMAND));
    io_queue.cq = (NVME_COMPLETION*)kmalloc(64 * sizeof(NVME_COMPLETION));
    io_queue.sq_size = 64;
    io_queue.cq_size = 64;
    io_queue.sq_tail = 0;
    io_queue.cq_head = 0;
    io_queue.phase = 1;
    memset(io_queue.sq, 0, 64 * sizeof(NVME_COMMAND));
    memset(io_queue.cq, 0, 64 * sizeof(NVME_COMPLETION));

    // Create I/O completion queue
    NVME_COMMAND *cmd = &admin_queue.sq[admin_queue.sq_tail];
    memset(cmd, 0, sizeof(NVME_COMMAND));
    cmd->cdw0 = NVME_ADMIN_CREATE_CQ;
    cmd->prp1 = (uint64_t)io_queue.cq;
    cmd->cdw10 = ((io_queue.cq_size - 1) << 16) | 1; // QID=1, Size=64
    cmd->cdw11 = 1; // Physically contiguous

    admin_queue.sq_tail = (admin_queue.sq_tail + 1) % admin_queue.sq_size;
    *(volatile uint32_t*)(((uint64_t)nvme_regs) + 0x1000) = admin_queue.sq_tail;

    // Wait for completion
    while ((admin_queue.cq[admin_queue.cq_head].status & 1) != admin_queue.phase);
    admin_queue.cq_head = (admin_queue.cq_head + 1) % admin_queue.cq_size;
    if (admin_queue.cq_head == 0) admin_queue.phase = !admin_queue.phase;
    *(volatile uint32_t*)(((uint64_t)nvme_regs) + 0x1000 + 4) = admin_queue.cq_head;

    // Create I/O submission queue
    cmd = &admin_queue.sq[admin_queue.sq_tail];
    memset(cmd, 0, sizeof(NVME_COMMAND));
    cmd->cdw0 = NVME_ADMIN_CREATE_SQ;
    cmd->prp1 = (uint64_t)io_queue.sq;
    cmd->cdw10 = ((io_queue.sq_size - 1) << 16) | 1; // QID=1, Size=64
    cmd->cdw11 = (1 << 16) | 1; // CQID=1, Physically contiguous

    admin_queue.sq_tail = (admin_queue.sq_tail + 1) % admin_queue.sq_size;
    *(volatile uint32_t*)(((uint64_t)nvme_regs) + 0x1000) = admin_queue.sq_tail;

    // Wait for completion
    while ((admin_queue.cq[admin_queue.cq_head].status & 1) != admin_queue.phase);
    admin_queue.cq_head = (admin_queue.cq_head + 1) % admin_queue.cq_size;
    if (admin_queue.cq_head == 0) admin_queue.phase = !admin_queue.phase;
    *(volatile uint32_t*)(((uint64_t)nvme_regs) + 0x1000 + 4) = admin_queue.cq_head;
}

int nvme_read(uint32_t nsid, uint64_t lba, uint32_t count, uint8_t *buf) {
    NVME_COMMAND *cmd = &io_queue.sq[io_queue.sq_tail];

    memset(cmd, 0, sizeof(NVME_COMMAND));
    cmd->cdw0 = NVME_CMD_READ;
    cmd->nsid = nsid;
    cmd->prp1 = (uint64_t)buf;

    // Handle PRP2 for transfers > 4KB
    if (count > 8) {
        cmd->prp2 = (uint64_t)(buf + 4096);
    }

    cmd->cdw10 = (uint32_t)lba;
    cmd->cdw11 = (uint32_t)(lba >> 32);
    cmd->cdw12 = count - 1; // 0-based

    io_queue.sq_tail = (io_queue.sq_tail + 1) % io_queue.sq_size;
    *(volatile uint32_t*)(((uint64_t)nvme_regs) + 0x1000 + 8) = io_queue.sq_tail;

    // Wait for completion
    int timeout = 1000000;
    while (((io_queue.cq[io_queue.cq_head].status & 1) != io_queue.phase) && timeout > 0) {
        timeout--;
    }

    if (timeout == 0) return 0;

    uint16_t status = io_queue.cq[io_queue.cq_head].status >> 1;
    io_queue.cq_head = (io_queue.cq_head + 1) % io_queue.cq_size;
    if (io_queue.cq_head == 0) io_queue.phase = !io_queue.phase;

    *(volatile uint32_t*)(((uint64_t)nvme_regs) + 0x1000 + 12) = io_queue.cq_head;

    return status == 0 ? 1 : 0;
}

int nvme_write(uint32_t nsid, uint64_t lba, uint32_t count, uint8_t *buf) {
    NVME_COMMAND *cmd = &io_queue.sq[io_queue.sq_tail];

    memset(cmd, 0, sizeof(NVME_COMMAND));
    cmd->cdw0 = NVME_CMD_WRITE;
    cmd->nsid = nsid;
    cmd->prp1 = (uint64_t)buf;

    // Handle PRP2 for transfers > 4KB
    if (count > 8) {
        cmd->prp2 = (uint64_t)(buf + 4096);
    }

    cmd->cdw10 = (uint32_t)lba;
    cmd->cdw11 = (uint32_t)(lba >> 32);
    cmd->cdw12 = count - 1;

    io_queue.sq_tail = (io_queue.sq_tail + 1) % io_queue.sq_size;
    *(volatile uint32_t*)(((uint64_t)nvme_regs) + 0x1000 + 8) = io_queue.sq_tail;

    // Wait for completion
    int timeout = 1000000;
    while (((io_queue.cq[io_queue.cq_head].status & 1) != io_queue.phase) && timeout > 0) {
        timeout--;
    }

    if (timeout == 0) return 0;

    uint16_t status = io_queue.cq[io_queue.cq_head].status >> 1;
    io_queue.cq_head = (io_queue.cq_head + 1) % io_queue.cq_size;
    if (io_queue.cq_head == 0) io_queue.phase = !io_queue.phase;

    *(volatile uint32_t*)(((uint64_t)nvme_regs) + 0x1000 + 12) = io_queue.cq_head;

    return status == 0 ? 1 : 0;
}
