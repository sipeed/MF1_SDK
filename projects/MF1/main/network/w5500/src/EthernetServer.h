#ifndef __ETHERNETSERVER_H
#define __ETHERNETSERVER_H

#include "utility/w5100.h"

#include "EthernetClient.h"

typedef struct _eth_tcp_server eth_tcp_server_t;

typedef struct _eth_tcp_server
{
	uint16_t port;

	void (*begin)(eth_tcp_server_t *server);
	eth_tcp_client_t *(*available)(eth_tcp_server_t *server);

	eth_tcp_client_t *(*accept)(eth_tcp_server_t *server);
	size_t (*write)(eth_tcp_server_t *server, uint8_t *buffer, size_t size);

	void (*destory)(eth_tcp_server_t *server);

} eth_tcp_server_t;

eth_tcp_server_t *eth_tcp_server_new(uint16_t port);

#endif
