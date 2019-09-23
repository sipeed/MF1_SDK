/*
 * Copyright 2018 Paul Stoffregen
 * Copyright (c) 2010 by Cristian Maglie <c.maglie@bug.st>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#ifndef W5100_H
#define W5100_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "sysctl.h"
#include "sleep.h"
#include "printf.h"

#pragma GCC diagnostic ignored "-Wunused-function"

// By default, each socket uses 2K buffers inside the Wiznet chip.  If
// MAX_SOCK_NUM is set to fewer than the chip's maximum, uncommenting
// this will use larger buffers within the Wiznet chip.  Large buffers
// can really help with UDP protocols like Artnet.  In theory larger
// buffers should allow faster TCP over high-latency links, but this
// does not always seem to work in practice (maybe Wiznet bugs?)
#define SPI_HAS_TRANSFER_BUF 1
#define ETHERNET_LARGE_BUFFERS 1
#define MAX_SOCK_NUM 4

#if 0
#define DEBUG(...)                                      \
  do                                                    \
  {                                                     \
    printk("%s\t[%s]:%d\r\n", TAG, __func__, __LINE__); \
    printf(__VA_ARGS__);                                \
  } while (0)
#else
#define DEBUG(...)
#endif

///////////////////////////////////////////////////////////////////////////////

typedef struct
{
  union {
    uint8_t bytes[4];
    uint32_t dword;
  };
} IPAddress;

typedef uint8_t SOCKET;

enum SockCMD
{
  Sock_OPEN = 0x01,
  Sock_LISTEN = 0x02,
  Sock_CONNECT = 0x04,
  Sock_DISCON = 0x08,
  Sock_CLOSE = 0x10,
  Sock_SEND = 0x20,
  Sock_SEND_MAC = 0x21,
  Sock_SEND_KEEP = 0x22,
  Sock_RECV = 0x40
};

enum W5100Linkstatus
{
  UNKNOWN = 0,
  LINK_ON = 1,
  LINK_OFF = 2
};

enum SnMR
{
  SnMR_CLOSE = 0x00,
  SnMR_TCP = 0x21,
  SnMR_UDP = 0x02,
  SnMR_IPRAW = 0x03,
  SnMR_MACRAW = 0x04,
  SnMR_PPPOE = 0x05,
  SnMR_ND = 0x20,
  SnMR_MULTI = 0x80,
};

enum SnIR
{
  SnIR_SEND_OK = 0x10,
  SnIR_TIMEOUT = 0x08,
  SnIR_RECV = 0x04,
  SnIR_DISCON = 0x02,
  SnIR_CON = 0x01,
};

enum SnSR
{
  SnSR_CLOSED = 0x00,
  SnSR_INIT = 0x13,
  SnSR_LISTEN = 0x14,
  SnSR_SYNSENT = 0x15,
  SnSR_SYNRECV = 0x16,
  SnSR_ESTABLISHED = 0x17,
  SnSR_FIN_WAIT = 0x18,
  SnSR_CLOSING = 0x1A,
  SnSR_TIME_WAIT = 0x1B,
  SnSR_CLOSE_WAIT = 0x1C,
  SnSR_LAST_ACK = 0x1D,
  SnSR_UDP = 0x22,
  SnSR_IPRAW = 0x32,
  SnSR_MACRAW = 0x42,
  SnSR_PPPOE = 0x5F,
};

enum IPPROTO
{
  IPPROTO_IP = 0,
  IPPROTO_ICMP = 1,
  IPPROTO_IGMP = 2,
  IPPROTO_GGP = 3,
  IPPROTO_TCP = 6,
  IPPROTO_PUP = 12,
  IPPROTO_UDP = 17,
  IPPROTO_IDP = 22,
  IPPROTO_ND = 77,
  IPPROTO_RAW = 255,
};

///////////////////////////////////////////////////////////////////////////////
uint8_t W5500_reg_cs_func(void (*cs_sel)(void), void (*cs_desel)(void));
uint8_t W500_reg_spi_func(void (*spi_write)(uint8_t *send, size_t len),
                          void (*spi_read)(uint8_t *recv, size_t len));

typedef struct _W5100 W5100_t;

typedef struct _W5100
{
  uint8_t chip;
  uint8_t CH_BASE_MSB;
  uint16_t CH_SIZE;
  uint16_t SSIZE;
  uint16_t SMASK;

  uint8_t (*init)(void);
  uint8_t (*soft_reset)(void);

  uint8_t (*isW5100)(void);
  uint8_t (*isW5200)(void);
  uint8_t (*isW5500)(void);

  enum W5100Linkstatus (*getLinkStatus)(void);

  uint16_t (*write)(uint16_t addr, /* const */ uint8_t *buf, uint16_t len);
  uint16_t (*read)(uint16_t addr, uint8_t *buf, uint16_t len);

  void (*execCmdSn)(SOCKET s, enum SockCMD _cmd);
} W5100_t;

///////////////////////////////////////////////////////////////////////////////

extern W5100_t *W5100;

uint8_t W5100_init(W5100_t *w5100);

unsigned long millis(void);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// W5100 Registers

#define __GP_REGISTER8(name, address)                 \
  static inline void W5100_write##name(uint8_t _data) \
  {                                                   \
    W5100->write(address, &_data, 1);                 \
  }                                                   \
  static inline uint8_t W5100_read##name()            \
  {                                                   \
    uint8_t data;                                     \
    W5100->read(address, &data, 1);                   \
    return data;                                      \
  }
///////////////////////////////////////////////////////////////////////////////
#define __GP_REGISTER16(name, address)                 \
  static inline void W5100_write##name(uint16_t _data) \
  {                                                    \
    uint8_t buf[2];                                    \
    buf[0] = _data >> 8;                               \
    buf[1] = _data & 0xFF;                             \
    W5100->write(address, buf, 2);                     \
  }                                                    \
  static inline uint16_t W5100_read##name()            \
  {                                                    \
    uint8_t buf[2];                                    \
    W5100->read(address, buf, 2);                      \
    return (buf[0] << 8) | buf[1];                     \
  }
///////////////////////////////////////////////////////////////////////////////
#define __GP_REGISTER_N(name, address, size)               \
  static inline uint16_t W5100_write##name(uint8_t *_buff) \
  {                                                        \
    return W5100->write(address, _buff, size);             \
  }                                                        \
  static inline uint16_t W5100_read##name(uint8_t *_buff)  \
  {                                                        \
    return W5100->read(address, _buff, size);              \
  }
///////////////////////////////////////////////////////////////////////////////

__GP_REGISTER8(MR, 0x0000);             // Mode
__GP_REGISTER_N(GAR, 0x0001, 4);        // Gateway IP address
__GP_REGISTER_N(SUBR, 0x0005, 4);       // Subnet mask address
__GP_REGISTER_N(SHAR, 0x0009, 6);       // Source MAC address
__GP_REGISTER_N(SIPR, 0x000F, 4);       // Source IP address
__GP_REGISTER8(IR, 0x0015);             // Interrupt
__GP_REGISTER8(IMR, 0x0016);            // Interrupt Mask
__GP_REGISTER16(RTR, 0x0017);           // Timeout address
__GP_REGISTER8(RCR, 0x0019);            // Retry count
__GP_REGISTER8(RMSR, 0x001A);           // Receive memory size (W5100 only)
__GP_REGISTER8(TMSR, 0x001B);           // Transmit memory size (W5100 only)
__GP_REGISTER8(PATR, 0x001C);           // Authentication type address in PPPoE mode
__GP_REGISTER8(PTIMER, 0x0028);         // PPP LCP Request Timer
__GP_REGISTER8(PMAGIC, 0x0029);         // PPP LCP Magic Number
__GP_REGISTER_N(UIPR, 0x002A, 4);       // Unreachable IP address in UDP mode (W5100 only)
__GP_REGISTER16(UPORT, 0x002E);         // Unreachable Port address in UDP mode (W5100 only)
__GP_REGISTER8(VERSIONR_W5200, 0x001F); // Chip Version Register (W5200 only)
__GP_REGISTER8(VERSIONR_W5500, 0x0039); // Chip Version Register (W5500 only)
__GP_REGISTER8(PSTATUS_W5200, 0x0035);  // PHY Status
__GP_REGISTER8(PHYCFGR_W5500, 0x002E);  // PHY Configuration register, default: 10111xxx

#undef __GP_REGISTER8
#undef __GP_REGISTER16
#undef __GP_REGISTER_N
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// W5100 Socket registers
static inline uint16_t readSn(SOCKET s, uint16_t addr, uint8_t *buf, uint16_t len)
{
  return W5100->read(((W5100->CH_BASE_MSB) << 8) + s * W5100->CH_SIZE + addr, buf, len);
}
static inline uint16_t writeSn(SOCKET s, uint16_t addr, uint8_t *buf, uint16_t len)
{
  return W5100->write(((W5100->CH_BASE_MSB) << 8) + s * W5100->CH_SIZE + addr, buf, len);
}
///////////////////////////////////////////////////////////////////////////////
#define __SOCKET_REGISTER8(name, address)                        \
  static inline void W5100_write##name(SOCKET _s, uint8_t _data) \
  {                                                              \
    writeSn(_s, address, &_data, 1);                             \
  }                                                              \
  static inline uint8_t W5100_read##name(SOCKET _s)              \
  {                                                              \
    uint8_t data;                                                \
    readSn(_s, address, &data, 1);                               \
    return data;                                                 \
  }
///////////////////////////////////////////////////////////////////////////////
#define __SOCKET_REGISTER16(name, address)                        \
  static inline void W5100_write##name(SOCKET _s, uint16_t _data) \
  {                                                               \
    uint8_t buf[2];                                               \
    buf[0] = _data >> 8;                                          \
    buf[1] = _data & 0xFF;                                        \
    writeSn(_s, address, buf, 2);                                 \
  }                                                               \
  static inline uint16_t W5100_read##name(SOCKET _s)              \
  {                                                               \
    uint8_t buf[2];                                               \
    readSn(_s, address, buf, 2);                                  \
    return (buf[0] << 8) | buf[1];                                \
  }
///////////////////////////////////////////////////////////////////////////////
#define __SOCKET_REGISTER_N(name, address, size)                      \
  static inline uint16_t W5100_write##name(SOCKET _s, uint8_t *_buff) \
  {                                                                   \
    return writeSn(_s, address, _buff, size);                         \
  }                                                                   \
  static inline uint16_t W5100_read##name(SOCKET _s, uint8_t *_buff)  \
  {                                                                   \
    return readSn(_s, address, _buff, size);                          \
  }
///////////////////////////////////////////////////////////////////////////////
__SOCKET_REGISTER8(SnMR, 0x0000)       // Mode
__SOCKET_REGISTER8(SnCR, 0x0001)       // Command
__SOCKET_REGISTER8(SnIR, 0x0002)       // Interrupt
__SOCKET_REGISTER8(SnSR, 0x0003)       // Status
__SOCKET_REGISTER16(SnPORT, 0x0004)    // Source Port
__SOCKET_REGISTER_N(SnDHAR, 0x0006, 6) // Destination Hardw Addr
__SOCKET_REGISTER_N(SnDIPR, 0x000C, 4) // Destination IP Addr
__SOCKET_REGISTER16(SnDPORT, 0x0010)   // Destination Port
__SOCKET_REGISTER16(SnMSSR, 0x0012)    // Max Segment Size
__SOCKET_REGISTER8(SnPROTO, 0x0014)    // Protocol in IP RAW Mode
__SOCKET_REGISTER8(SnTOS, 0x0015)      // IP TOS
__SOCKET_REGISTER8(SnTTL, 0x0016)      // IP TTL
__SOCKET_REGISTER8(SnRX_SIZE, 0x001E)  // RX Memory Size (W5200 only)
__SOCKET_REGISTER8(SnTX_SIZE, 0x001F)  // RX Memory Size (W5200 only)
__SOCKET_REGISTER16(SnTX_FSR, 0x0020)  // TX Free Size
__SOCKET_REGISTER16(SnTX_RD, 0x0022)   // TX Read Pointer
__SOCKET_REGISTER16(SnTX_WR, 0x0024)   // TX Write Pointer
__SOCKET_REGISTER16(SnRX_RSR, 0x0026)  // RX Free Size
__SOCKET_REGISTER16(SnRX_RD, 0x0028)   // RX Read Pointer
__SOCKET_REGISTER16(SnRX_WR, 0x002A)   // RX Write Pointer (supported?)

#undef __SOCKET_REGISTER8
#undef __SOCKET_REGISTER16
#undef __SOCKET_REGISTER_N
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static inline void W5100_setGatewayIp(uint8_t *addr) { W5100_writeGAR(addr); }
static inline void W5100_getGatewayIp(uint8_t *addr) { W5100_readGAR(addr); }

static inline void W5100_setSubnetMask(uint8_t *addr) { W5100_writeSUBR(addr); }
static inline void W5100_getSubnetMask(uint8_t *addr) { W5100_readSUBR(addr); }

static inline void W5100_setMACAddress(uint8_t *addr) { W5100_writeSHAR(addr); }
static inline void W5100_getMACAddress(uint8_t *addr) { W5100_readSHAR(addr); }

static inline void W5100_setIPAddress(uint8_t *addr) { W5100_writeSIPR(addr); }
static inline void W5100_getIPAddress(uint8_t *addr) { W5100_readSIPR(addr); }

static inline void W5100_setRetransmissionTime(uint16_t timeout) { W5100_writeRTR(timeout); }
static inline void W5100_setRetransmissionCount(uint8_t retry) { W5100_writeRCR(retry); }

static uint16_t W5100_SBASE(uint8_t socknum)
{
  if (W5100->chip == 51)
  {
    return socknum * W5100->SSIZE + 0x4000;
  }
  else
  {
    return socknum * W5100->SSIZE + 0x8000;
  }
}

static uint16_t W5100_RBASE(uint8_t socknum)
{
  if (W5100->chip == 51)
  {
    return socknum * W5100->SSIZE + 0x6000;
  }
  else
  {
    return socknum * W5100->SSIZE + 0xC000;
  }
}

static bool W5100_hasOffsetAddressMapping(void)
{
  if (W5100->chip == 55)
    return true;
  return false;
}

#endif
