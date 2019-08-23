#ifndef __LCD_SPIEED_H
#define __LCD_SPIEED_H

#include <stdint.h>

#include "lcd_config.h"

#if CONFIG_LCD_TYPE_SIPEED

#include "spi.h"

/* clang-format off */
#define LCD_BL_ON                       (0x61)
#define LCD_BL_OFF                      (0x60)
#define LCD_DISOLAY_ON                  (0x30)
#define LCD_FRAME_START                 (0x41)

#define LCDDisMode_normal               (0x01)
#define LCDDisMode_TranBK               (0x00)
#define LCDDisMode_Reverse              (0x10)

#define LCD_SIPEED_SPI_DEV              (SPI_DEVICE_0)
#define LCD_SIPEED_SPI_SS               (SPI_CHIP_SELECT_3)
#define LCD_SIPEED_SPI_FREQ             (35000000)

#define LCD_SIPEED_FRAME_END_LINE       (LCD_H * 2 + 63 - 1)
/* clang-format on */

extern volatile uint8_t dis_flag;

void lcd_init(void);

void LCDPrintStr(uint8_t *img, uint16_t XPos, uint16_t YPos, char *Srt,
                 uint8_t Mode, uint16_t FontColor, uint16_t BKColor);

uint8_t lcd_covert_cam_order(uint32_t *addr, uint32_t length);

void copy_image_cma_to_lcd(uint8_t *cam_img, uint8_t *lcd_img);

/* flush img_buf to lcd */
void lcd_sipeed_display(uint8_t *img_buf, uint8_t block);

#endif /* CONFIG_LCD_TYPE_SIPEED */
#endif /* __LCD_SPIEED_H */