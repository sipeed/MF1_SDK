/*
 *  Udp.cpp: Library to send/receive UDP packets with the Arduino ethernet shield.
 *  This version only offers minimal wrapping of socket.cpp
 *  Drop Udp.h/.cpp into the Ethernet library directory at hardware/libraries/Ethernet/
 *
 * MIT License:
 * Copyright (c) 2008 Bjoern Hartmann
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * bjoern@cs.stanford.edu 12/30/2008
 */

#include "EthernetUdp.h"

#include <stdlib.h>

#include "Ethernet.h"
#include "socket.h"
#include "Dns.h"

#define TAG "UDP"

///////////////////////////////////////////////////////////////////////////////
/* Start EthernetUDP socket, listening at local port PORT */
static uint8_t begin(eth_udp_t *udp, uint16_t port)
{
    if (udp->sockindex < MAX_SOCK_NUM)
        W5100_socketClose(udp->sockindex);
    udp->sockindex = W5100_socketBegin(SnMR_UDP, port);
    if (udp->sockindex >= MAX_SOCK_NUM)
        return 0;
    udp->_port = port;
    udp->_remaining = 0;
    return 1;
}

/* return number of bytes available in the current packet,
   will return zero if parsePacket hasn't been called yet */
static int available(eth_udp_t *udp)
{
    return udp->_remaining;
}

/* Release any resources being used by this EthernetUDP instance */
static void stop(eth_udp_t *udp)
{
    if (udp->sockindex < MAX_SOCK_NUM)
    {
        W5100_socketClose(udp->sockindex);
        udp->sockindex = MAX_SOCK_NUM;
    }
}

static int beginPacket_domain(eth_udp_t *udp, const char *host, uint16_t port)
{
    // Look up the host first
    int ret = 0;
    IPAddress dns, remote_addr;

    dns = eth_w5100_dnsServerIP();
    ret = eth_dns_getHostByName(host, &remote_addr, 50000);
    if (ret != 1)
        return ret;
    return udp->beginPacket_ip(udp, remote_addr, port);
}

static int beginPacket_ip(eth_udp_t *udp, IPAddress ip, uint16_t port)
{
    udp->_offset = 0;
    DEBUG("UDP beginPacket\r\n");
    return W5100_socketStartUDP(udp->sockindex, ip.bytes, port);
}

static int endPacket(eth_udp_t *udp)
{
    return W5100_socketSendUDP(udp->sockindex);
}

static size_t write(eth_udp_t *udp, const uint8_t *buffer, size_t size)
{
    DEBUG("UDP write %ld\r\n", size);
    uint16_t bytes_written = W5100_socketBufferData(udp->sockindex, udp->_offset, buffer, size);
    udp->_offset += bytes_written;
    return bytes_written;
}

static int parsePacket(eth_udp_t *udp)
{
    // discard any remaining bytes in the last packet
    while (udp->_remaining)
    {
        // could this fail (loop endlessly) if _remaining > 0 and recv in read fails?
        // should only occur if recv fails after telling us the data is there, lets
        // hope the w5100 always behaves :)
        udp->read(udp, (uint8_t *)NULL, udp->_remaining);
        DEBUG("[%s]->%d\r\n", __func__, __LINE__);
    }

    DEBUG("[%s]->%d\r\n", __func__, __LINE__);

    uint16_t aret = 0;

    if ((aret = W5100_socketRecvAvailable(udp->sockindex)) > 0)
    {
        DEBUG("aret:%d\r\n", aret);
        DEBUG("[%s]->%d\r\n", __func__, __LINE__);

        //HACK - hand-parse the UDP packet using TCP recv method
        uint8_t tmpBuf[8];
        int ret = 0;
        //read 8 header bytes and get IP and port from it
        ret = W5100_socketRecv(udp->sockindex, tmpBuf, 8);

        if (ret > 0)
        {
            DEBUG("ret:%d\r\n", ret);
            DEBUG("[%s]->%d\r\n", __func__, __LINE__);

            udp->_remoteIP.bytes[0] = tmpBuf[0];
            udp->_remoteIP.bytes[1] = tmpBuf[1];
            udp->_remoteIP.bytes[2] = tmpBuf[2];
            udp->_remoteIP.bytes[3] = tmpBuf[3];
            udp->_remotePort = tmpBuf[4];
            udp->_remotePort = (udp->_remotePort << 8) + tmpBuf[5];
            udp->_remaining = tmpBuf[6];
            udp->_remaining = (udp->_remaining << 8) + tmpBuf[7];

            // When we get here, any remaining bytes are the data
            ret = udp->_remaining;
        }
        DEBUG("aret:%d\r\n", aret);
        DEBUG("ret:%d\r\n", ret);
        DEBUG("[%s]->%d\r\n", __func__, __LINE__);
        return ret;
    }
    DEBUG("aret:%d\r\n", aret);
    DEBUG("[%s]->%d\r\n", __func__, __LINE__);
    // There aren't any packets available
    return 0;
}

static int read_one(eth_udp_t *udp)
{
    uint8_t byte;

    if ((udp->_remaining > 0) && (W5100_socketRecv(udp->sockindex, &byte, 1) > 0))
    {
        // We read things without any problems
        udp->_remaining--;
        return byte;
    }

    // If we get here, there's no data available
    return -1;
}

static int read(eth_udp_t *udp, unsigned char *buffer, size_t len)
{
    if (udp->_remaining > 0)
    {
        int got;
        if (udp->_remaining <= len)
        {
            // data should fit in the buffer
            got = W5100_socketRecv(udp->sockindex, buffer, udp->_remaining);
        }
        else
        {
            // too much data for the buffer,
            // grab as much as will fit
            got = W5100_socketRecv(udp->sockindex, buffer, len);
        }
        if (got > 0)
        {
            udp->_remaining -= got;
            //DEBUG("UDP read %d\r\n", got);
            return got;
        }
    }
    // If we get here, there's no data available or recv failed
    return -1;
}

static int peek(eth_udp_t *udp)
{
    // Unlike recv, peek doesn't check to see if there's any data available, so we must.
    // If the user hasn't called parsePacket yet then return nothing otherwise they
    // may get the UDP header
    if (udp->sockindex >= MAX_SOCK_NUM || udp->_remaining == 0)
        return -1;
    return W5100_socketPeek(udp->sockindex);
}

static void flush(eth_udp_t *udp)
{
    // TODO: we should wait for TX buffer to be emptied
}

/* Start EthernetUDP socket, listening at local port PORT */
static uint8_t beginMulticast(eth_udp_t *udp, IPAddress ip, uint16_t port)
{
    if (udp->sockindex < MAX_SOCK_NUM)
        W5100_socketClose(udp->sockindex);
    udp->sockindex = W5100_socketBeginMulticast(SnMR_UDP | SnMR_MULTI, ip, port);
    if (udp->sockindex >= MAX_SOCK_NUM)
        return 0;
    udp->_port = port;
    udp->_remaining = 0;
    return 1;
}

static uint16_t remote_port(eth_udp_t *udp)
{
    return udp->_remotePort;
}

static void remote_ip(eth_udp_t *udp, IPAddress *ip)
{
    ip->dword = udp->_remoteIP.dword;
}

static void destory(eth_udp_t *udp)
{
    free(udp);
    udp = NULL;
}

eth_udp_t *eth_udp_new(void)
{
    eth_udp_t *ret = malloc(sizeof(eth_udp_t));

    if (ret)
    {
        memset(ret, 0, sizeof(eth_udp_t));

        ret->sockindex = MAX_SOCK_NUM;

        ret->begin = begin;
        ret->available = available;
        ret->stop = stop;
        ret->beginPacket_domain = beginPacket_domain;
        ret->beginPacket_ip = beginPacket_ip;
        ret->endPacket = endPacket;
        ret->write = write;
        ret->parsePacket = parsePacket;
        ret->read_one = read_one;
        ret->read = read;
        ret->peek = peek;
        ret->flush = flush;
        ret->beginMulticast = beginMulticast;
        ret->remote_port = remote_port;
        ret->remote_ip = remote_ip;
        ret->destory = destory;
        return ret;
    }
    return NULL;
}
