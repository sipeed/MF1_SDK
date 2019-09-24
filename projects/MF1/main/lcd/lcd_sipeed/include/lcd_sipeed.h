#ifndef __LCD_SPIEED_H
#define __LCD_SPIEED_H

#include <stdint.h>

#include "spi.h"
#include "lcd.h"

#include "global_config.h"

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

#if CONFIG_TYPE_800_480_57_INCH
	#define LCD_XUP						(2)
	#define LCD_YUP						(2)
	#define LCD_VFP						(59)
	#define LCD_SIPEED_SPI_FREQ 		(100000000)	
	//实用最高频率100M，更高FPC延长线无法工作，需要onboard。另外FPGA的FIFO IP读写时钟速率限制在120M左右
#elif CONFIG_TYPE_480_272_4_3_INCH
	#define LCD_XUP						(1)
	#define LCD_YUP						(1)
	#define LCD_VFP						(8)
	#define LCD_SIPEED_SPI_FREQ  		(50000000)
#elif CONFIG_TYPE_1024_600
	#define LCD_XUP						(3)
	#define LCD_YUP						(3)
	#define LCD_VFP						(72)
	#define LCD_SIPEED_SPI_FREQ  		(30000000)
#else
	#define LCD_XUP						(3)
	#define LCD_YUP						(3)
	#define LCD_VFP						(72)
	#define LCD_SIPEED_SPI_FREQ  		(30000000)
#endif

#define LCD_SIPEED_FRAME_END_LINE   (SIPEED_LCD_H * LCD_YUP + LCD_VFP - 1)
/* clang-format on */

extern volatile uint8_t dis_flag;

int lcd_sipeed_init(lcd_t *lcd);

void LCDPrintStr(uint8_t *img, uint16_t XPos, uint16_t YPos, char *Srt,
				 uint8_t Mode, uint16_t FontColor, uint16_t BKColor);

uint8_t lcd_covert_cam_order(uint32_t *addr, uint32_t length);
void copy_image_cam_to_lcd(uint8_t *cam_img, uint8_t *lcd_img);

/* flush img_buf to lcd */
// void lcd_sipeed_display(uint8_t *img_buf, uint8_t block);

#endif /* __LCD_SPIEED_H */