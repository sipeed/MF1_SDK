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
#include "EthernetClient.h"

#include <stdlib.h>

#include "Ethernet.h"
#include "socket.h"
#include "Dns.h"

///////////////////////////////////////////////////////////////////////////////
static int connect_domain(eth_tcp_client_t *client, const char *host, uint16_t port)
{
    // Look up the host first
    IPAddress dns, remote_addr;

    if (client->sockindex < MAX_SOCK_NUM)
    {
        if (W5100_socketStatus(client->sockindex) != SnSR_CLOSED)
        {
            W5100_socketDisconnect(client->sockindex); // TODO: should we call stop()?
        }
        client->sockindex = MAX_SOCK_NUM;
    }
    dns = eth_w5100_dnsServerIP();
    eth_dns_begin(&dns);
    if (!eth_dns_getHostByName(host, &remote_addr, 50000))
        return 0; // TODO: use _timeout
    return client->connect_ip(client, remote_addr, port);
}

static int connect_ip(eth_tcp_client_t *client, IPAddress ip, uint16_t port)
{
    if (client->sockindex < MAX_SOCK_NUM)
    {
        if (W5100_socketStatus(client->sockindex) != SnSR_CLOSED)
        {
            W5100_socketDisconnect(client->sockindex); // TODO: should we call stop()?
        }
        client->sockindex = MAX_SOCK_NUM;
    }

    if (ip.dword == 0UL || ip.dword == 0xFFFFFFFFUL)
    {
        return 0;
    }
    client->sockindex = W5100_socketBegin(SnMR_TCP, 0);
    if (client->sockindex >= MAX_SOCK_NUM)
    {
        return 0;
    }
    W5100_socketConnect(client->sockindex, ip.bytes, port);
    uint32_t start = millis();
    while (1)
    {
        uint8_t stat = W5100_socketStatus(client->sockindex);
        if (stat == SnSR_ESTABLISHED)
            return 1;
        if (stat == SnSR_CLOSE_WAIT)
            return 1;
        if (stat == SnSR_CLOSED)
            return 0;
        if (millis() - start > client->_timeout)
            break;
        msleep(1);
    }
    W5100_socketClose(client->sockindex);
    client->sockindex = MAX_SOCK_NUM;
    return 0;
}

static int availableForWrite(eth_tcp_client_t *client)
{
    if (client->sockindex >= MAX_SOCK_NUM)
        return 0;
    return W5100_socketSendAvailable(client->sockindex);
}

static size_t write(eth_tcp_client_t *client, const uint8_t *buf, size_t size)
{
    if (client->sockindex >= MAX_SOCK_NUM)
        return 0;
    if (W5100_socketSend(client->sockindex, buf, size))
        return size;

    printk("write err @ %d\r\n", __LINE__);
    // setWriteError();
    return 0;
}

static size_t write_fix(eth_tcp_client_t *client, const uint8_t *buf, size_t size, uint16_t max_wait_ms)
{
    if (client->sockindex >= MAX_SOCK_NUM)
        return 0;

    uint32_t start = millis();

    size_t sent = 0, per_send;

    while (sent < size)
    {
        per_send = availableForWrite(client);
        per_send = ((size - sent) > per_send) ? per_send : (size - sent);

        if (millis() - start > max_wait_ms)
        {
            return sent;
        }

        if (per_send <= 0)
        {
            continue;
        }

        if (W5100_socketSend(client->sockindex, buf + sent, per_send) == 0)
            return sent;

        sent += per_send;
        start = millis();
    }

    return sent;
}

static int available(eth_tcp_client_t *client)
{
    if (client->sockindex >= MAX_SOCK_NUM)
        return 0;
    return W5100_socketRecvAvailable(client->sockindex);
    // TODO: do the Wiznet chips automatically retransmit TCP ACK
    // packets if they are lost by the network?  Someday this should
    // be checked by a man-in-the-middle test which discards certain
    // packets.  If ACKs aren't resent, we would need to check for
    // returning 0 here and after a timeout do another Sock_RECV
    // command to cause the Wiznet chip to resend the ACK packet.
}

static int eth_tcp_client_read(eth_tcp_client_t *client, uint8_t *buf, size_t size)
{
    if (client->sockindex >= MAX_SOCK_NUM)
        return 0;
    return W5100_socketRecv(client->sockindex, buf, size);
}

static int peek(eth_tcp_client_t *client)
{
    if (client->sockindex >= MAX_SOCK_NUM)
        return -1;
    if (!client->available(client))
        return -1;
    return W5100_socketPeek(client->sockindex);
}

static int eth_tcp_client_read_one(eth_tcp_client_t *client)
{
    uint8_t b;
    if (W5100_socketRecv(client->sockindex, &b, 1) > 0)
        return b;
    return -1;
}

static void flush(eth_tcp_client_t *client)
{
    while (client->sockindex < MAX_SOCK_NUM)
    {
        uint8_t stat = W5100_socketStatus(client->sockindex);
        if (stat != SnSR_ESTABLISHED && stat != SnSR_CLOSE_WAIT)
            return;
        if (W5100_socketSendAvailable(client->sockindex) >= W5100->SSIZE)
            return;
    }
}

static void stop(eth_tcp_client_t *client)
{
    if (client->sockindex >= MAX_SOCK_NUM)
        return;

    // attempt to close the connection gracefully (send a FIN to other side)
    W5100_socketDisconnect(client->sockindex);
    unsigned long start = millis();

    // wait up to a second for the connection to close
    do
    {
        if (W5100_socketStatus(client->sockindex) == SnSR_CLOSED)
        {
            client->sockindex = MAX_SOCK_NUM;
            return; // exit the loop
        }
        msleep(1);
    } while (millis() - start < client->_timeout);

    // if it hasn't closed, close it forcefully
    W5100_socketClose(client->sockindex);
    client->sockindex = MAX_SOCK_NUM;
}

static uint8_t connected(eth_tcp_client_t *client)
{
    if (client->sockindex >= MAX_SOCK_NUM)
        return 0;

    uint8_t s = W5100_socketStatus(client->sockindex);
    return !(s == SnSR_LISTEN || s == SnSR_CLOSED || s == SnSR_FIN_WAIT ||
             (s == SnSR_CLOSE_WAIT && !client->available(client)));
}

static uint8_t status(eth_tcp_client_t *client)
{
    if (client->sockindex >= MAX_SOCK_NUM)
        return SnSR_CLOSED;
    return W5100_socketStatus(client->sockindex);
}

//FIXME: what this for?
// // the next function allows us to use the client returned by
// // EthernetServer::available() as the condition in an if-statement.
// bool eth_tcp_client_operator == (const EthernetClient &rhs)
// {
//     if (sockindex != rhs.sockindex)
//         return false;
//     if (sockindex >= MAX_SOCK_NUM)
//         return false;
//     if (rhs.sockindex >= MAX_SOCK_NUM)
//         return false;
//     return true;
// }

// https://github.com/per1234/EthernetMod
// from: https://github.com/ntruchsess/Arduino-1/commit/937bce1a0bb2567f6d03b15df79525569377dabd
static uint16_t localPort(eth_tcp_client_t *client)
{
    if (client->sockindex >= MAX_SOCK_NUM)
        return 0;
    uint16_t port;
    port = W5100_readSnPORT(client->sockindex);
    return port;
}

// https://github.com/per1234/EthernetMod
// returns the remote IP address: http://forum.arduino.cc/index.php?topic=82416.0
static IPAddress remoteIP(eth_tcp_client_t *client)
{
    IPAddress ret;

    if (client->sockindex >= MAX_SOCK_NUM)
    {
        ret.dword = 0;
        return ret;
    }

    uint8_t remoteIParray[4];
    W5100_readSnDIPR(client->sockindex, remoteIParray);

    ret.bytes[0] = remoteIParray[0];
    ret.bytes[1] = remoteIParray[1];
    ret.bytes[2] = remoteIParray[2];
    ret.bytes[3] = remoteIParray[3];

    return ret;
}

// https://github.com/per1234/EthernetMod
// from: https://github.com/ntruchsess/Arduino-1/commit/ca37de4ba4ecbdb941f14ac1fe7dd40f3008af75
static uint16_t remotePort(eth_tcp_client_t *client)
{
    if (client->sockindex >= MAX_SOCK_NUM)
        return 0;
    uint16_t port;
    port = W5100_readSnDPORT(client->sockindex);
    return port;
}

static void destory(eth_tcp_client_t *client)
{
    free(client);
    return;
}

eth_tcp_client_t *eth_tcp_client_new(uint8_t sock, uint16_t timeout)
{
    eth_tcp_client_t *ret = malloc(sizeof(eth_tcp_client_t));
    if (ret)
    {
        memset(ret, 0, sizeof(eth_tcp_client_t));

        ret->sockindex = sock;
        ret->_timeout = timeout;

        ret->connect_domain = connect_domain;
        ret->connect_ip = connect_ip;
        ret->availableForWrite = availableForWrite;
        ret->write = write;
        ret->write_fix = write_fix;
        ret->available = available;
        ret->read_one = eth_tcp_client_read_one;
        ret->read = eth_tcp_client_read;
        ret->peek = peek;
        ret->flush = flush;
        ret->stop = stop;
        ret->connected = connected;
        ret->status = status;
        ret->localPort = localPort;
        ret->remoteIP = remoteIP;
        ret->remotePort = remotePort;
        ret->destory = destory;
        return ret;
    }
    return NULL;
}
