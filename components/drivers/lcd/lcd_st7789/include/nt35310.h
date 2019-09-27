#ifndef _NT35310_H_
#define _NT35310_H_

#include <stdint.h>

void tft_hard_init(void);
void tft_write_command(uint8_t cmd);
void tft_write_byte(uint8_t *data_buf, uint32_t length);
void tft_write_half(uint16_t *data_buf, uint32_t length);
void tft_write_word(uint32_t *data_buf, uint32_t length, uint32_t flag);
void tft_fill_data(uint32_t *data_buf, uint32_t length);

#endif
