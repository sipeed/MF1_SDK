#ifndef __MYSPI_H
#define __MYSPI_H

#include <stdint.h>

// extern volatile uint8_t w5500_irq_flag;

void eth_w5500_spi_init(void);
void eth_w5500_reset(uint8_t val);

//lib call
void eth_w5500_spi_cs_sel(void);
void eth_w5500_spi_cs_desel(void);

void eth_w5500_spi_write(uint8_t *send, size_t len);
void eth_w5500_spi_read(uint8_t *recv, size_t len);
#endif
