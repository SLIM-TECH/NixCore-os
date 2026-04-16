#include "ahci.h"
#include "../kernel/mm/mm.h"
#include <stdint.h>
#include <stddef.h>

static HBA_MEM *ahci_base = NULL;
static uint8_t *port_buffers[32];

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

void ahci_init(void) {
    // Scan PCI bus for AHCI controller (Class 01, Subclass 06, Interface 01)
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint32_t vendor = pci_read(bus, slot, func, 0);
                if (vendor == 0xFFFFFFFF) continue;

                uint32_t class_code = pci_read(bus, slot, func, 0x08);
                uint8_t base_class = (class_code >> 24) & 0xFF;
                uint8_t sub_class = (class_code >> 16) & 0xFF;
                uint8_t prog_if = (class_code >> 8) & 0xFF;

                if (base_class == 0x01 && sub_class == 0x06 && prog_if == 0x01) {
                    // Found AHCI controller
                    uint32_t bar5 = pci_read(bus, slot, func, 0x24);
                    ahci_base = (HBA_MEM*)(uint64_t)(bar5 & 0xFFFFFFF0);

                    // Enable bus mastering and memory space
                    uint32_t cmd = pci_read(bus, slot, func, 0x04);
                    cmd |= 0x06; // Bus master + Memory space
                    pci_write(bus, slot, func, 0x04, cmd);

                    // Enable AHCI mode
                    ahci_base->ghc |= (1 << 31);

                    // Probe all ports
                    ahci_probe_ports(ahci_base);
                    return;
                }
            }
        }
    }
}

void ahci_probe_ports(HBA_MEM *abar) {
    uint32_t pi = abar->pi;

    for (int i = 0; i < 32; i++) {
        if (pi & 1) {
            int dt = ahci_check_type(&abar->ports[i]);
            if (dt == AHCI_DEV_SATA) {
                // Found SATA drive - initialize port
                ahci_port_rebase(&abar->ports[i], i);
            }
        }
        pi >>= 1;
    }
}

void ahci_port_rebase(HBA_PORT *port, int portno) {
    stop_cmd(port);

    // Allocate command list (1K per port)
    uint64_t clb = (uint64_t)kmalloc(1024);
    port->clb = (uint32_t)(clb & 0xFFFFFFFF);
    port->clbu = (uint32_t)((clb >> 32) & 0xFFFFFFFF);
    memset((void*)clb, 0, 1024);

    // Allocate FIS receive area (256 bytes per port)
    uint64_t fb = (uint64_t)kmalloc(256);
    port->fb = (uint32_t)(fb & 0xFFFFFFFF);
    port->fbu = (uint32_t)((fb >> 32) & 0xFFFFFFFF);
    memset((void*)fb, 0, 256);

    // Allocate command tables (256 bytes each, 32 commands)
    HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER*)clb;
    for (int i = 0; i < 32; i++) {
        cmdheader[i].prdtl = 8; // 8 PRDT entries per command table

        uint64_t ctba = (uint64_t)kmalloc(256 + 8 * sizeof(HBA_PRDT_ENTRY));
        cmdheader[i].ctba = (uint32_t)(ctba & 0xFFFFFFFF);
        cmdheader[i].ctbau = (uint32_t)((ctba >> 32) & 0xFFFFFFFF);
        memset((void*)ctba, 0, 256 + 8 * sizeof(HBA_PRDT_ENTRY));
    }

    start_cmd(port);
}

int ahci_check_type(HBA_PORT *port) {
    uint32_t ssts = port->ssts;

    uint8_t ipm = (ssts >> 8) & 0x0F;
    uint8_t det = ssts & 0x0F;

    if (det != 3) return AHCI_DEV_NULL;
    if (ipm != 1) return AHCI_DEV_NULL;

    switch (port->sig) {
        case 0xEB140101:
            return AHCI_DEV_SATAPI;
        case 0xC33C0101:
            return AHCI_DEV_SEMB;
        case 0x96690101:
            return AHCI_DEV_PM;
        default:
            return AHCI_DEV_SATA;
    }
}

static void start_cmd(HBA_PORT *port) {
    while (port->cmd & HBA_PORT_CMD_CR);
    port->cmd |= HBA_PORT_CMD_FRE;
    port->cmd |= HBA_PORT_CMD_ST;
}

static void stop_cmd(HBA_PORT *port) {
    port->cmd &= ~HBA_PORT_CMD_ST;
    port->cmd &= ~HBA_PORT_CMD_FRE;

    while (1) {
        if (port->cmd & HBA_PORT_CMD_FR) continue;
        if (port->cmd & HBA_PORT_CMD_CR) continue;
        break;
    }
}

int ahci_read(uint8_t port_num, uint64_t lba, uint32_t count, uint8_t *buf) {
    HBA_PORT *port = &ahci_base->ports[port_num];

    port->is = (uint32_t)-1;

    int slot = 0; // Find free command slot

    HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER*)((uint64_t)port->clb | ((uint64_t)port->clbu << 32));
    cmdheader += slot;
    cmdheader->cfl = sizeof(FIS_REG_H2D) / sizeof(uint32_t);
    cmdheader->w = 0;
    cmdheader->prdtl = (uint16_t)((count - 1) >> 4) + 1;

    HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL*)((uint64_t)cmdheader->ctba | ((uint64_t)cmdheader->ctbau << 32));

    int i;
    for (i = 0; i < cmdheader->prdtl - 1; i++) {
        HBA_PRDT_ENTRY *prdt = (HBA_PRDT_ENTRY*)&cmdtbl->prdt_entry[i];
        prdt->dba = (uint32_t)((uint64_t)buf & 0xFFFFFFFF);
        prdt->dbau = (uint32_t)(((uint64_t)buf >> 32) & 0xFFFFFFFF);
        prdt->dbc = 8 * 1024 - 1;
        prdt->i = 1;
        buf += 4 * 1024;
        count -= 16;
    }

    HBA_PRDT_ENTRY *prdt = (HBA_PRDT_ENTRY*)&cmdtbl->prdt_entry[i];
    prdt->dba = (uint32_t)((uint64_t)buf & 0xFFFFFFFF);
    prdt->dbau = (uint32_t)(((uint64_t)buf >> 32) & 0xFFFFFFFF);
    prdt->dbc = (count << 9) - 1;
    prdt->i = 1;

    FIS_REG_H2D *cmdfis = (FIS_REG_H2D*)(&cmdtbl->cfis);
    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1;
    cmdfis->command = 0x25; // READ DMA EXT

    cmdfis->lba0 = (uint8_t)lba;
    cmdfis->lba1 = (uint8_t)(lba >> 8);
    cmdfis->lba2 = (uint8_t)(lba >> 16);
    cmdfis->device = 1 << 6;

    cmdfis->lba3 = (uint8_t)(lba >> 24);
    cmdfis->lba4 = (uint8_t)(lba >> 32);
    cmdfis->lba5 = (uint8_t)(lba >> 40);

    cmdfis->countl = count & 0xFF;
    cmdfis->counth = (count >> 8) & 0xFF;

    // Wait for completion
    int spin = 0;
    while ((port->tfd & (0x80 | 0x08)) && spin < 1000000) {
        spin++;
    }

    if (spin == 1000000) {
        return 0;
    }

    port->ci = 1 << slot;

    while (1) {
        if ((port->ci & (1 << slot)) == 0) break;
        if (port->is & HBA_PxIS_TFES) {
            return 0;
        }
    }

    if (port->is & HBA_PxIS_TFES) {
        return 0;
    }

    return 1;
}

int ahci_write(uint8_t port_num, uint64_t lba, uint32_t count, uint8_t *buf) {
    HBA_PORT *port = &ahci_base->ports[port_num];

    port->is = (uint32_t)-1;

    int slot = 0; // Find free command slot

    HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER*)((uint64_t)port->clb | ((uint64_t)port->clbu << 32));
    cmdheader += slot;
    cmdheader->cfl = sizeof(FIS_REG_H2D) / sizeof(uint32_t);
    cmdheader->w = 1; // Write to device
    cmdheader->prdtl = (uint16_t)((count - 1) >> 4) + 1;

    HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL*)((uint64_t)cmdheader->ctba | ((uint64_t)cmdheader->ctbau << 32));

    int i;
    for (i = 0; i < cmdheader->prdtl - 1; i++) {
        HBA_PRDT_ENTRY *prdt = (HBA_PRDT_ENTRY*)&cmdtbl->prdt_entry[i];
        prdt->dba = (uint32_t)((uint64_t)buf & 0xFFFFFFFF);
        prdt->dbau = (uint32_t)(((uint64_t)buf >> 32) & 0xFFFFFFFF);
        prdt->dbc = 8 * 1024 - 1;
        prdt->i = 1;
        buf += 4 * 1024;
        count -= 16;
    }

    HBA_PRDT_ENTRY *prdt = (HBA_PRDT_ENTRY*)&cmdtbl->prdt_entry[i];
    prdt->dba = (uint32_t)((uint64_t)buf & 0xFFFFFFFF);
    prdt->dbau = (uint32_t)(((uint64_t)buf >> 32) & 0xFFFFFFFF);
    prdt->dbc = (count << 9) - 1;
    prdt->i = 1;

    FIS_REG_H2D *cmdfis = (FIS_REG_H2D*)(&cmdtbl->cfis);
    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1;
    cmdfis->command = 0x35; // WRITE DMA EXT

    cmdfis->lba0 = (uint8_t)lba;
    cmdfis->lba1 = (uint8_t)(lba >> 8);
    cmdfis->lba2 = (uint8_t)(lba >> 16);
    cmdfis->device = 1 << 6;

    cmdfis->lba3 = (uint8_t)(lba >> 24);
    cmdfis->lba4 = (uint8_t)(lba >> 32);
    cmdfis->lba5 = (uint8_t)(lba >> 40);

    cmdfis->countl = count & 0xFF;
    cmdfis->counth = (count >> 8) & 0xFF;

    // Wait for completion
    int spin = 0;
    while ((port->tfd & (0x80 | 0x08)) && spin < 1000000) {
        spin++;
    }

    if (spin == 1000000) {
        return 0;
    }

    port->ci = 1 << slot;

    while (1) {
        if ((port->ci & (1 << slot)) == 0) break;
        if (port->is & HBA_PxIS_TFES) {
            return 0;
        }
    }

    if (port->is & HBA_PxIS_TFES) {
        return 0;
    }

    return 1;
}
