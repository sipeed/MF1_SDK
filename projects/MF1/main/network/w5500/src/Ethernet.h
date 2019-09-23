/* Copyright 2018 Paul Stoffregen
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __ETHERNET_H
#define __ETHERNET_H

#include "utility/w5100.h"

enum EthernetLinkStatus
{
    Unknown,
    LinkON,
    LinkOFF
};

enum EthernetHardwareStatus
{
    EthernetNoHardware,
    EthernetW5100,
    EthernetW5200,
    EthernetW5500
};

int eth_w5100_begin_dhcp(uint8_t *mac, unsigned long timeout, unsigned long responseTimeout);

void eth_w5100_begin0(uint8_t *mac, IPAddress ip);
void eth_w5100_begin1(uint8_t *mac, IPAddress ip, IPAddress dns);
void eth_w5100_begin2(uint8_t *mac, IPAddress ip, IPAddress dns, IPAddress gateway);

void eth_w5100_begin3(uint8_t *mac, IPAddress ip, IPAddress dns, IPAddress gateway, IPAddress subnet);

enum EthernetLinkStatus eth_w5100_linkStatus();
enum EthernetHardwareStatus eth_w5100_hardwareStatus();
int eth_w5100_maintain();
void eth_w5100_MACAddress(uint8_t *mac_address);
IPAddress eth_w5100_localIP();
IPAddress eth_w5100_subnetMask();
IPAddress eth_w5100_gatewayIP();
IPAddress eth_w5100_dnsServerIP();
void eth_w5100_setMACAddress(const uint8_t *mac_address);
void eth_w5100_setLocalIP(const IPAddress local_ip);
void eth_w5100_setSubnetMask(const IPAddress subnet);
void eth_w5100_setGatewayIP(const IPAddress gateway);
void eth_w5100_setRetransmissionTimeout(uint16_t milliseconds);
void eth_w5100_setRetransmissionCount(uint8_t num);

#endif
