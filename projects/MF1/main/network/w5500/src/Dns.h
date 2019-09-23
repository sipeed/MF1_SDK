// Arduino DNS client for WizNet5100-based Ethernet shield
// (c) Copyright 2009-2010 MCQN Ltd.
// Released under Apache License, version 2.0

#ifndef DNSClient_h
#define DNSClient_h

#include "utility/w5100.h"

void eth_dns_begin(const IPAddress *DNSServer);
int eth_dns_inet_aton(const char *address, IPAddress *result);
int eth_dns_getHostByName(const char *Hostname, IPAddress *Result, uint16_t timeout);

#endif
