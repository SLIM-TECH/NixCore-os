#ifndef SOCKET_H
#define SOCKET_H

#include <stdint.h>
#include <stddef.h>

// Socket types
#define SOCK_STREAM     1
#define SOCK_DGRAM      2
#define SOCK_RAW        3

// Address families
#define AF_INET         2
#define AF_INET6        10
#define AF_UNIX         1

// Protocol families
#define PF_INET         AF_INET
#define PF_INET6        AF_INET6
#define PF_UNIX         AF_UNIX

// Protocols
#define IPPROTO_IP      0
#define IPPROTO_ICMP    1
#define IPPROTO_TCP     6
#define IPPROTO_UDP     17

// Socket options
#define SOL_SOCKET      1
#define SO_REUSEADDR    2
#define SO_KEEPALIVE    9
#define SO_BROADCAST    6
#define SO_RCVBUF       8
#define SO_SNDBUF       7

// Shutdown modes
#define SHUT_RD         0
#define SHUT_WR         1
#define SHUT_RDWR       2

// sockaddr structures
typedef struct {
    uint16_t sin_family;
    uint16_t sin_port;
    uint32_t sin_addr;
    uint8_t sin_zero[8];
} sockaddr_in_t;

typedef struct {
    uint16_t sa_family;
    char sa_data[14];
} sockaddr_t;

// Socket structure
typedef struct socket {
    int type;
    int protocol;
    int state;
    uint32_t local_ip;
    uint16_t local_port;
    uint32_t remote_ip;
    uint16_t remote_port;
    void *recv_queue;
    void *send_queue;
    int backlog;
    struct socket *accept_queue;
    struct socket *parent;
    void *private_data;
} socket_t;

// TCP states
#define TCP_CLOSED          0
#define TCP_LISTEN          1
#define TCP_SYN_SENT        2
#define TCP_SYN_RECEIVED    3
#define TCP_ESTABLISHED     4
#define TCP_FIN_WAIT_1      5
#define TCP_FIN_WAIT_2      6
#define TCP_CLOSE_WAIT      7
#define TCP_CLOSING         8
#define TCP_LAST_ACK        9
#define TCP_TIME_WAIT       10

// DHCP
typedef struct {
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint32_t ciaddr;
    uint32_t yiaddr;
    uint32_t siaddr;
    uint32_t giaddr;
    uint8_t chaddr[16];
    uint8_t sname[64];
    uint8_t file[128];
    uint32_t magic;
    uint8_t options[312];
} __attribute__((packed)) dhcp_packet_t;

// DNS
typedef struct {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} __attribute__((packed)) dns_header_t;

// Socket API (POSIX-compatible)
int socket(int domain, int type, int protocol);
int bind(int sockfd, const sockaddr_t *addr, uint32_t addrlen);
int listen(int sockfd, int backlog);
int accept(int sockfd, sockaddr_t *addr, uint32_t *addrlen);
int connect(int sockfd, const sockaddr_t *addr, uint32_t addrlen);
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const sockaddr_t *dest_addr, uint32_t addrlen);
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 sockaddr_t *src_addr, uint32_t *addrlen);
int shutdown(int sockfd, int how);
int setsockopt(int sockfd, int level, int optname, const void *optval, uint32_t optlen);
int getsockopt(int sockfd, int level, int optname, void *optval, uint32_t *optlen);

// DHCP client
int dhcp_discover(const char *interface);
int dhcp_request(const char *interface, uint32_t offered_ip);
void dhcp_release(const char *interface);

// DNS resolver
uint32_t dns_resolve(const char *hostname);
int dns_reverse_lookup(uint32_t ip, char *hostname, size_t len);

// Network utilities
uint32_t inet_addr(const char *cp);
char *inet_ntoa(uint32_t addr);
uint16_t htons(uint16_t hostshort);
uint16_t ntohs(uint16_t netshort);
uint32_t htonl(uint32_t hostlong);
uint32_t ntohl(uint32_t netlong);

#endif
