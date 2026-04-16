#include "network.h"
#include "../kernel/mm/mm.h"
#include <stdint.h>
#include <stddef.h>

static NET_INTERFACE net_if;
static uint8_t arp_table[256][10]; // IP -> MAC mapping
static uint32_t arp_table_count = 0;

// E1000 (Intel Gigabit Ethernet) registers
#define E1000_REG_CTRL     0x0000
#define E1000_REG_STATUS   0x0008
#define E1000_REG_EECD     0x0010
#define E1000_REG_EERD     0x0014
#define E1000_REG_CTRL_EXT 0x0018
#define E1000_REG_MDIC     0x0020
#define E1000_REG_FCAL     0x0028
#define E1000_REG_FCAH     0x002C
#define E1000_REG_FCT      0x0030
#define E1000_REG_VET      0x0038
#define E1000_REG_ICR      0x00C0
#define E1000_REG_ITR      0x00C4
#define E1000_REG_ICS      0x00C8
#define E1000_REG_IMS      0x00D0
#define E1000_REG_IMC      0x00D8
#define E1000_REG_RCTL     0x0100
#define E1000_REG_TCTL     0x0400
#define E1000_REG_RDBAL    0x2800
#define E1000_REG_RDBAH    0x2804
#define E1000_REG_RDLEN    0x2808
#define E1000_REG_RDH      0x2810
#define E1000_REG_RDT      0x2818
#define E1000_REG_TDBAL    0x3800
#define E1000_REG_TDBAH    0x3804
#define E1000_REG_TDLEN    0x3808
#define E1000_REG_TDH      0x3810
#define E1000_REG_TDT      0x3818
#define E1000_REG_MTA      0x5200
#define E1000_REG_RAL      0x5400
#define E1000_REG_RAH      0x5404

static volatile uint8_t *e1000_mmio = NULL;

typedef struct {
    uint64_t addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} __attribute__((packed)) e1000_rx_desc_t;

typedef struct {
    uint64_t addr;
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t status;
    uint8_t css;
    uint16_t special;
} __attribute__((packed)) e1000_tx_desc_t;

static e1000_rx_desc_t *rx_descs;
static e1000_tx_desc_t *tx_descs;
static uint8_t **rx_buffers;
static uint8_t **tx_buffers;
static uint16_t rx_cur = 0;
static uint16_t tx_cur = 0;

#define NUM_RX_DESC 32
#define NUM_TX_DESC 32
#define RX_BUFFER_SIZE 2048
#define TX_BUFFER_SIZE 2048

static void e1000_write(uint16_t reg, uint32_t value) {
    *((volatile uint32_t*)(e1000_mmio + reg)) = value;
}

static uint32_t e1000_read(uint16_t reg) {
    return *((volatile uint32_t*)(e1000_mmio + reg));
}

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

static void e1000_send_packet(uint8_t *data, uint32_t length) {
    // Copy data to TX buffer
    for (uint32_t i = 0; i < length; i++) {
        tx_buffers[tx_cur][i] = data[i];
    }

    // Set up descriptor
    tx_descs[tx_cur].addr = (uint64_t)tx_buffers[tx_cur];
    tx_descs[tx_cur].length = length;
    tx_descs[tx_cur].cmd = 0x0B; // EOP | IFCS | RS
    tx_descs[tx_cur].status = 0;

    // Update tail
    tx_cur = (tx_cur + 1) % NUM_TX_DESC;
    e1000_write(E1000_REG_TDT, tx_cur);
}

void network_init(void) {
    // Scan PCI for E1000 (Intel 8254x)
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint32_t vendor = pci_read(bus, slot, func, 0);
                if (vendor == 0xFFFFFFFF) continue;

                uint16_t vendor_id = vendor & 0xFFFF;
                uint16_t device_id = (vendor >> 16) & 0xFFFF;

                // Check for Intel E1000 devices
                if (vendor_id == 0x8086 &&
                    (device_id == 0x100E || device_id == 0x100F || device_id == 0x10D3)) {

                    // Found E1000
                    uint32_t bar0 = pci_read(bus, slot, func, 0x10);
                    e1000_mmio = (volatile uint8_t*)(uint64_t)(bar0 & 0xFFFFFFF0);

                    // Enable bus mastering
                    uint32_t cmd = pci_read(bus, slot, func, 0x04);
                    cmd |= 0x07; // Bus master + Memory space + I/O space
                    pci_write(bus, slot, func, 0x04, cmd);

                    goto found_e1000;
                }
            }
        }
    }
    return; // No network card found

found_e1000:
    // Read MAC address from EEPROM
    uint32_t ral = e1000_read(E1000_REG_RAL);
    uint32_t rah = e1000_read(E1000_REG_RAH);

    net_if.mac[0] = ral & 0xFF;
    net_if.mac[1] = (ral >> 8) & 0xFF;
    net_if.mac[2] = (ral >> 16) & 0xFF;
    net_if.mac[3] = (ral >> 24) & 0xFF;
    net_if.mac[4] = rah & 0xFF;
    net_if.mac[5] = (rah >> 8) & 0xFF;

    // Allocate RX descriptors and buffers
    rx_descs = (e1000_rx_desc_t*)kmalloc(NUM_RX_DESC * sizeof(e1000_rx_desc_t));
    rx_buffers = (uint8_t**)kmalloc(NUM_RX_DESC * sizeof(uint8_t*));

    for (int i = 0; i < NUM_RX_DESC; i++) {
        rx_buffers[i] = (uint8_t*)kmalloc(RX_BUFFER_SIZE);
        rx_descs[i].addr = (uint64_t)rx_buffers[i];
        rx_descs[i].status = 0;
    }

    // Allocate TX descriptors and buffers
    tx_descs = (e1000_tx_desc_t*)kmalloc(NUM_TX_DESC * sizeof(e1000_tx_desc_t));
    tx_buffers = (uint8_t**)kmalloc(NUM_TX_DESC * sizeof(uint8_t*));

    for (int i = 0; i < NUM_TX_DESC; i++) {
        tx_buffers[i] = (uint8_t*)kmalloc(TX_BUFFER_SIZE);
        tx_descs[i].addr = (uint64_t)tx_buffers[i];
        tx_descs[i].status = 0;
    }

    // Set up RX ring
    e1000_write(E1000_REG_RDBAL, (uint32_t)((uint64_t)rx_descs & 0xFFFFFFFF));
    e1000_write(E1000_REG_RDBAH, (uint32_t)(((uint64_t)rx_descs >> 32) & 0xFFFFFFFF));
    e1000_write(E1000_REG_RDLEN, NUM_RX_DESC * sizeof(e1000_rx_desc_t));
    e1000_write(E1000_REG_RDH, 0);
    e1000_write(E1000_REG_RDT, NUM_RX_DESC - 1);

    // Set up TX ring
    e1000_write(E1000_REG_TDBAL, (uint32_t)((uint64_t)tx_descs & 0xFFFFFFFF));
    e1000_write(E1000_REG_TDBAH, (uint32_t)(((uint64_t)tx_descs >> 32) & 0xFFFFFFFF));
    e1000_write(E1000_REG_TDLEN, NUM_TX_DESC * sizeof(e1000_tx_desc_t));
    e1000_write(E1000_REG_TDH, 0);
    e1000_write(E1000_REG_TDT, 0);

    // Enable RX
    e1000_write(E1000_REG_RCTL, (1 << 1) | (1 << 2) | (1 << 15) | (1 << 26)); // EN | SBP | BAM | BSEX

    // Enable TX
    e1000_write(E1000_REG_TCTL, (1 << 1) | (1 << 3)); // EN | PSP

    // Set send function
    net_if.send = e1000_send_packet;

    // Default IP configuration (will be overridden by DHCP)
    net_if.ip = (192 << 24) | (168 << 16) | (1 << 8) | 100;
    net_if.netmask = (255 << 24) | (255 << 16) | (255 << 8) | 0;
    net_if.gateway = (192 << 24) | (168 << 16) | (1 << 8) | 1;
}

uint16_t checksum(uint8_t *data, uint32_t length) {
    uint32_t sum = 0;
    uint16_t *ptr = (uint16_t*)data;

    while (length > 1) {
        sum += *ptr++;
        length -= 2;
    }

    if (length > 0) {
        sum += *(uint8_t*)ptr;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return ~sum;
}

void eth_send(uint8_t *dest_mac, uint16_t ethertype, uint8_t *data, uint32_t length) {
    uint8_t packet[1518];
    ETH_HEADER *eth = (ETH_HEADER*)packet;

    // Fill Ethernet header
    for (int i = 0; i < 6; i++) {
        eth->dest_mac[i] = dest_mac[i];
        eth->src_mac[i] = net_if.mac[i];
    }
    eth->ethertype = __builtin_bswap16(ethertype);

    // Copy data
    for (uint32_t i = 0; i < length; i++) {
        packet[sizeof(ETH_HEADER) + i] = data[i];
    }

    // Send packet
    if (net_if.send) {
        net_if.send(packet, sizeof(ETH_HEADER) + length);
    }
}

void ip_send(uint32_t dest_ip, uint8_t protocol, uint8_t *data, uint32_t length) {
    uint8_t packet[1500];
    IP_HEADER *ip = (IP_HEADER*)packet;

    ip->version_ihl = 0x45; // IPv4, 20 bytes header
    ip->tos = 0;
    ip->total_length = __builtin_bswap16(sizeof(IP_HEADER) + length);
    ip->id = 0;
    ip->flags_fragment = 0;
    ip->ttl = 64;
    ip->protocol = protocol;
    ip->checksum = 0;
    ip->src_ip = __builtin_bswap32(net_if.ip);
    ip->dest_ip = __builtin_bswap32(dest_ip);

    ip->checksum = checksum((uint8_t*)ip, sizeof(IP_HEADER));

    // Copy data
    for (uint32_t i = 0; i < length; i++) {
        packet[sizeof(IP_HEADER) + i] = data[i];
    }

    // ARP lookup for dest MAC
    uint8_t dest_mac[6];
    int found = 0;

    // Check if destination is on local network
    uint32_t dest_for_arp = dest_ip;
    if ((dest_ip & net_if.netmask) != (net_if.ip & net_if.netmask)) {
        // Use gateway for non-local destinations
        dest_for_arp = net_if.gateway;
    }

    // Search ARP table
    for (uint32_t i = 0; i < arp_table_count; i++) {
        uint32_t table_ip = (arp_table[i][0] << 24) | (arp_table[i][1] << 16) |
                           (arp_table[i][2] << 8) | arp_table[i][3];
        if (table_ip == dest_for_arp) {
            for (int j = 0; j < 6; j++) {
                dest_mac[j] = arp_table[i][4 + j];
            }
            found = 1;
            break;
        }
    }

    if (!found) {
        // Send ARP request and use broadcast for now
        arp_request(dest_for_arp);
        for (int i = 0; i < 6; i++) {
            dest_mac[i] = 0xFF;
        }
    }

    eth_send(dest_mac, ETH_TYPE_IP, packet, sizeof(IP_HEADER) + length);
}

void arp_add_entry(uint32_t ip, uint8_t *mac) {
    if (arp_table_count >= 256) return;

    arp_table[arp_table_count][0] = (ip >> 24) & 0xFF;
    arp_table[arp_table_count][1] = (ip >> 16) & 0xFF;
    arp_table[arp_table_count][2] = (ip >> 8) & 0xFF;
    arp_table[arp_table_count][3] = ip & 0xFF;

    for (int i = 0; i < 6; i++) {
        arp_table[arp_table_count][4 + i] = mac[i];
    }

    arp_table_count++;
}

void icmp_echo(uint32_t dest_ip, uint16_t id, uint16_t seq) {
    uint8_t packet[64];
    ICMP_HEADER *icmp = (ICMP_HEADER*)packet;

    icmp->type = 8; // Echo request
    icmp->code = 0;
    icmp->checksum = 0;
    icmp->id = __builtin_bswap16(id);
    icmp->sequence = __builtin_bswap16(seq);

    // Fill with data
    for (int i = 0; i < 56; i++) {
        packet[sizeof(ICMP_HEADER) + i] = 0x20 + i;
    }

    icmp->checksum = checksum(packet, sizeof(ICMP_HEADER) + 56);

    ip_send(dest_ip, 1, packet, sizeof(ICMP_HEADER) + 56); // Protocol 1 = ICMP
}

void network_receive(uint8_t *packet, uint32_t length) {
    ETH_HEADER *eth = (ETH_HEADER*)packet;
    uint16_t ethertype = __builtin_bswap16(eth->ethertype);

    if (ethertype == ETH_TYPE_ARP) {
        ARP_PACKET *arp = (ARP_PACKET*)(packet + sizeof(ETH_HEADER));
        uint16_t opcode = __builtin_bswap16(arp->opcode);

        if (opcode == 2) { // ARP reply
            uint32_t sender_ip = __builtin_bswap32(arp->sender_ip);
            arp_add_entry(sender_ip, arp->sender_mac);
        } else if (opcode == 1) { // ARP request
            uint32_t target_ip = __builtin_bswap32(arp->target_ip);
            if (target_ip == net_if.ip) {
                // Send ARP reply
                ARP_PACKET reply;
                reply.hw_type = __builtin_bswap16(1);
                reply.proto_type = __builtin_bswap16(ETH_TYPE_IP);
                reply.hw_size = 6;
                reply.proto_size = 4;
                reply.opcode = __builtin_bswap16(2); // Reply

                for (int i = 0; i < 6; i++) {
                    reply.sender_mac[i] = net_if.mac[i];
                    reply.target_mac[i] = arp->sender_mac[i];
                }

                reply.sender_ip = __builtin_bswap32(net_if.ip);
                reply.target_ip = arp->sender_ip;

                eth_send(arp->sender_mac, ETH_TYPE_ARP, (uint8_t*)&reply, sizeof(ARP_PACKET));
            }
        }
    } else if (ethertype == ETH_TYPE_IP) {
        IP_HEADER *ip = (IP_HEADER*)(packet + sizeof(ETH_HEADER));
        uint32_t dest_ip = __builtin_bswap32(ip->dest_ip);

        if (dest_ip == net_if.ip || dest_ip == 0xFFFFFFFF) {
            // Packet is for us
            uint8_t *ip_data = packet + sizeof(ETH_HEADER) + sizeof(IP_HEADER);
            uint16_t ip_data_len = __builtin_bswap16(ip->total_length) - sizeof(IP_HEADER);

            if (ip->protocol == 1) { // ICMP
                ICMP_HEADER *icmp = (ICMP_HEADER*)ip_data;
                if (icmp->type == 8) { // Echo request
                    // Send echo reply
                    icmp->type = 0;
                    icmp->checksum = 0;
                    icmp->checksum = checksum(ip_data, ip_data_len);

                    uint32_t src_ip = __builtin_bswap32(ip->src_ip);
                    ip_send(src_ip, 1, ip_data, ip_data_len);
                }
            } else if (ip->protocol == 6) { // TCP
                // TODO: Handle TCP
            } else if (ip->protocol == 17) { // UDP
                // TODO: Handle UDP
            }
        }
    }
}

void tcp_send(uint32_t dest_ip, uint16_t dest_port, uint16_t src_port,
              uint8_t flags, uint8_t *data, uint32_t length) {
    uint8_t packet[1500];
    TCP_HEADER *tcp = (TCP_HEADER*)packet;

    tcp->src_port = __builtin_bswap16(src_port);
    tcp->dest_port = __builtin_bswap16(dest_port);
    tcp->seq_num = 0;
    tcp->ack_num = 0;
    tcp->flags = __builtin_bswap16((5 << 12) | flags); // Data offset = 5 (20 bytes)
    tcp->window = __builtin_bswap16(65535);
    tcp->checksum = 0;
    tcp->urgent = 0;

    // Copy data
    for (uint32_t i = 0; i < length; i++) {
        packet[sizeof(TCP_HEADER) + i] = data[i];
    }

    // Calculate checksum with pseudo-header
    uint8_t pseudo[12 + sizeof(TCP_HEADER) + length];
    uint32_t *p = (uint32_t*)pseudo;
    p[0] = __builtin_bswap32(net_if.ip);
    p[1] = __builtin_bswap32(dest_ip);
    p[2] = __builtin_bswap32((IP_PROTO_TCP << 16) | (sizeof(TCP_HEADER) + length));

    for (uint32_t i = 0; i < sizeof(TCP_HEADER) + length; i++) {
        pseudo[12 + i] = packet[i];
    }

    tcp->checksum = checksum(pseudo, sizeof(pseudo));

    ip_send(dest_ip, IP_PROTO_TCP, packet, sizeof(TCP_HEADER) + length);
}

void udp_send(uint32_t dest_ip, uint16_t dest_port, uint16_t src_port,
              uint8_t *data, uint32_t length) {
    uint8_t packet[1500];
    UDP_HEADER *udp = (UDP_HEADER*)packet;

    udp->src_port = __builtin_bswap16(src_port);
    udp->dest_port = __builtin_bswap16(dest_port);
    udp->length = __builtin_bswap16(sizeof(UDP_HEADER) + length);
    udp->checksum = 0;

    // Copy data
    for (uint32_t i = 0; i < length; i++) {
        packet[sizeof(UDP_HEADER) + i] = data[i];
    }

    ip_send(dest_ip, IP_PROTO_UDP, packet, sizeof(UDP_HEADER) + length);
}

void arp_request(uint32_t ip) {
    ARP_PACKET arp;

    arp.hw_type = __builtin_bswap16(1); // Ethernet
    arp.proto_type = __builtin_bswap16(ETH_TYPE_IP);
    arp.hw_size = 6;
    arp.proto_size = 4;
    arp.opcode = __builtin_bswap16(1); // Request

    for (int i = 0; i < 6; i++) {
        arp.sender_mac[i] = net_if.mac[i];
        arp.target_mac[i] = 0;
    }

    arp.sender_ip = __builtin_bswap32(net_if.ip);
    arp.target_ip = __builtin_bswap32(ip);

    uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    eth_send(broadcast, ETH_TYPE_ARP, (uint8_t*)&arp, sizeof(ARP_PACKET));
}
