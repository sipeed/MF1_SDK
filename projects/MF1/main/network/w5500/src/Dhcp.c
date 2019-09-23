// DHCP Library v0.3 - April 25, 2009
// Author: Jordan Terrell - blog.jordanterrell.com


#include "Dhcp.h"

#include <stddef.h>
#include <string.h>

#include "utility/util.h"

#define TAG "DHCP"

///////////////////////////////////////////////////////////////////////////////
static dhcp_t s_dhcp = {
    .udp = NULL,
    .getLocalIp = NULL,
    .getSubnetMask = NULL,
    .getGatewayIp = NULL,
    .getDhcpServerIp = NULL,
    .getDnsServerIp = NULL,
    .checkLease = NULL,
    .beginWithDHCP = NULL,
};

dhcp_t *dhcp = &s_dhcp;

///////////////////////////////////////////////////////////////////////////////
static uint32_t _dhcpInitialTransactionId;
static uint32_t _dhcpTransactionId;

static uint8_t _dhcpMacAddr[6];
static uint8_t _dhcpLocalIp[4] __attribute__((aligned(4)));
static uint8_t _dhcpSubnetMask[4] __attribute__((aligned(4)));
static uint8_t _dhcpGatewayIp[4] __attribute__((aligned(4)));
static uint8_t _dhcpDhcpServerIp[4] __attribute__((aligned(4)));
static uint8_t _dhcpDnsServerIp[4] __attribute__((aligned(4)));

static uint32_t _dhcpLeaseTime;
static uint32_t _dhcpT1, _dhcpT2;
static uint32_t _renewInSec;
static uint32_t _rebindInSec;
static unsigned long _timeout;
static unsigned long _responseTimeout;
static unsigned long _lastCheckLeaseMillis;
static uint8_t _dhcp_state;
///////////////////////////////////////////////////////////////////////////////
static void reset_DHCP_lease(void);
static int request_DHCP_lease(void);
static void presend_DHCP(void);
static void send_DHCP_MESSAGE(uint8_t messageType, uint16_t secondsElapsed);
static uint8_t parseDHCPResponse(unsigned long responseTimeout, uint32_t *transactionId);
static void printByte(char *buf, uint8_t n);
///////////////////////////////////////////////////////////////////////////////

static int dhcp_beginWithDHCP(uint8_t *mac, unsigned long timeout, unsigned long responseTimeout)
{
    _dhcpLeaseTime = 0;
    _dhcpT1 = 0;
    _dhcpT2 = 0;
    _timeout = timeout;
    _responseTimeout = responseTimeout;

    // zero out _dhcpMacAddr
    memset(_dhcpMacAddr, 0, 6);
    reset_DHCP_lease();

    memcpy((void *)_dhcpMacAddr, (void *)mac, 6);
    _dhcp_state = STATE_DHCP_START;

    return request_DHCP_lease();
}

static void reset_DHCP_lease(void)
{
    // zero out _dhcpSubnetMask, _dhcpGatewayIp, _dhcpLocalIp, _dhcpDhcpServerIp, _dhcpDnsServerIp
    memset(_dhcpLocalIp, 0, 4);
    memset(_dhcpSubnetMask, 0, 4);
    memset(_dhcpGatewayIp, 0, 4);
    memset(_dhcpDhcpServerIp, 0, 4);
    memset(_dhcpDnsServerIp, 0, 4);
    return;
}

//return:0 on error, 1 if request is sent and response is received
static int request_DHCP_lease(void)
{
    uint8_t messageType = 0;

    // Pick an initial transaction ID
    uint64_t tim = sysctl_get_time_us();
    _dhcpTransactionId = (uint16_t)(tim & 2000UL) + 1; //random(1UL, 2000UL);
    _dhcpInitialTransactionId = _dhcpTransactionId;

    dhcp->udp->stop(dhcp->udp);
    if (dhcp->udp->begin(dhcp->udp, DHCP_CLIENT_PORT) == 0)
    {
        // Couldn't get a socket
        DEBUG("Couldn't get a socket\r\n");
        return 0;
    }

    presend_DHCP();

    int result = 0;

    unsigned long startTime = millis();

    while (_dhcp_state != STATE_DHCP_LEASED)
    {
        if (_dhcp_state == STATE_DHCP_START)
        {
            _dhcpTransactionId++;
            send_DHCP_MESSAGE(DHCP_DISCOVER, ((millis() - startTime) / 1000));
            _dhcp_state = STATE_DHCP_DISCOVER;
        }
        else if (_dhcp_state == STATE_DHCP_REREQUEST)
        {
            _dhcpTransactionId++;
            send_DHCP_MESSAGE(DHCP_REQUEST, ((millis() - startTime) / 1000));
            _dhcp_state = STATE_DHCP_REQUEST;
        }
        else if (_dhcp_state == STATE_DHCP_DISCOVER)
        {
            uint32_t respId = 0;
            messageType = parseDHCPResponse(_responseTimeout, &respId);
            if (messageType == DHCP_OFFER)
            {
                // We'll use the transaction ID that the offer came with,
                // rather than the one we were up to
                _dhcpTransactionId = respId;
                send_DHCP_MESSAGE(DHCP_REQUEST, ((millis() - startTime) / 1000));
                _dhcp_state = STATE_DHCP_REQUEST;
            }
        }
        else if (_dhcp_state == STATE_DHCP_REQUEST)
        {
            uint32_t respId;
            messageType = parseDHCPResponse(_responseTimeout, &respId);
            if (messageType == DHCP_ACK)
            {
                _dhcp_state = STATE_DHCP_LEASED;
                result = 1;
                //use default lease time if we didn't get it
                if (_dhcpLeaseTime == 0)
                {
                    _dhcpLeaseTime = DEFAULT_LEASE;
                }
                // Calculate T1 & T2 if we didn't get it
                if (_dhcpT1 == 0)
                {
                    // T1 should be 50% of _dhcpLeaseTime
                    _dhcpT1 = _dhcpLeaseTime >> 1;
                }
                if (_dhcpT2 == 0)
                {
                    // T2 should be 87.5% (7/8ths) of _dhcpLeaseTime
                    _dhcpT2 = _dhcpLeaseTime - (_dhcpLeaseTime >> 3);
                }
                _renewInSec = _dhcpT1;
                _rebindInSec = _dhcpT2;
            }
            else if (messageType == DHCP_NAK)
            {
                _dhcp_state = STATE_DHCP_START;
            }
        }

        if (messageType == 255)
        {
            messageType = 0;
            _dhcp_state = STATE_DHCP_START;
        }

        if (result != 1 && ((millis() - startTime) > _timeout))
            break;
    }

    // We're done with the socket now
    dhcp->udp->stop(dhcp->udp);
    // dhcp->udp->destory(dhcp->udp);

    _dhcpTransactionId++;

    _lastCheckLeaseMillis = millis();
    return result;
}

static void presend_DHCP(void)
{
    return;
}

static void send_DHCP_MESSAGE(uint8_t messageType, uint16_t secondsElapsed)
{
    uint8_t buffer[64];
    memset(buffer, 0, sizeof(buffer));

    IPAddress dest_addr;
    dest_addr.dword = 0xFFFFFFFF; // Broadcast address

    if (dhcp->udp->beginPacket_ip(dhcp->udp, dest_addr, DHCP_SERVER_PORT) != true)
    {
        DEBUG("DHCP transmit error\r\n");
        // FIXME Need to return errors
        return;
    }

    buffer[0] = DHCP_BOOTREQUEST;  // op
    buffer[1] = DHCP_HTYPE10MB;    // htype
    buffer[2] = DHCP_HLENETHERNET; // hlen
    buffer[3] = DHCP_HOPS;         // hops

    // xid
    unsigned long xid = htonl(_dhcpTransactionId);
    memcpy(buffer + 4, &(xid), 4);

    // 8, 9 - seconds elapsed
    buffer[8] = ((secondsElapsed & 0xff00) >> 8);
    buffer[9] = (secondsElapsed & 0x00ff);

    // flags
    unsigned short flags = htons(DHCP_FLAGSBROADCAST);
    memcpy(buffer + 10, &(flags), 2);

    // ciaddr: already zeroed
    // yiaddr: already zeroed
    // siaddr: already zeroed
    // giaddr: already zeroed

    //put data in W5100 transmit buffer
    dhcp->udp->write(dhcp->udp, buffer, 28);

    memset(buffer, 0, 32); // clear local buffer

    memcpy(buffer, _dhcpMacAddr, 6); // chaddr

    //put data in W5100 transmit buffer
    dhcp->udp->write(dhcp->udp, buffer, 16);

    memset(buffer, 0, 32); // clear local buffer

    // leave zeroed out for sname && file
    // put in W5100 transmit buffer x 6 (192 bytes)
    for (int i = 0; i < 6; i++)
    {
        dhcp->udp->write(dhcp->udp, buffer, 32);
    }

    // OPT - Magic Cookie
    buffer[0] = (uint8_t)((MAGIC_COOKIE >> 24) & 0xFF);
    buffer[1] = (uint8_t)((MAGIC_COOKIE >> 16) & 0xFF);
    buffer[2] = (uint8_t)((MAGIC_COOKIE >> 8) & 0xFF);
    buffer[3] = (uint8_t)(MAGIC_COOKIE & 0xFF);

    // OPT - message type
    buffer[4] = dhcpMessageType;
    buffer[5] = 0x01;
    buffer[6] = messageType; //DHCP_REQUEST;

    // OPT - client identifier
    buffer[7] = dhcpClientIdentifier;
    buffer[8] = 0x07;
    buffer[9] = 0x01;
    memcpy(buffer + 10, _dhcpMacAddr, 6);

    // OPT - host name
    buffer[16] = hostName;
    size_t hostname_len = strlen(HOST_NAME);
    buffer[17] = hostname_len + 6; // length of hostname + last 3 bytes of mac address
    strcpy((char *)&(buffer[18]), HOST_NAME);

    printByte((char *)&(buffer[hostname_len + 18 + 0]), _dhcpMacAddr[3]);
    printByte((char *)&(buffer[hostname_len + 18 + 2]), _dhcpMacAddr[4]);
    printByte((char *)&(buffer[hostname_len + 18 + 4]), _dhcpMacAddr[5]);

    //put data in W5100 transmit buffer
    dhcp->udp->write(dhcp->udp, buffer, hostname_len + 24);

    if (messageType == DHCP_REQUEST)
    {
        buffer[0] = dhcpRequestedIPaddr;
        buffer[1] = 0x04;
        buffer[2] = _dhcpLocalIp[0];
        buffer[3] = _dhcpLocalIp[1];
        buffer[4] = _dhcpLocalIp[2];
        buffer[5] = _dhcpLocalIp[3];

        buffer[6] = dhcpServerIdentifier;
        buffer[7] = 0x04;
        buffer[8] = _dhcpDhcpServerIp[0];
        buffer[9] = _dhcpDhcpServerIp[1];
        buffer[10] = _dhcpDhcpServerIp[2];
        buffer[11] = _dhcpDhcpServerIp[3];

        //put data in W5100 transmit buffer
        dhcp->udp->write(dhcp->udp, buffer, 12);
    }

    buffer[0] = dhcpParamRequest;
    buffer[1] = 0x06;
    buffer[2] = subnetMask;
    buffer[3] = routersOnSubnet;
    buffer[4] = dns;
    buffer[5] = domainName;
    buffer[6] = dhcpT1value;
    buffer[7] = dhcpT2value;
    buffer[8] = endOption;

    //put data in W5100 transmit buffer
    dhcp->udp->write(dhcp->udp, buffer, 9);

    dhcp->udp->endPacket(dhcp->udp);
    return;
}

static uint8_t parseDHCPResponse(unsigned long responseTimeout, uint32_t *transactionId)
{
    uint8_t type = 0;
    uint8_t opt_len = 0;

    unsigned long startTime = millis();
    int ret = 0;

    while ((ret = dhcp->udp->parsePacket(dhcp->udp)) <= 0)
    {
        DEBUG("parseDHCPResponse: ret:%d\r\n", ret);
        if ((millis() - startTime) > responseTimeout)
        {
            return 255;
        }
        msleep(50);
    }

    // start reading in the packet
    RIP_MSG_FIXED fixedMsg;
    dhcp->udp->read(dhcp->udp, (uint8_t *)&fixedMsg, sizeof(RIP_MSG_FIXED));

    if (fixedMsg.op == DHCP_BOOTREPLY && dhcp->udp->remote_port(dhcp->udp) == DHCP_SERVER_PORT)
    {
        *transactionId = ntohl(fixedMsg.xid);
        if (memcmp(fixedMsg.chaddr, _dhcpMacAddr, 6) != 0 ||
            (*transactionId < _dhcpInitialTransactionId) ||
            (*transactionId > _dhcpTransactionId))
        {
            // Need to read the rest of the packet here regardless
            dhcp->udp->flush(dhcp->udp); // FIXME
            return 0;
        }

        memcpy(_dhcpLocalIp, fixedMsg.yiaddr, 4);

        // Skip to the option part
        dhcp->udp->read(dhcp->udp, (uint8_t *)NULL, 240 - (int)sizeof(RIP_MSG_FIXED));

        while (dhcp->udp->available(dhcp->udp) > 0)
        {
            switch (dhcp->udp->read_one(dhcp->udp))
            {
            case endOption:
                break;

            case padOption:
                break;

            case dhcpMessageType:
                opt_len = dhcp->udp->read_one(dhcp->udp);
                type = dhcp->udp->read_one(dhcp->udp);
                break;

            case subnetMask:
                opt_len = dhcp->udp->read_one(dhcp->udp);
                dhcp->udp->read(dhcp->udp, _dhcpSubnetMask, 4);
                break;

            case routersOnSubnet:
                opt_len = dhcp->udp->read_one(dhcp->udp);
                dhcp->udp->read(dhcp->udp, _dhcpGatewayIp, 4);
                dhcp->udp->read(dhcp->udp, (uint8_t *)NULL, opt_len - 4);
                break;

            case dns:
                opt_len = dhcp->udp->read_one(dhcp->udp);
                dhcp->udp->read(dhcp->udp, _dhcpDnsServerIp, 4);
                dhcp->udp->read(dhcp->udp, (uint8_t *)NULL, opt_len - 4);
                break;

            case dhcpServerIdentifier:
                opt_len = dhcp->udp->read_one(dhcp->udp);

                IPAddress rip;
                dhcp->udp->remote_ip(dhcp->udp, &rip);

                if (((_dhcpDhcpServerIp[0] | _dhcpDhcpServerIp[1] | _dhcpDhcpServerIp[2] | _dhcpDhcpServerIp[3]) == 0) ||
                    (_dhcpDhcpServerIp[0] == rip.bytes[0] ||
                     _dhcpDhcpServerIp[1] == rip.bytes[1] ||
                     _dhcpDhcpServerIp[2] == rip.bytes[2] ||
                     _dhcpDhcpServerIp[3] == rip.bytes[3]))
                {
                    dhcp->udp->read(dhcp->udp, _dhcpDhcpServerIp, sizeof(_dhcpDhcpServerIp));
                }
                else
                {
                    // Skip over the rest of this option
                    dhcp->udp->read(dhcp->udp, (uint8_t *)NULL, opt_len);
                }
                break;

            case dhcpT1value:
                opt_len = dhcp->udp->read_one(dhcp->udp);
                dhcp->udp->read(dhcp->udp, (uint8_t *)&_dhcpT1, sizeof(_dhcpT1));
                _dhcpT1 = ntohl(_dhcpT1);
                break;

            case dhcpT2value:
                opt_len = dhcp->udp->read_one(dhcp->udp);
                dhcp->udp->read(dhcp->udp, (uint8_t *)&_dhcpT2, sizeof(_dhcpT2));
                _dhcpT2 = ntohl(_dhcpT2);
                break;

            case dhcpIPaddrLeaseTime:
                opt_len = dhcp->udp->read_one(dhcp->udp);
                dhcp->udp->read(dhcp->udp, (uint8_t *)&_dhcpLeaseTime, sizeof(_dhcpLeaseTime));
                _dhcpLeaseTime = ntohl(_dhcpLeaseTime);
                _renewInSec = _dhcpLeaseTime;
                break;

            default:
                opt_len = dhcp->udp->read_one(dhcp->udp);
                // Skip over the rest of this option
                dhcp->udp->read(dhcp->udp, (uint8_t *)NULL, opt_len);
                break;
            }
        }
    }
    else
    {
        DEBUG("error at %d\r\n", __LINE__);
    }

    // Need to skip to end of the packet regardless here
    dhcp->udp->flush(dhcp->udp); // FIXME

    return type;
}

/*
    returns:
    0/DHCP_CHECK_NONE: nothing happened
    1/DHCP_CHECK_RENEW_FAIL: renew failed
    2/DHCP_CHECK_RENEW_OK: renew success
    3/DHCP_CHECK_REBIND_FAIL: rebind fail
    4/DHCP_CHECK_REBIND_OK: rebind success
*/
static int dhcp_checkLease(void)
{
    int rc = DHCP_CHECK_NONE;

    unsigned long now = millis();
    unsigned long elapsed = now - _lastCheckLeaseMillis;

    // if more then one sec passed, reduce the counters accordingly
    if (elapsed >= 1000)
    {
        // set the new timestamps
        _lastCheckLeaseMillis = now - (elapsed % 1000);
        elapsed = elapsed / 1000;

        // decrease the counters by elapsed seconds
        // we assume that the cycle time (elapsed) is fairly constant
        // if the remainder is less than cycle time * 2
        // do it early instead of late
        if (_renewInSec < elapsed * 2)
        {
            _renewInSec = 0;
        }
        else
        {
            _renewInSec -= elapsed;
        }
        if (_rebindInSec < elapsed * 2)
        {
            _rebindInSec = 0;
        }
        else
        {
            _rebindInSec -= elapsed;
        }
    }

    // if we have a lease but should renew, do it
    if (_renewInSec == 0 && _dhcp_state == STATE_DHCP_LEASED)
    {
        _dhcp_state = STATE_DHCP_REREQUEST;
        rc = 1 + request_DHCP_lease();
    }

    // if we have a lease or is renewing but should bind, do it
    if (_rebindInSec == 0 && (_dhcp_state == STATE_DHCP_LEASED ||
                              _dhcp_state == STATE_DHCP_START))
    {
        // this should basically restart completely
        _dhcp_state = STATE_DHCP_START;
        reset_DHCP_lease();
        rc = 3 + request_DHCP_lease();
    }
    return rc;
}

static IPAddress dhcp_getLocalIp(void)
{
    IPAddress ret;
    ret.bytes[0] = _dhcpLocalIp[0];
    ret.bytes[1] = _dhcpLocalIp[1];
    ret.bytes[2] = _dhcpLocalIp[2];
    ret.bytes[3] = _dhcpLocalIp[3];

    return ret;
}

static IPAddress dhcp_getSubnetMask(void)
{
    IPAddress ret;
    ret.bytes[0] = _dhcpSubnetMask[0];
    ret.bytes[1] = _dhcpSubnetMask[1];
    ret.bytes[2] = _dhcpSubnetMask[2];
    ret.bytes[3] = _dhcpSubnetMask[3];

    return ret;
}

static IPAddress dhcp_getGatewayIp(void)
{
    IPAddress ret;
    ret.bytes[0] = _dhcpGatewayIp[0];
    ret.bytes[1] = _dhcpGatewayIp[1];
    ret.bytes[2] = _dhcpGatewayIp[2];
    ret.bytes[3] = _dhcpGatewayIp[3];

    return ret;
}

static IPAddress dhcp_getDhcpServerIp(void)
{
    IPAddress ret;
    ret.bytes[0] = _dhcpDhcpServerIp[0];
    ret.bytes[1] = _dhcpDhcpServerIp[1];
    ret.bytes[2] = _dhcpDhcpServerIp[2];
    ret.bytes[3] = _dhcpDhcpServerIp[3];

    return ret;
}

static IPAddress dhcp_getDnsServerIp(void)
{
    IPAddress ret;
    ret.bytes[0] = _dhcpDnsServerIp[0];
    ret.bytes[1] = _dhcpDnsServerIp[1];
    ret.bytes[2] = _dhcpDnsServerIp[2];
    ret.bytes[3] = _dhcpDnsServerIp[3];

    return ret;
}

static void printByte(char *buf, uint8_t n)
{
    char *str = &buf[1];
    buf[0] = '0';
    do
    {
        unsigned long m = n;
        n /= 16;
        char c = m - 16 * n;
        *str-- = c < 10 ? c + '0' : c + 'A' - 10;
    } while (n);
}

int dhcp_init(dhcp_t *dhcp)
{
    dhcp->getLocalIp = dhcp_getLocalIp;
    dhcp->getSubnetMask = dhcp_getSubnetMask;
    dhcp->getGatewayIp = dhcp_getGatewayIp;
    dhcp->getDhcpServerIp = dhcp_getDhcpServerIp;
    dhcp->getDnsServerIp = dhcp_getDnsServerIp;

    dhcp->checkLease = dhcp_checkLease;
    dhcp->beginWithDHCP = dhcp_beginWithDHCP;

    dhcp->udp = eth_udp_new();
    if (dhcp->udp == NULL)
    {
        printf("error @ %s:%d\r\n", __func__, __LINE__);
        return -1;
    }

    return 0;
}
