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
#include "Ethernet.h"
#include "Dhcp.h"
#include "socket.h"

static IPAddress _dnsServerAddress;
///////////////////////////////////////////////////////////////////////////////
int eth_w5100_begin_dhcp(uint8_t *mac, unsigned long timeout, unsigned long responseTimeout)
{
    IPAddress zero;
    zero.dword = 0;

    // Initialise the basic info
    if (W5100_init(W5100) == 0)
        return 0;

    W5100_setMACAddress(mac);
    W5100_setIPAddress(zero.bytes);

    printk("start with dhcp,please wait get ip\r\n");

    dhcp_init(dhcp);

    // Now try to get our config info from a DHCP server
    int ret = dhcp->beginWithDHCP(mac, timeout, responseTimeout);
    if (ret == 1)
    {
        // We've successfully found a DHCP server and got our configuration
        // info, so set things accordingly
        W5100_setIPAddress(dhcp->getLocalIp().bytes);
        W5100_setGatewayIp(dhcp->getGatewayIp().bytes);
        W5100_setSubnetMask(dhcp->getSubnetMask().bytes);
        _dnsServerAddress = dhcp->getDnsServerIp();
        W5100_socketPortRand((uint16_t)(millis() / 1000));
    }
    return ret;
}
///////////////////////////////////////////////////////////////////////////////
void eth_w5100_begin0(uint8_t *mac, IPAddress ip)
{
    // Assume the DNS server will be the machine on the same network as the local IP
    // but with last octet being '1'
    IPAddress dns;

    dns.dword = ip.dword;
    dns.bytes[3] = 1;

    eth_w5100_begin1(mac, ip, dns);
}

void eth_w5100_begin1(uint8_t *mac, IPAddress ip, IPAddress dns)
{
    // Assume the gateway will be the machine on the same network as the local IP
    // but with last octet being '1'
    IPAddress gateway;
    gateway.dword = ip.dword;
    gateway.bytes[3] = 1;
    eth_w5100_begin2(mac, ip, dns, gateway);
}

void eth_w5100_begin2(uint8_t *mac, IPAddress ip, IPAddress dns, IPAddress gateway)
{
    IPAddress subnet;
    subnet.bytes[0] = 255;
    subnet.bytes[1] = 255;
    subnet.bytes[2] = 255;
    subnet.bytes[3] = 0;
    eth_w5100_begin3(mac, ip, dns, gateway, subnet);
}

void eth_w5100_begin3(uint8_t *mac, IPAddress ip, IPAddress dns, IPAddress gateway, IPAddress subnet)
{
    if (W5100_init(W5100) == 0)
        return;

    printk("start with static ip\r\n");

    W5100_setMACAddress(mac);
    W5100_setIPAddress(ip.bytes);
    W5100_setGatewayIp(gateway.bytes);
    W5100_setSubnetMask(subnet.bytes);
    _dnsServerAddress.dword = dns.dword;
    return;
}

///////////////////////////////////////////////////////////////////////////////

enum EthernetLinkStatus eth_w5100_linkStatus()
{
    switch (W5100->getLinkStatus())
    {
    case UNKNOWN:
        return Unknown;
    case LINK_ON:
        return LinkON;
    case LINK_OFF:
        return LinkOFF;
    default:
        return Unknown;
    }
}

enum EthernetHardwareStatus eth_w5100_hardwareStatus()
{
    switch (W5100->chip)
    {
    case 51:
        return EthernetW5100;
    case 52:
        return EthernetW5200;
    case 55:
        return EthernetW5500;
    default:
        return EthernetNoHardware;
    }
}

int eth_w5100_maintain()
{
    int rc = DHCP_CHECK_NONE;
    if (dhcp != NULL)
    {
        // we have a pointer to dhcp, use it
        rc = dhcp->checkLease();
        switch (rc)
        {
        case DHCP_CHECK_NONE:
            //nothing done
            break;
        case DHCP_CHECK_RENEW_OK:
        case DHCP_CHECK_REBIND_OK:
            //we might have got a new IP.
            W5100_setIPAddress(dhcp->getLocalIp().bytes);
            W5100_setGatewayIp(dhcp->getGatewayIp().bytes);
            W5100_setSubnetMask(dhcp->getSubnetMask().bytes);
            _dnsServerAddress = dhcp->getDnsServerIp();
            break;
        default:
            //this is actually an error, it will retry though
            break;
        }
    }
    return rc;
}

void eth_w5100_MACAddress(uint8_t *mac_address)
{
    W5100_getMACAddress(mac_address);
}

IPAddress eth_w5100_localIP()
{
    IPAddress ret;
    W5100_getIPAddress(ret.bytes);
    return ret;
}

IPAddress eth_w5100_subnetMask()
{
    IPAddress ret;
    W5100_getSubnetMask(ret.bytes);
    return ret;
}

IPAddress eth_w5100_gatewayIP()
{
    IPAddress ret;
    W5100_getGatewayIp(ret.bytes);
    return ret;
}

IPAddress eth_w5100_dnsServerIP()
{
    return _dnsServerAddress;
}

void eth_w5100_setMACAddress(const uint8_t *mac_address)
{
    W5100_setMACAddress(mac_address);
}

void eth_w5100_setLocalIP(const IPAddress local_ip)
{
    IPAddress ip;
    ip.dword = local_ip.dword;
    W5100_setIPAddress(ip.bytes);
}

void eth_w5100_setSubnetMask(const IPAddress subnet)
{
    IPAddress ip;
    ip.dword = subnet.dword;
    W5100_setSubnetMask(ip.bytes);
}

void eth_w5100_setGatewayIP(const IPAddress gateway)
{
    IPAddress ip;
    ip.dword = gateway.dword;
    W5100_setGatewayIp(ip.bytes);
}

void eth_w5100_setRetransmissionTimeout(uint16_t milliseconds)
{
    if (milliseconds > 6553)
        milliseconds = 6553;
    W5100_setRetransmissionTime(milliseconds * 10);
}

void eth_w5100_setRetransmissionCount(uint8_t num)
{
    W5100_setRetransmissionCount(num);
}
