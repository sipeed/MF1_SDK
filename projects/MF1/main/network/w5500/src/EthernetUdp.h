#ifndef __ETHERNET_UDP_H
#define __ETHERNET_UDP_H

#include "utility/w5100.h"

#define UDP_TX_PACKET_MAX_SIZE 24

typedef struct _eth_udp eth_udp_t;

typedef struct _eth_udp
{
    uint16_t _port;       // local port to listen on
    IPAddress _remoteIP;  // remote IP address for the incoming packet whilst it's being processed
    uint16_t _remotePort; // remote port for the incoming packet whilst it's being processed
    uint16_t _offset;     // offset into the packet being sent

    uint8_t sockindex;
    uint16_t _remaining; // remaining bytes of incoming packet yet to be processed

    uint8_t (*begin)(eth_udp_t *udp, uint16_t port);
    int (*available)(eth_udp_t *udp);
    void (*stop)(eth_udp_t *udp);
    int (*beginPacket_domain)(eth_udp_t *udp, const char *host, uint16_t port);
    int (*beginPacket_ip)(eth_udp_t *udp, IPAddress ip, uint16_t port);
    int (*endPacket)(eth_udp_t *udp);
    size_t (*write)(eth_udp_t *udp, const uint8_t *buffer, size_t size);
    int (*parsePacket)(eth_udp_t *udp);
    int (*read_one)(eth_udp_t *udp);
    int (*read)(eth_udp_t *udp, unsigned char *buffer, size_t len);
    int (*peek)(eth_udp_t *udp);
    void (*flush)(eth_udp_t *udp);
    uint8_t (*beginMulticast)(eth_udp_t *udp, IPAddress ip, uint16_t port);
    uint16_t (*remote_port)(eth_udp_t *udp);
    void (*remote_ip)(eth_udp_t *udp, IPAddress *ip);

    void (*destory)(eth_udp_t *udp);
} eth_udp_t;

eth_udp_t *eth_udp_new(void);

#endif
