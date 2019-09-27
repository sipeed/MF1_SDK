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
	#define LCD_SIPEED_SPI_FREQ  		(100000000)//50000000
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
uint8_t lcd_sipeed_config_disp_buf(uint8_t *lcd_disp_buf, uint8_t *lcd_disp_banner_buf);

#endif /* __LCD_SPIEED_H */