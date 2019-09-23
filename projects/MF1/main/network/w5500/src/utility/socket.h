#ifndef __SOCKET_H
#define __SOCKET_H

#include "utility/w5100.h"

extern uint16_t eth_server_port[MAX_SOCK_NUM];

void W5100_socketPortRand(uint16_t n);
uint8_t W5100_socketBegin(uint8_t protocol, uint16_t port);
uint8_t W5100_socketBeginMulticast(uint8_t protocol, IPAddress ip, uint16_t port);
void W5100_socketClose(uint8_t s);
uint8_t W5100_socketListen(uint8_t s);
void W5100_socketConnect(uint8_t s, uint8_t *addr, uint16_t port);
void W5100_socketDisconnect(uint8_t s);
int W5100_socketRecv(uint8_t s, uint8_t *buf, int16_t len);
uint16_t W5100_socketRecvAvailable(uint8_t s);
uint8_t W5100_socketPeek(uint8_t s);
uint16_t W5100_socketSend(uint8_t s, const uint8_t *buf, uint16_t len);
uint16_t W5100_socketSendAvailable(uint8_t s);
uint16_t W5100_socketBufferData(uint8_t s, uint16_t offset, const uint8_t *buf, uint16_t len);
bool W5100_socketStartUDP(uint8_t s, uint8_t *addr, uint16_t port);
bool W5100_socketSendUDP(uint8_t s);

uint8_t W5100_socketStatus(uint8_t s);

#endif
