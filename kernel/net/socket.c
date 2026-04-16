#include "socket.h"
#include "../mm/mm.h"
#include <stdint.h>

static socket_t sockets[1024];
static int next_socket_id = 0;

int socket(int domain, int type, int protocol) {
    // Find free socket
    int sockfd = -1;
    for (int i = 0; i < 1024; i++) {
        if (sockets[i].state == TCP_CLOSED) {
            sockfd = i;
            break;
        }
    }

    if (sockfd < 0) return -1;

    // Initialize socket
    socket_t *sock = &sockets[sockfd];
    sock->type = type;
    sock->protocol = protocol;
    sock->state = TCP_CLOSED;
    sock->local_ip = 0;
    sock->local_port = 0;
    sock->remote_ip = 0;
    sock->remote_port = 0;

    return sockfd;
}

int bind(int sockfd, const sockaddr_t *addr, uint32_t addrlen) {
    if (sockfd < 0 || sockfd >= 1024) return -1;

    socket_t *sock = &sockets[sockfd];
    sockaddr_in_t *addr_in = (sockaddr_in_t*)addr;

    sock->local_ip = addr_in->sin_addr;
    sock->local_port = ntohs(addr_in->sin_port);

    return 0;
}

int listen(int sockfd, int backlog) {
    if (sockfd < 0 || sockfd >= 1024) return -1;

    socket_t *sock = &sockets[sockfd];
    sock->state = TCP_LISTEN;
    sock->backlog = backlog;

    return 0;
}

int connect(int sockfd, const sockaddr_t *addr, uint32_t addrlen) {
    if (sockfd < 0 || sockfd >= 1024) return -1;

    socket_t *sock = &sockets[sockfd];
    sockaddr_in_t *addr_in = (sockaddr_in_t*)addr;

    sock->remote_ip = addr_in->sin_addr;
    sock->remote_port = ntohs(addr_in->sin_port);

    // Send SYN
    tcp_send(sock->remote_ip, sock->remote_port, sock->local_port, TCP_SYN, NULL, 0);
    sock->state = TCP_SYN_SENT;

    // Wait for SYN-ACK (simplified)
    // TODO: Implement proper state machine

    return 0;
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
    if (sockfd < 0 || sockfd >= 1024) return -1;

    socket_t *sock = &sockets[sockfd];

    if (sock->type == SOCK_STREAM) {
        // TCP
        tcp_send(sock->remote_ip, sock->remote_port, sock->local_port,
                TCP_PSH | TCP_ACK, (uint8_t*)buf, len);
    } else if (sock->type == SOCK_DGRAM) {
        // UDP
        udp_send(sock->remote_ip, sock->remote_port, sock->local_port,
                (uint8_t*)buf, len);
    }

    return len;
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    if (sockfd < 0 || sockfd >= 1024) return -1;

    socket_t *sock = &sockets[sockfd];

    // TODO: Implement receive queue
    return 0;
}

// Network byte order conversion
uint16_t htons(uint16_t hostshort) {
    return ((hostshort & 0xFF) << 8) | ((hostshort >> 8) & 0xFF);
}

uint16_t ntohs(uint16_t netshort) {
    return htons(netshort);
}

uint32_t htonl(uint32_t hostlong) {
    return ((hostlong & 0xFF) << 24) |
           ((hostlong & 0xFF00) << 8) |
           ((hostlong & 0xFF0000) >> 8) |
           ((hostlong & 0xFF000000) >> 24);
}

uint32_t ntohl(uint32_t netlong) {
    return htonl(netlong);
}

// DNS resolver
uint32_t dns_resolve(const char *hostname) {
    // Simple DNS query
    uint8_t query[512];
    dns_header_t *header = (dns_header_t*)query;

    header->id = 0x1234;
    header->flags = htons(0x0100); // Standard query
    header->qdcount = htons(1);
    header->ancount = 0;
    header->nscount = 0;
    header->arcount = 0;

    // Build question
    uint8_t *qname = query + sizeof(dns_header_t);
    int i = 0, j = 0;
    int len_pos = 0;

    while (hostname[i]) {
        if (hostname[i] == '.') {
            qname[len_pos] = j;
            len_pos = i + 1;
            j = 0;
        } else {
            qname[i + 1] = hostname[i];
            j++;
        }
        i++;
    }
    qname[len_pos] = j;
    qname[i + 1] = 0;

    // Type A, Class IN
    uint16_t *qtype = (uint16_t*)(qname + i + 2);
    *qtype = htons(1);
    uint16_t *qclass = (uint16_t*)(qname + i + 4);
    *qclass = htons(1);

    // Send to DNS server (8.8.8.8)
    uint32_t dns_server = (8 << 24) | (8 << 16) | (8 << 8) | 8;
    udp_send(dns_server, 53, 12345, query, sizeof(dns_header_t) + i + 6);

    // TODO: Wait for response and parse

    return 0;
}

// DHCP client
int dhcp_discover(const char *interface) {
    dhcp_packet_t packet;

    packet.op = 1; // Boot request
    packet.htype = 1; // Ethernet
    packet.hlen = 6;
    packet.hops = 0;
    packet.xid = 0x12345678;
    packet.secs = 0;
    packet.flags = htons(0x8000); // Broadcast
    packet.ciaddr = 0;
    packet.yiaddr = 0;
    packet.siaddr = 0;
    packet.giaddr = 0;

    // Set MAC address
    // TODO: Get from interface

    packet.magic = htonl(0x63825363);

    // Options
    uint8_t *opt = packet.options;
    *opt++ = 53; // DHCP Message Type
    *opt++ = 1;
    *opt++ = 1; // DHCP Discover

    *opt++ = 55; // Parameter Request List
    *opt++ = 3;
    *opt++ = 1; // Subnet Mask
    *opt++ = 3; // Router
    *opt++ = 6; // DNS

    *opt++ = 255; // End

    // Broadcast
    uint32_t broadcast = 0xFFFFFFFF;
    udp_send(broadcast, 67, 68, (uint8_t*)&packet, sizeof(packet));

    return 0;
}

uint32_t inet_addr(const char *cp) {
    uint32_t addr = 0;
    int shift = 24;

    while (*cp) {
        int val = 0;
        while (*cp >= '0' && *cp <= '9') {
            val = val * 10 + (*cp - '0');
            cp++;
        }
        addr |= (val << shift);
        shift -= 8;
        if (*cp == '.') cp++;
    }

    return addr;
}
