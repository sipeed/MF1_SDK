#ifndef __LCD_SPIEED_H
#define __LCD_SPIEED_H

#include <stdint.h>

#include "spi.h"
#include "lcd.h"

#include "global_config.h"

/* clang-format off */
///////////////////////////////////////////////////////////////////////////////
#define LCD_BL_ON                       (0x61)
#define LCD_BL_OFF                      (0x60)
#define LCD_DISOLAY_ON                  (0x30)
#define LCD_FRAME_START                 (0x41)
#define LCD_WRITE_REG                   (0x80)

///////////////////////////////////////////////////////////////////////////////
#define LCD_SCALE_DISABLE				(0x00)
#define LCD_SCALE_ENABLE				(0x01)

#define LCD_SCALE_NONE					(0x00)
#define LCD_SCALE_2X2					(0x01)
#define LCD_SCALE_3X3					(0x02)
#define LCD_SCALE_4X4					(0x03)

#define LCD_SCALE_X						(0x00)
#define LCD_SCALE_Y						(0x01)

///////////////////////////////////////////////////////////////////////////////
#define LCD_ADDR_VBP					(0x00)
#define LCD_ADDR_H						(0x01)
#define LCD_ADDR_VFP					(0x02)
#define LCD_ADDR_HBP					(0x03)
#define LCD_ADDR_W						(0x04)
#define LCD_ADDR_HFP					(0x05)
#define LCD_ADDR_DIVCFG					(0x06)	//b7:0 dis,1 en; b6: 0x,1y; b[5:3]: mul part1; b[2:0]: mul part2
#define LCD_ADDR_POS					(0x07)

///////////////////////////////////////////////////////////////////////////////
#define LCD_SIPEED_SPI_DEV              (SPI_DEVICE_0)
#define LCD_SIPEED_SPI_SS               (SPI_CHIP_SELECT_3)

/* 实用最高频率100M，更高FPC延长线无法工作，需要onboard。 */
/* 另外FPGA的FIFO IP读写时钟速率限制在120M左右 */
#define LCD_SIPEED_SPI_FREQ             (100000000)

///////////////////////////////////////////////////////////////////////////////
/* 这些时序设置最好都是偶数 */
#if CONFIG_TYPE_800_480_57_INCH
#define LCD_INIT_LINE       (2) /* 必须为偶数 */
#define LCD_LINE_FIX_PIXEL  (0)

#define LCD_WIDTH           (800)
#define LCD_HEIGHT          (480)

#define LCD_HBP             (210)
#define LCD_HFP             (44)

#define LCD_VBP             (22)
#define LCD_VFP             (22)
#elif CONFIG_TYPE_480_272_4_3_INCH

#elif CONFIG_TYPE_1024_600
#error "Not Support LCD Type"
#else
#error "Unknown LCD Type"
#endif

/* clang-format on */

extern volatile uint8_t dis_flag;

int lcd_sipeed_init(lcd_t *lcd);
uint8_t lcd_sipeed_config_disp_buf(uint8_t *lcd_disp_buf, uint8_t *lcd_disp_banner_buf);

#endif /* __LCD_SPIEED_H */