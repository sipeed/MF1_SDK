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
#include "EthernetServer.h"

#include <stdlib.h>

#include "utility/socket.h"

static void begin(eth_tcp_server_t *server)
{
    uint8_t sockindex = W5100_socketBegin(SnMR_TCP, server->port);
    if (sockindex < MAX_SOCK_NUM)
    {
        if (W5100_socketListen(sockindex))
        {
            eth_server_port[sockindex] = server->port;
        }
        else
        {
            W5100_socketDisconnect(sockindex);
        }
    }
}

static eth_tcp_client_t *available(eth_tcp_server_t *server)
{
    bool listening = false;
    uint8_t sockindex = MAX_SOCK_NUM;
    uint8_t chip, maxindex = MAX_SOCK_NUM;

    chip = W5100->chip;
    if (!chip)
        return NULL;

#if MAX_SOCK_NUM > 4
    if (chip == 51)
        maxindex = 4; // W5100 chip never supports more than 4 sockets
#endif

    // printf("%s->%d\r\n", __func__, __LINE__);

    for (uint8_t i = 0; i < maxindex; i++)
    {
        if (eth_server_port[i] == server->port)
        {
            // printf("%s->%d\r\n", __func__, __LINE__);

            uint8_t stat = W5100_socketStatus(i);
            if (stat == SnSR_ESTABLISHED || stat == SnSR_CLOSE_WAIT)
            {
                if (W5100_socketRecvAvailable(i) > 0)
                {
                    sockindex = i;
                }
                else
                {
                    // remote host closed connection, our end still open
                    if (stat == SnSR_CLOSE_WAIT)
                    {
                        W5100_socketDisconnect(i);
                        // status becomes LAST_ACK for short time
                    }
                }
            }
            else if (stat == SnSR_LISTEN)
            {
                listening = true;
            }
            else if (stat == SnSR_CLOSED)
            {
                eth_server_port[i] = 0;
            }
        }
    }

    // printf("%s->%d\r\n", __func__, __LINE__);

    if (!listening)
    {
        begin(server);
        // printf("%s->%d\r\n", __func__, __LINE__);
    }

    // printf("%s->%d\r\n", __func__, __LINE__);
    // printf("sock:%d\r\n", sockindex);

    return (sockindex == MAX_SOCK_NUM) ? NULL : eth_tcp_client_new(sockindex, TCP_CLIENT_DEFAULT_TIMEOUT);
}

static eth_tcp_client_t *accept(eth_tcp_server_t *server)
{
    bool listening = false;
    uint8_t sockindex = MAX_SOCK_NUM;
    uint8_t chip, maxindex = MAX_SOCK_NUM;

    chip = W5100->chip;
    if (!chip)
        return NULL;
#if MAX_SOCK_NUM > 4
    if (chip == 51)
        maxindex = 4; // W5100 chip never supports more than 4 sockets
#endif
    // printf("%s->%d\r\n", __func__, __LINE__);

    for (uint8_t i = 0; i < maxindex; i++)
    {
        if (eth_server_port[i] == server->port)
        {
            // printf("%s->%d\r\n", __func__, __LINE__);

            uint8_t stat = W5100_socketStatus(i);
            if (sockindex == MAX_SOCK_NUM &&
                (stat == SnSR_ESTABLISHED || stat == SnSR_CLOSE_WAIT))
            {
                // Return the connected client even if no data received.
                // Some protocols like FTP expect the server to send the
                // first data.
                sockindex = i;
                eth_server_port[i] = 0; // only return the client once
            }
            else if (stat == SnSR_LISTEN)
            {
                listening = true;
            }
            else if (stat == SnSR_CLOSED)
            {
                eth_server_port[i] = 0;
            }
        }
    }
    if (!listening)
    {
        begin(server);
        // printf("%s->%d\r\n", __func__, __LINE__);
    }

    return (sockindex == MAX_SOCK_NUM) ? NULL : eth_tcp_client_new(sockindex, TCP_CLIENT_DEFAULT_TIMEOUT);
}

static size_t write(eth_tcp_server_t *server, const uint8_t *buffer, size_t size)
{
    uint8_t chip, maxindex = MAX_SOCK_NUM;

    chip = W5100->chip;
    if (!chip)
        return 0;
#if MAX_SOCK_NUM > 4
    if (chip == 51)
        maxindex = 4; // W5100 chip never supports more than 4 sockets
#endif
    available(server);
    for (uint8_t i = 0; i < maxindex; i++)
    {
        if (eth_server_port[i] == server->port)
        {
            if (W5100_socketStatus(i) == SnSR_ESTABLISHED)
            {
                W5100_socketSend(i, buffer, size);
            }
        }
    }
    return size;
}

static void destory(eth_tcp_server_t *server)
{
    if (server)
    {
        free(server);
    }
}

eth_tcp_server_t *eth_tcp_server_new(uint16_t port)
{
    eth_tcp_server_t *ret = malloc(sizeof(eth_tcp_server_t));

    if (ret)
    {
        ret->port = port;

        ret->accept = accept;
        ret->available = available;
        ret->begin = begin;
        ret->write = write;
        ret->destory = destory;

        return ret;
    }
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
#if 0
operator bool()
{
    uint8_t maxindex = MAX_SOCK_NUM;
#if MAX_SOCK_NUM > 4
    if (W5100.getChip() == 51)
        maxindex = 4; // W5100 chip never supports more than 4 sockets
#endif
    for (uint8_t i = 0; i < maxindex; i++)
    {
        if (eth_server_port[i] == _port)
        {
            if (W5100_socketStatus(i) == SnSR::LISTEN)
            {
                return true; // server is listening for incoming clients
            }
        }
    }
    return false;
}

void statusreport()
{
	Serial.printf("EthernetServer, port=%d\n", _port);
	for (uint8_t i=0; i < MAX_SOCK_NUM; i++) {
		uint16_t port = eth_server_port[i];
		uint8_t stat = W5100_socketStatus(i);
		const char *name;
		switch (stat) {
			case 0x00: name = "CLOSED"; break;
			case 0x13: name = "INIT"; break;
			case 0x14: name = "LISTEN"; break;
			case 0x15: name = "SYNSENT"; break;
			case 0x16: name = "SYNRECV"; break;
			case 0x17: name = "ESTABLISHED"; break;
			case 0x18: name = "FIN_WAIT"; break;
			case 0x1A: name = "CLOSING"; break;
			case 0x1B: name = "TIME_WAIT"; break;
			case 0x1C: name = "CLOSE_WAIT"; break;
			case 0x1D: name = "LAST_ACK"; break;
			case 0x22: name = "UDP"; break;
			case 0x32: name = "IPRAW"; break;
			case 0x42: name = "MACRAW"; break;
			case 0x5F: name = "PPPOE"; break;
			default: name = "???";
		}
		int avail = W5100_socketRecvAvailable(i);
		Serial.printf("  %d: port=%d, status=%s (0x%02X), avail=%d\n",
			i, port, name, stat, avail);
	}
}
#endif
