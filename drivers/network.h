#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>

// Ethernet
#define ETH_TYPE_IP     0x0800
#define ETH_TYPE_ARP    0x0806

// IP
#define IP_PROTO_ICMP   1
#define IP_PROTO_TCP    6
#define IP_PROTO_UDP    17

// TCP Flags
#define TCP_FIN         0x01
#define TCP_SYN         0x02
#define TCP_RST         0x04
#define TCP_PSH         0x08
#define TCP_ACK         0x10
#define TCP_URG         0x20

typedef struct {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
} __attribute__((packed)) ETH_HEADER;

typedef struct {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t total_length;
    uint16_t id;
    uint16_t flags_fragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
} __attribute__((packed)) IP_HEADER;

typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint16_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent;
} __attribute__((packed)) TCP_HEADER;

typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) UDP_HEADER;

typedef struct {
    uint16_t hw_type;
    uint16_t proto_type;
    uint8_t hw_size;
    uint8_t proto_size;
    uint16_t opcode;
    uint8_t sender_mac[6];
    uint32_t sender_ip;
    uint8_t target_mac[6];
    uint32_t target_ip;
} __attribute__((packed)) ARP_PACKET;

// Network interface
typedef struct {
    uint8_t mac[6];
    uint32_t ip;
    uint32_t netmask;
    uint32_t gateway;
    void (*send)(uint8_t *packet, uint32_t length);
    void (*recv)(uint8_t *packet, uint32_t length);
} NET_INTERFACE;

// TCP Connection
typedef struct {
    uint32_t remote_ip;
    uint16_t local_port;
    uint16_t remote_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t state;
} TCP_CONNECTION;

// Function prototypes
void network_init(void);
void eth_send(uint8_t *dest_mac, uint16_t ethertype, uint8_t *data, uint32_t length);
void ip_send(uint32_t dest_ip, uint8_t protocol, uint8_t *data, uint32_t length);
void tcp_send(uint32_t dest_ip, uint16_t dest_port, uint16_t src_port, uint8_t flags, uint8_t *data, uint32_t length);
void udp_send(uint32_t dest_ip, uint16_t dest_port, uint16_t src_port, uint8_t *data, uint32_t length);
void arp_request(uint32_t ip);
uint16_t checksum(uint8_t *data, uint32_t length);

#endif
