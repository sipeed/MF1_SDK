#ifndef __ETHERNETCLIENT_H
#define __ETHERNETCLIENT_H

#include "utility/w5100.h"

#define TCP_CLIENT_DEFAULT_TIMEOUT 10000

typedef struct _eth_tcp_client eth_tcp_client_t;

typedef struct _eth_tcp_client
{
    uint8_t sockindex; // MAX_SOCK_NUM means client not in use
    uint16_t _timeout; //default 10000

    int (*connect_domain)(eth_tcp_client_t *client, const char *host, uint16_t port);
    int (*connect_ip)(eth_tcp_client_t *client, IPAddress ip, uint16_t port);

    int (*availableForWrite)(eth_tcp_client_t *client);
    size_t (*write)(eth_tcp_client_t *client, const uint8_t *buf, size_t size);
    size_t (*write_fix)(eth_tcp_client_t *client, const uint8_t *buf, size_t size, uint16_t max_wait_ms);

    int (*available)(eth_tcp_client_t *client);
    int (*read_one)(eth_tcp_client_t *client);
    int (*read)(eth_tcp_client_t *client, uint8_t *buf, size_t size);

    int (*peek)(eth_tcp_client_t *client);
    void (*flush)(eth_tcp_client_t *client);
    void (*stop)(eth_tcp_client_t *client);

    uint8_t (*connected)(eth_tcp_client_t *client);
    uint8_t (*status)(eth_tcp_client_t *client);

    uint16_t (*localPort)(eth_tcp_client_t *client);
    IPAddress (*remoteIP)(eth_tcp_client_t *client);
    uint16_t (*remotePort)(eth_tcp_client_t *client);

    void (*destory)(eth_tcp_client_t *client);
} eth_tcp_client_t;

eth_tcp_client_t *eth_tcp_client_new(uint8_t sock, uint16_t timeout);

#endif
