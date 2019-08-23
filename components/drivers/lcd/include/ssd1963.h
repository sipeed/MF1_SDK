#ifndef __SSD1963_H
#define __SSD1963_H

#include <stdint.h>
#include "lcd_sipeed.h"

#if CONFIG_LCD_TYPE_SSD1963

void tft_hard_init(void);
void tft_ssd1963_write_cmd(uint8_t cmd);
void tft_ssd1963_write_data(uint8_t *data_buf, uint32_t length);

#endif
#endif