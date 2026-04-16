#ifndef NVME_H
#define NVME_H

#include <stdint.h>

// NVMe Register Offsets
#define NVME_REG_CAP    0x00
#define NVME_REG_VS     0x08
#define NVME_REG_CC     0x14
#define NVME_REG_CSTS   0x1C
#define NVME_REG_AQA    0x24
#define NVME_REG_ASQ    0x28
#define NVME_REG_ACQ    0x30

// Controller Configuration
#define NVME_CC_ENABLE  (1 << 0)
#define NVME_CC_IOSQES  (6 << 16)
#define NVME_CC_IOCQES  (4 << 20)

// Controller Status
#define NVME_CSTS_RDY   (1 << 0)

// Admin Commands
#define NVME_ADMIN_CREATE_SQ    0x01
#define NVME_ADMIN_CREATE_CQ    0x05
#define NVME_ADMIN_IDENTIFY     0x06

// NVM Commands
#define NVME_CMD_READ   0x02
#define NVME_CMD_WRITE  0x01

typedef struct {
    uint64_t cap;
    uint32_t vs;
    uint32_t intms;
    uint32_t intmc;
    uint32_t cc;
    uint32_t reserved1;
    uint32_t csts;
    uint32_t nssr;
    uint32_t aqa;
    uint64_t asq;
    uint64_t acq;
} __attribute__((packed)) NVME_REGS;

typedef struct {
    uint32_t cdw0;
    uint32_t nsid;
    uint64_t reserved;
    uint64_t mptr;
    uint64_t prp1;
    uint64_t prp2;
    uint32_t cdw10;
    uint32_t cdw11;
    uint32_t cdw12;
    uint32_t cdw13;
    uint32_t cdw14;
    uint32_t cdw15;
} __attribute__((packed)) NVME_COMMAND;

typedef struct {
    uint32_t dw0;
    uint32_t dw1;
    uint16_t sq_head;
    uint16_t sq_id;
    uint16_t cid;
    uint16_t status;
} __attribute__((packed)) NVME_COMPLETION;

typedef struct {
    NVME_COMMAND *sq;
    NVME_COMPLETION *cq;
    uint16_t sq_tail;
    uint16_t cq_head;
    uint16_t sq_size;
    uint16_t cq_size;
    uint8_t phase;
} NVME_QUEUE;

// Function prototypes
void nvme_init(void);
int nvme_read(uint32_t nsid, uint64_t lba, uint32_t count, uint8_t *buf);
int nvme_write(uint32_t nsid, uint64_t lba, uint32_t count, uint8_t *buf);

#endif
