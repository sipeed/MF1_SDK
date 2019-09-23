/*
 * Copyright 2018 Paul Stoffregen
 * Copyright (c) 2010 by Cristian Maglie <c.maglie@bug.st>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */
#include "w5100.h"

#define TAG "W5500"

///////////////////////////////////////////////////////////////////////////////
static W5100_t _W5100;
W5100_t *W5100 = &_W5100;
///////////////////////////////////////////////////////////////////////////////
static uint8_t init(void);
static uint8_t softReset(void);
static uint8_t isW5100(void);
static uint8_t isW5200(void);
static uint8_t isW5500(void);
static enum W5100Linkstatus getLinkStatus();
static uint16_t write(uint16_t addr, /* const */ uint8_t *buf, uint16_t len);
static uint16_t read(uint16_t addr, uint8_t *buf, uint16_t len);
static void execCmdSn(SOCKET s, enum SockCMD _cmd);
///////////////////////////////////////////////////////////////////////////////
typedef struct
{
	void (*_select)(void);
	void (*_deselect)(void);

	void (*spi_write)(uint8_t *send, size_t len);
	void (*spi_read)(uint8_t *recv, size_t len);
} w5500_chip_op_t;

static w5500_chip_op_t W5500_CHIP = {
	._select = NULL,
	._deselect = NULL,
	.spi_write = NULL,
	.spi_read = NULL,
};

///////////////////////////////////////////////////////////////////////////////
uint8_t W5500_reg_cs_func(void (*cs_sel)(void), void (*cs_desel)(void))
{
	if (!cs_sel || !cs_desel)
	{
		return 0;
	}

	DEBUG("[%s]->%d\r\n", __func__, __LINE__);

	W5500_CHIP._select = cs_sel;
	W5500_CHIP._deselect = cs_desel;

	return 1;
}

///////////////////////////////////////////////////////////////////////////////
uint8_t W500_reg_spi_func(void (*spi_write)(uint8_t *send, size_t len),
						  void (*spi_read)(uint8_t *recv, size_t len))
{
	if (!spi_write || !spi_read)
	{
		return 0;
	}

	DEBUG("[%s]->%d\r\n", __func__, __LINE__);

	W5500_CHIP.spi_write = spi_write;
	W5500_CHIP.spi_read = spi_read;

	return 1;
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static uint8_t init(void)
{
	static uint8_t initialized = false;
	uint8_t i;

	if (initialized)
		return 1;

	// Many Ethernet shields have a CAT811 or similar reset chip
	// connected to W5100 or W5200 chips.  The W5200 will not work at
	// all, and may even drive its MISO pin, until given an active low
	// reset pulse!  The CAT811 has a 240 ms typical pulse length, and
	// a 400 ms worst case maximum pulse length.  MAX811 has a worst
	// case maximum 560 ms pulse length.  This delay is meant to wait
	// until the reset pulse is ended.  If your hardware has a shorter
	// reset time, this can be edited or removed.
	msleep(560);
	DEBUG("w5100 init\r\n");

	// Attempt W5200 detection first, because W5200 does not properly
	// reset its SPI state when CS goes high (inactive).  Communication
	// from detecting the other chips can leave the W5200 in a state
	// where it won't recover, unless given a reset pulse.
	if (isW5200())
	{
		_W5100.CH_BASE_MSB = 0x40;
#ifdef ETHERNET_LARGE_BUFFERS
#if MAX_SOCK_NUM <= 1
		_W5100.SSIZE = 16384;
#elif MAX_SOCK_NUM <= 2
		_W5100.SSIZE = 8192;
#elif MAX_SOCK_NUM <= 4
		_W5100.SSIZE = 4096;
#else
		_W5100.SSIZE = 2048;
#endif
		_W5100.SMASK = _W5100.SSIZE - 1;
#endif
		for (i = 0; i < MAX_SOCK_NUM; i++)
		{
			W5100_writeSnRX_SIZE(i, _W5100.SSIZE >> 10);
			W5100_writeSnTX_SIZE(i, _W5100.SSIZE >> 10);
		}
		for (; i < 8; i++)
		{
			W5100_writeSnRX_SIZE(i, 0);
			W5100_writeSnTX_SIZE(i, 0);
		}
		// Try W5500 next.  Wiznet finally seems to have implemented
		// SPI well with this chip.  It appears to be very resilient,
		// so try it after the fragile W5200
	}
	else if (isW5500())
	{
		_W5100.CH_BASE_MSB = 0x10;
#ifdef ETHERNET_LARGE_BUFFERS
#if MAX_SOCK_NUM <= 1
		_W5100.SSIZE = 16384;
#elif MAX_SOCK_NUM <= 2
		_W5100.SSIZE = 8192;
#elif MAX_SOCK_NUM <= 4
		_W5100.SSIZE = 4096;
#else
		_W5100.SSIZE = 2048;
#endif
		_W5100.SMASK = _W5100.SSIZE - 1;
		for (i = 0; i < MAX_SOCK_NUM; i++)
		{
			W5100_writeSnRX_SIZE(i, _W5100.SSIZE >> 10);
			W5100_writeSnTX_SIZE(i, _W5100.SSIZE >> 10);
		}
		for (; i < 8; i++)
		{
			W5100_writeSnRX_SIZE(i, 0);
			W5100_writeSnTX_SIZE(i, 0);
		}
#endif
		// Try W5100 last.  This simple chip uses fixed 4 byte frames
		// for every 8 bit access.  Terribly inefficient, but so simple
		// it recovers from "hearing" unsuccessful W5100 or W5200
		// communication.  W5100 is also the only chip without a VERSIONR
		// register for identification, so we check this last.
	}
	else if (isW5100())
	{
		_W5100.CH_BASE_MSB = 0x04;
#ifdef ETHERNET_LARGE_BUFFERS
#if MAX_SOCK_NUM <= 1
		_W5100.SSIZE = 8192;
		W5100_writeTMSR(0x03);
		W5100_writeRMSR(0x03);
#elif MAX_SOCK_NUM <= 2
		_W5100.SSIZE = 4096;
		W5100_writeTMSR(0x0A);
		W5100_writeRMSR(0x0A);
#else
		_W5100.SSIZE = 2048;
		W5100_writeTMSR(0x55);
		W5100_writeRMSR(0x55);
#endif
		_W5100.SMASK = _W5100.SSIZE - 1;
#else
		W5100_writeTMSR(0x55);
		W5100_writeRMSR(0x55);
#endif
		// No hardware seems to be present.  Or it could be a W5200
		// that's heard other SPI communication if its chip select
		// pin wasn't high when a SD card or other SPI chip was used.
	}
	else
	{
		DEBUG("no chip :-(\r\n");
		_W5100.chip = 0;
		return 0; // no known chip is responding :-(
	}
	initialized = true;
	return 1; // successful init
}

// Soft reset the Wiznet chip, by writing to its MR register reset bit
static uint8_t softReset(void)
{
	uint16_t count = 0;

	DEBUG("Wiznet soft reset\r\n");
	// write to reset bit
	W5100_writeMR(0x80);
	// then wait for soft reset to complete
	do
	{
		uint8_t mr = W5100_readMR();
		DEBUG("mr:%02X\r\n", mr);
		if (mr == 0)
			return 1;
		msleep(1);
	} while (++count < 20);
	return 0;
}

static uint8_t isW5100(void)
{
	_W5100.chip = 51;
	DEBUG("w5100.c: detect W5100 chip\r\n");
	if (!softReset())
		return 0;
	W5100_writeMR(0x10);
	if (W5100_readMR() != 0x10)
		return 0;
	W5100_writeMR(0x12);
	if (W5100_readMR() != 0x12)
		return 0;
	W5100_writeMR(0x00);
	if (W5100_readMR() != 0x00)
		return 0;
	DEBUG("chip is W5100\r\n");
	return 1;
}

static uint8_t isW5200(void)
{
	_W5100.chip = 52;
	DEBUG("w5100.c: detect W5200 chip\r\n");
	if (!softReset())
		return 0;
	W5100_writeMR(0x08);
	if (W5100_readMR() != 0x08)
		return 0;
	W5100_writeMR(0x10);
	if (W5100_readMR() != 0x10)
		return 0;
	W5100_writeMR(0x00);
	if (W5100_readMR() != 0x00)
		return 0;
	int ver = W5100_readVERSIONR_W5200();
	DEBUG("version=%d\r\n\r\n", ver);
	if (ver != 3)
		return 0;
	DEBUG("chip is W5200\r\n");
	return 1;
}

static uint8_t isW5500(void)
{
	_W5100.chip = 55;
	DEBUG("w5100.c: detect W5500 chip\r\n");
	if (!softReset())
		return 0;
	W5100_writeMR(0x08);
	if (W5100_readMR() != 0x08)
		return 0;
	W5100_writeMR(0x10);
	if (W5100_readMR() != 0x10)
		return 0;
	W5100_writeMR(0x00);
	if (W5100_readMR() != 0x00)
		return 0;
	int ver = W5100_readVERSIONR_W5500();
	DEBUG("version=%d\r\n", ver);
	if (ver != 4)
		return 0;
	DEBUG("chip is W5500\r\n");
	return 1;
}

static enum W5100Linkstatus getLinkStatus()
{
	uint8_t phystatus;

	if (!init())
		return UNKNOWN;
	switch (_W5100.chip)
	{
	case 52:
		phystatus = W5100_readPSTATUS_W5200();
		if (phystatus & 0x20)
			return LINK_ON;
		return LINK_OFF;
	case 55:
		phystatus = W5100_readPHYCFGR_W5500();
		if (phystatus & 0x01)
			return LINK_ON;
		return LINK_OFF;
	default:
		return UNKNOWN;
	}
	return UNKNOWN;
}

static uint16_t write(uint16_t addr, /* const */ uint8_t *buf, uint16_t len)
{
	uint8_t cmd[8];

	DEBUG("write: addr:%X len:%d\r\n", addr, len);
	//XXX:not support now
	// 	if (_W5100.chip == 51)
	// 	{
	// 		for (uint16_t i = 0; i < len; i++)
	// 		{
	// 			W5500_CHIP._select();
	// 			SPI.transfer(0xF0);
	// 			SPI.transfer(addr >> 8);
	// 			SPI.transfer(addr & 0xFF);
	// 			addr++;
	// 			SPI.transfer(buf[i]);
	// 			W5500_CHIP._deselect();
	// 		}
	// 	}
	// 	else if (_W5100.chip == 52)
	// 	{
	// 		W5500_CHIP._select();
	// 		cmd[0] = addr >> 8;
	// 		cmd[1] = addr & 0xFF;
	// 		cmd[2] = ((len >> 8) & 0x7F) | 0x80;
	// 		cmd[3] = len & 0xFF;
	// 		SPI.transfer(cmd, 4);
	// #ifdef SPI_HAS_TRANSFER_BUF
	// 		SPI.transfer(buf, NULL, len);
	// #else
	// 		// TODO: copy 8 bytes at a time to cmd[] and block transfer
	// 		for (uint16_t i = 0; i < len; i++)
	// 		{
	// 			SPI.transfer(buf[i]);
	// 		}
	// #endif
	// 		W5500_CHIP._deselect();
	// 	}
	// 	else
	{ // chip == 55

		W5500_CHIP._select();

		if (addr < 0x100)
		{
			// common registers 00nn
			cmd[0] = 0;
			cmd[1] = addr & 0xFF;
			cmd[2] = 0x04;
		}
		else if (addr < 0x8000)
		{
			// socket registers  10nn, 11nn, 12nn, 13nn, etc
			cmd[0] = 0;
			cmd[1] = addr & 0xFF;
			cmd[2] = ((addr >> 3) & 0xE0) | 0x0C;
		}
		else if (addr < 0xC000)
		{
			// transmit buffers  8000-87FF, 8800-8FFF, 9000-97FF, etc
			//  10## #nnn nnnn nnnn
			cmd[0] = addr >> 8;
			cmd[1] = addr & 0xFF;
#if defined(ETHERNET_LARGE_BUFFERS) && MAX_SOCK_NUM <= 1
			cmd[2] = 0x14; // 16K buffers
#elif defined(ETHERNET_LARGE_BUFFERS) && MAX_SOCK_NUM <= 2
			cmd[2] = ((addr >> 8) & 0x20) | 0x14; // 8K buffers
#elif defined(ETHERNET_LARGE_BUFFERS) && MAX_SOCK_NUM <= 4
			cmd[2] = ((addr >> 7) & 0x60) | 0x14; // 4K buffers
#else
			cmd[2] = ((addr >> 6) & 0xE0) | 0x14; // 2K buffers
#endif
		}
		else
		{
			// receive buffers
			cmd[0] = addr >> 8;
			cmd[1] = addr & 0xFF;
#if defined(ETHERNET_LARGE_BUFFERS) && MAX_SOCK_NUM <= 1
			cmd[2] = 0x1C; // 16K buffers
#elif defined(ETHERNET_LARGE_BUFFERS) && MAX_SOCK_NUM <= 2
			cmd[2] = ((addr >> 8) & 0x20) | 0x1C; // 8K buffers
#elif defined(ETHERNET_LARGE_BUFFERS) && MAX_SOCK_NUM <= 4
			cmd[2] = ((addr >> 7) & 0x60) | 0x1C; // 4K buffers
#else
			cmd[2] = ((addr >> 6) & 0xE0) | 0x1C; // 2K buffers
#endif
		}
		if (len <= 5)
		{
			for (uint8_t i = 0; i < len; i++)
			{
				cmd[i + 3] = buf[i];
			}
			W5500_CHIP.spi_write(cmd, len + 3);
		}
		else
		{
			W5500_CHIP.spi_write(cmd, 3);
#ifdef SPI_HAS_TRANSFER_BUF
			W5500_CHIP.spi_write(buf, len);
#else
			// TODO: copy 8 bytes at a time to cmd[] and block transfer
			for (uint16_t i = 0; i < len; i++)
			{
				W5500_CHIP.spi_write(&(buf[i]), 1);
			}
#endif
		}
		W5500_CHIP._deselect();
	}
	return len;
}

static uint16_t read(uint16_t addr, uint8_t *buf, uint16_t len)
{
	uint8_t cmd[4];

	DEBUG("read: addr:%X len:%d\r\n", addr, len);

	//XXX:not support now
	// 	if (_W5100.chip == 51)
	// 	{
	// 		for (uint16_t i = 0; i < len; i++)
	// 		{
	// 			W5500_CHIP._select();
	// #if 1
	// 			SPI.transfer(0x0F);
	// 			SPI.transfer(addr >> 8);
	// 			SPI.transfer(addr & 0xFF);
	// 			addr++;
	// 			buf[i] = SPI.transfer(0);
	// #else
	// 			cmd[0] = 0x0F;
	// 			cmd[1] = addr >> 8;
	// 			cmd[2] = addr & 0xFF;
	// 			cmd[3] = 0;
	// 			SPI.transfer(cmd, 4); // TODO: why doesn't this work?
	// 			buf[i] = cmd[3];
	// 			addr++;
	// #endif
	// 			W5500_CHIP._deselect();
	// 		}
	// 	}
	// 	else if (chip == 52)
	// 	{
	// 		W5500_CHIP._select();
	// 		cmd[0] = addr >> 8;
	// 		cmd[1] = addr & 0xFF;
	// 		cmd[2] = (len >> 8) & 0x7F;
	// 		cmd[3] = len & 0xFF;
	// 		SPI.transfer(cmd, 4);
	// 		memset(buf, 0, len);
	// 		SPI.transfer(buf, len);
	// 		W5500_CHIP._deselect();
	// 	}
	// 	else
	{ // chip == 55
		W5500_CHIP._select();
		if (addr < 0x100)
		{
			// common registers 00nn
			cmd[0] = 0;
			cmd[1] = addr & 0xFF;
			cmd[2] = 0x00;
		}
		else if (addr < 0x8000)
		{
			// socket registers  10nn, 11nn, 12nn, 13nn, etc
			cmd[0] = 0;
			cmd[1] = addr & 0xFF;
			cmd[2] = ((addr >> 3) & 0xE0) | 0x08;
		}
		else if (addr < 0xC000)
		{
			// transmit buffers  8000-87FF, 8800-8FFF, 9000-97FF, etc
			//  10## #nnn nnnn nnnn
			cmd[0] = addr >> 8;
			cmd[1] = addr & 0xFF;
#if defined(ETHERNET_LARGE_BUFFERS) && MAX_SOCK_NUM <= 1
			cmd[2] = 0x10; // 16K buffers
#elif defined(ETHERNET_LARGE_BUFFERS) && MAX_SOCK_NUM <= 2
			cmd[2] = ((addr >> 8) & 0x20) | 0x10; // 8K buffers
#elif defined(ETHERNET_LARGE_BUFFERS) && MAX_SOCK_NUM <= 4
			cmd[2] = ((addr >> 7) & 0x60) | 0x10; // 4K buffers
#else
			cmd[2] = ((addr >> 6) & 0xE0) | 0x10; // 2K buffers
#endif
		}
		else
		{
			// receive buffers
			cmd[0] = addr >> 8;
			cmd[1] = addr & 0xFF;
#if defined(ETHERNET_LARGE_BUFFERS) && MAX_SOCK_NUM <= 1
			cmd[2] = 0x18; // 16K buffers
#elif defined(ETHERNET_LARGE_BUFFERS) && MAX_SOCK_NUM <= 2
			cmd[2] = ((addr >> 8) & 0x20) | 0x18; // 8K buffers
#elif defined(ETHERNET_LARGE_BUFFERS) && MAX_SOCK_NUM <= 4
			cmd[2] = ((addr >> 7) & 0x60) | 0x18; // 4K buffers
#else
			cmd[2] = ((addr >> 6) & 0xE0) | 0x18; // 2K buffers
#endif
		}
		W5500_CHIP.spi_write(cmd, 3);
		memset(buf, 0, len);
		W5500_CHIP.spi_read(buf, len);
		W5500_CHIP._deselect();
	}
	return len;
}

static void execCmdSn(SOCKET s, enum SockCMD _cmd)
{
	// Send command to socket
	W5100_writeSnCR(s, _cmd);
	// Wait for command to complete
	while (W5100_readSnCR(s))
		;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// W5100 controller instance
uint8_t W5100_init(W5100_t *w5100)
{
	w5100->chip = 0;
	w5100->CH_SIZE = 0x0100;
#ifdef ETHERNET_LARGE_BUFFERS
	w5100->SSIZE = 2048;
	w5100->SMASK = 0x07FF;
#endif

	w5100->init = init;
	w5100->soft_reset = softReset;

	w5100->isW5100 = isW5100;
	w5100->isW5200 = isW5200;
	w5100->isW5500 = isW5500;

	w5100->getLinkStatus = getLinkStatus;

	w5100->read = read;
	w5100->write = write;

	W5100->execCmdSn = execCmdSn;

	//exec init
	return w5100->init();
}

unsigned long millis(void)
{
	return (unsigned long)(sysctl_get_time_us() / 1000);
}
