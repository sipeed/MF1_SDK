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
#include "socket.h"

///////////////////////////////////////////////////////////////////////////////
#define yield()

#define TAG "SOCKET"

//这个要写到其他文件。
uint16_t eth_server_port[MAX_SOCK_NUM];

///////////////////////////////////////////////////////////////////////////////

// TODO: randomize this when not using DHCP, but how?
static uint16_t local_port = 49152; // 49152 to 65535

typedef struct
{
    uint16_t RX_RSR; // Number of bytes received
    uint16_t RX_RD;  // Address to read
    uint16_t TX_FSR; // Free space ready for transmit
    uint8_t RX_inc;  // how much have we advanced RX_RD
} socketstate_t;

static socketstate_t state[MAX_SOCK_NUM];

static uint16_t getSnTX_FSR(uint8_t s);
static uint16_t getSnRX_RSR(uint8_t s);
static void write_data(uint8_t s, uint16_t offset, const uint8_t *data, uint16_t len);
static void read_data(uint8_t s, uint16_t src, uint8_t *dst, uint16_t len);

/*****************************************/
/*          Socket management            */
/*****************************************/

void W5100_socketPortRand(uint16_t n)
{
    n &= 0x3FFF;
    local_port ^= n;
    DEBUG("socketPortRand %d, srcport = %d\r\n", n, local_port);
    return;
}

uint8_t W5100_socketBegin(uint8_t protocol, uint16_t port)
{
    uint8_t s, status[MAX_SOCK_NUM], chip, maxindex = MAX_SOCK_NUM;

    // first check hardware compatibility
    chip = W5100->chip;
    if (!chip)
        return MAX_SOCK_NUM; // immediate error if no hardware detected
#if MAX_SOCK_NUM > 4
    if (chip == 51)
        maxindex = 4; // W5100 chip never supports more than 4 sockets
#endif
    DEBUG("W5000socket begin, protocol=%d, port=%d\r\n", protocol, port);
    // look at all the hardware sockets, use any that are closed (unused)
    for (s = 0; s < maxindex; s++)
    {
        status[s] = W5100_readSnSR(s);
        if (status[s] == SnSR_CLOSED)
            goto makesocket;
    }
    DEBUG("W5000socket step2\r\n");
    // as a last resort, forcibly close any already closing
    for (s = 0; s < maxindex; s++)
    {
        uint8_t stat = status[s];
        if (stat == SnSR_LAST_ACK)
            goto closemakesocket;
        if (stat == SnSR_TIME_WAIT)
            goto closemakesocket;
        if (stat == SnSR_FIN_WAIT)
            goto closemakesocket;
        if (stat == SnSR_CLOSING)
            goto closemakesocket;
    }
#if 0
	DEBUG("W5000socket step3\r\n");
	// next, use any that are effectively closed
	for (s=0; s < MAX_SOCK_NUM; s++) {
		uint8_t stat = status[s];
		// TODO: this also needs to check if no more data
		if (stat == SnSR_CLOSE_WAIT) goto closemakesocket;
	}
#endif
    return MAX_SOCK_NUM; // all sockets are in use
///////////////////////////////////////////////////////////////////////////////
closemakesocket:
    DEBUG("W5000socket close\r\n");
    W5100->execCmdSn(s, Sock_CLOSE);
makesocket:
    DEBUG("W5000socket %d\r\n", s);
    eth_server_port[s] = 0;
    usleep(250); // TODO: is this needed??
    W5100_writeSnMR(s, protocol);
    W5100_writeSnIR(s, 0xFF);
    if (port > 0)
    {
        W5100_writeSnPORT(s, port);
    }
    else
    {
        // if don't set the source port, set local_port number.
        if (++local_port < 49152)
            local_port = 49152;
        W5100_writeSnPORT(s, local_port);
    }
    W5100->execCmdSn(s, Sock_OPEN);
    state[s].RX_RSR = 0;
    state[s].RX_RD = W5100_readSnRX_RD(s); // always zero?
    state[s].RX_inc = 0;
    state[s].TX_FSR = 0;
    DEBUG("W5000socket prot=%d, RX_RD=%d\r\n", W5100_readSnMR(s), state[s].RX_RD);
    return s;
}

// multicast version to set fields before open  thd
uint8_t W5100_socketBeginMulticast(uint8_t protocol, IPAddress ip, uint16_t port)
{
    uint8_t s, status[MAX_SOCK_NUM], chip, maxindex = MAX_SOCK_NUM;

    // first check hardware compatibility
    chip = W5100->chip;
    if (!chip)
        return MAX_SOCK_NUM; // immediate error if no hardware detected
#if MAX_SOCK_NUM > 4
    if (chip == 51)
        maxindex = 4; // W5100 chip never supports more than 4 sockets
#endif
    DEBUG("W5000socket begin, protocol=%d, port=%d\r\n", protocol, port);

    // look at all the hardware sockets, use any that are closed (unused)
    for (s = 0; s < maxindex; s++)
    {
        status[s] = W5100_readSnSR(s);
        if (status[s] == SnSR_CLOSED)
            goto makesocket;
    }
    DEBUG("W5000socket step2\r\n");
    // as a last resort, forcibly close any already closing
    for (s = 0; s < maxindex; s++)
    {
        uint8_t stat = status[s];
        if (stat == SnSR_LAST_ACK)
            goto closemakesocket;
        if (stat == SnSR_TIME_WAIT)
            goto closemakesocket;
        if (stat == SnSR_FIN_WAIT)
            goto closemakesocket;
        if (stat == SnSR_CLOSING)
            goto closemakesocket;
    }
#if 1
    DEBUG("W5000socket step3\r\n");
    // next, use any that are effectively closed
    for (s = 0; s < MAX_SOCK_NUM; s++)
    {
        uint8_t stat = status[s];
        // TODO: this also needs to check if no more data
        if (stat == SnSR_CLOSE_WAIT)
            goto closemakesocket;
    }
#endif
    return MAX_SOCK_NUM; // all sockets are in use
closemakesocket:
    //DEBUG("W5000socket close\n");
    W5100->execCmdSn(s, Sock_CLOSE);
makesocket:
    DEBUG("W5000socket %d\r\n", s);
    eth_server_port[s] = 0;
    usleep(250); // TODO: is this needed??
    W5100_writeSnMR(s, protocol);
    W5100_writeSnIR(s, 0xFF);
    if (port > 0)
    {
        W5100_writeSnPORT(s, port);
    }
    else
    {
        // if don't set the source port, set local_port number.
        if (++local_port < 49152)
            local_port = 49152;
        W5100_writeSnPORT(s, local_port);
    }
    // Calculate MAC address from Multicast IP Address
    uint8_t mac[] = {0x01, 0x00, 0x5E, 0x00, 0x00, 0x00};
    mac[3] = ip.bytes[1] & 0x7F;
    mac[4] = ip.bytes[2];
    mac[5] = ip.bytes[3];
    W5100_writeSnDIPR(s, ip.bytes); //239.255.0.1
    W5100_writeSnDPORT(s, port);
    W5100_writeSnDHAR(s, mac);
    W5100->execCmdSn(s, Sock_OPEN);
    state[s].RX_RSR = 0;
    state[s].RX_RD = W5100_readSnRX_RD(s); // always zero?
    state[s].RX_inc = 0;
    state[s].TX_FSR = 0;
    DEBUG("W5000socket prot=%d, RX_RD=%d\n", W5100_readSnMR(s), state[s].RX_RD);
    return s;
}
// Return the socket's status
//
uint8_t W5100_socketStatus(uint8_t s)
{
    uint8_t status = W5100_readSnSR(s);
    return status;
}

// Immediately close.  If a TCP connection is established, the
// remote host is left unaware we closed.
//
void W5100_socketClose(uint8_t s)
{
    W5100->execCmdSn(s, Sock_CLOSE);
    return;
}

// Place the socket in listening (server) mode
//
uint8_t W5100_socketListen(uint8_t s)
{
    if (W5100_readSnSR(s) != SnSR_INIT)
    {
        return 0;
    }
    W5100->execCmdSn(s, Sock_LISTEN);
    return 1;
}

// establish a TCP connection in Active (client) mode.
//
void W5100_socketConnect(uint8_t s, uint8_t *addr, uint16_t port)
{
    // set destination IP
    W5100_writeSnDIPR(s, addr);
    W5100_writeSnDPORT(s, port);
    W5100->execCmdSn(s, Sock_CONNECT);
    return;
}

// Gracefully disconnect a TCP connection.
//
void W5100_socketDisconnect(uint8_t s)
{
    W5100->execCmdSn(s, Sock_DISCON);
    return;
}

/*****************************************/
/*    Socket Data Receive Functions      */
/*****************************************/

static uint16_t getSnRX_RSR(uint8_t s)
{
#if 1
    uint16_t val, prev;

    prev = W5100_readSnRX_RSR(s);
    while (1)
    {
        val = W5100_readSnRX_RSR(s);
        if (val == prev)
        {
            return val;
        }
        prev = val;
    }
#else
    uint16_t val = W5100_readSnRX_RSR(s);
    return val;
#endif
    return 0;
}

static void read_data(uint8_t s, uint16_t src, uint8_t *dst, uint16_t len)
{
    uint16_t size;
    uint16_t src_mask;
    uint16_t src_ptr;

    DEBUG("read_data, len=%d, at:%d\r\n", len, src);
    src_mask = (uint16_t)src & W5100->SMASK;
    src_ptr = W5100_RBASE(s) + src_mask;

    if (W5100_hasOffsetAddressMapping() || (src_mask + len) <= W5100->SSIZE)
    {
        DEBUG("[%s]->%d\r\n", __func__, __LINE__);

        W5100->read(src_ptr, dst, len);
    }
    else
    {
        DEBUG("[%s]->%d\r\n", __func__, __LINE__);

        size = W5100->SSIZE - src_mask;
        W5100->read(src_ptr, dst, size);
        dst += size;
        W5100->read(W5100_RBASE(s), dst, len - size);
    }
    return;
}

// Receive data.  Returns size, or -1 for no data, or 0 if connection closed
//
int W5100_socketRecv(uint8_t s, uint8_t *buf, int16_t len)
{
    // Check how much data is available
    int ret = state[s].RX_RSR;
    if (ret < len)
    {
        uint16_t rsr = getSnRX_RSR(s);
        ret = rsr - state[s].RX_inc;
        state[s].RX_RSR = ret;
        DEBUG("Sock_RECV, RX_RSR=%d, RX_inc=%d\r\n", ret, state[s].RX_inc);
    }
    if (ret == 0)
    {
        // No data available.
        uint8_t status = W5100_readSnSR(s);
        if (status == SnSR_LISTEN || status == SnSR_CLOSED ||
            status == SnSR_CLOSE_WAIT)
        {
            // The remote end has closed its side of the connection,
            // so this is the eof state
            DEBUG("conn closed\r\n");
            ret = 0;
        }
        else
        {
            // The connection is still up, but there's no data waiting to be read
            DEBUG("conn waiting\r\n");
            ret = -1;
        }
    }
    else
    {
        if (ret > len)
            ret = len; // more data available than buffer length
        uint16_t ptr = state[s].RX_RD;
        if (buf)
        {
            read_data(s, ptr, buf, ret);
        }

        ptr += ret;
        state[s].RX_RD = ptr;
        state[s].RX_RSR -= ret;
        uint16_t inc = state[s].RX_inc + ret;
        if (inc >= 250 || state[s].RX_RSR == 0)
        {
            state[s].RX_inc = 0;
            W5100_writeSnRX_RD(s, ptr);
            W5100->execCmdSn(s, Sock_RECV);
            DEBUG("Sock_RECV cmd, RX_RD=%d, RX_RSR=%d\r\n", state[s].RX_RD, state[s].RX_RSR);
        }
        else
        {
            state[s].RX_inc = inc;
        }
    }
    DEBUG("socketRecv, ret=%d\r\n", ret);
    return ret;
}

uint16_t W5100_socketRecvAvailable(uint8_t s)
{
    uint16_t ret = state[s].RX_RSR;
    if (ret == 0)
    {
        uint16_t rsr = getSnRX_RSR(s);
        ret = rsr - state[s].RX_inc;
        state[s].RX_RSR = ret;
        DEBUG("sockRecvAvailable s=%d, RX_RSR=%d\r\n", s, ret);
    }
    return ret;
}

// get the first byte in the receive queue (no checking)
//
uint8_t W5100_socketPeek(uint8_t s)
{
    uint8_t b;
    uint16_t ptr = state[s].RX_RD;
    W5100->read((ptr & W5100->SMASK) + W5100_RBASE(s), &b, 1);
    return b;
}

/*****************************************/
/*    Socket Data Transmit Functions     */
/*****************************************/

static uint16_t getSnTX_FSR(uint8_t s)
{
    uint16_t val, prev;

    prev = W5100_readSnTX_FSR(s);
    while (1)
    {
        val = W5100_readSnTX_FSR(s);
        if (val == prev)
        {
            state[s].TX_FSR = val;
            return val;
        }
        prev = val;
    }
}

static void write_data(uint8_t s, uint16_t data_offset, const uint8_t *data, uint16_t len)
{
    uint16_t ptr = W5100_readSnTX_WR(s);
    ptr += data_offset;
    uint16_t offset = ptr & W5100->SMASK;
    uint16_t dstAddr = offset + W5100_SBASE(s);

    if (W5100_hasOffsetAddressMapping() || offset + len <= W5100->SSIZE)
    {
        W5100->write(dstAddr, data, len);
    }
    else
    {
        // Wrap around circular buffer
        uint16_t size = W5100->SSIZE - offset;
        W5100->write(dstAddr, data, size);
        W5100->write(W5100_SBASE(s), data + size, len - size);
    }
    ptr += len;
    W5100_writeSnTX_WR(s, ptr);
    return;
}

/**
 * @brief	This function used to send the data in TCP mode
 * @return	1 for success else 0.
 */
uint16_t W5100_socketSend(uint8_t s, const uint8_t *buf, uint16_t len)
{
    uint8_t status = 0;
    uint16_t ret = 0;
    uint16_t freesize = 0;

    if (len > W5100->SSIZE)
    {
        ret = W5100->SSIZE; // check size not to exceed MAX size.
    }
    else
    {
        ret = len;
    }

    // if freebuf is available, start.
    do
    {
        freesize = getSnTX_FSR(s);
        status = W5100_readSnSR(s);
        if ((status != SnSR_ESTABLISHED) && (status != SnSR_CLOSE_WAIT))
        {
            ret = 0;
            break;
        }
        yield();
    } while (freesize < ret);

    // copy data
    write_data(s, 0, (uint8_t *)buf, ret);
    W5100->execCmdSn(s, Sock_SEND);

    /* +2008.01 bj */
    while ((W5100_readSnIR(s) & SnIR_SEND_OK) != SnIR_SEND_OK)
    {
        /* m2008.01 [bj] : reduce code */
        if (W5100_readSnSR(s) == SnSR_CLOSED)
        {
            return 0;
        }
        yield();
    }
    /* +2008.01 bj */
    W5100_writeSnIR(s, SnIR_SEND_OK);
    return ret;
}

uint16_t W5100_socketSendAvailable(uint8_t s)
{
    uint8_t status = 0;
    uint16_t freesize = 0;
    freesize = getSnTX_FSR(s);
    status = W5100_readSnSR(s);
    if ((status == SnSR_ESTABLISHED) || (status == SnSR_CLOSE_WAIT))
    {
        return freesize;
    }
    return 0;
}

uint16_t W5100_socketBufferData(uint8_t s, uint16_t offset, const uint8_t *buf, uint16_t len)
{
    DEBUG("  bufferData, offset=%d, len=%d\r\n", offset, len);
    uint16_t ret = 0;
    uint16_t txfree = getSnTX_FSR(s);
    if (len > txfree)
    {
        ret = txfree; // check size not to exceed MAX size.
    }
    else
    {
        ret = len;
    }
    write_data(s, offset, buf, ret);
    return ret;
}

bool W5100_socketStartUDP(uint8_t s, uint8_t *addr, uint16_t port)
{
    if (((addr[0] == 0x00) && (addr[1] == 0x00) && (addr[2] == 0x00) && (addr[3] == 0x00)) ||
        ((port == 0x00)))
    {
        return false;
    }
    W5100_writeSnDIPR(s, addr);
    W5100_writeSnDPORT(s, port);
    return true;
}

bool W5100_socketSendUDP(uint8_t s)
{
    W5100->execCmdSn(s, Sock_SEND);

    /* +2008.01 bj */
    while ((W5100_readSnIR(s) & SnIR_SEND_OK) != SnIR_SEND_OK)
    {
        if (W5100_readSnIR(s) & SnIR_TIMEOUT)
        {
            /* +2008.01 [bj]: clear interrupt */
            W5100_writeSnIR(s, (SnIR_SEND_OK | SnIR_TIMEOUT));
            DEBUG("sendUDP timeout\r\n");
            return false;
        }
        yield();
    }

    /* +2008.01 bj */
    W5100_writeSnIR(s, SnIR_SEND_OK);

    DEBUG("sendUDP ok\r\n");
    /* Sent ok */
    return true;
}
