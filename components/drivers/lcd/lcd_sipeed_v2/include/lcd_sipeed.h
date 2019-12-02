#ifndef __LCD_SPIEED_H
#define __LCD_SPIEED_H

#include <stdint.h>

#include "spi.h"
#include "lcd.h"

#include "global_config.h"

/* clang-format off */
///////////////////////////////////////////////////////////////////////////////
#define LCD_SIPEED_SPI_DEV              (SPI_DEVICE_0)
#define LCD_SIPEED_SPI_SS               (SPI_CHIP_SELECT_3)
#define LCD_SIPEED_SPI_FREQ             (100000000)
//实用最高频率100M，更高FPC延长线无法工作，需要onboard。另外FPGA的FIFO IP读写时钟速率限制在120M左右

///////////////////////////////////////////////////////////////////////////////
#define LCD_WRITE_REG                   (0x80)

#define LCD_SCALE_DISABLE				(0x00)
#define LCD_SCALE_ENABLE				(0x01)

#define LCD_SCALE_NONE					(0x00)
#define LCD_SCALE_2X2					(0x01)
#define LCD_SCALE_3X3					(0x02)
#define LCD_SCALE_4X4					(0x03)

#define LCD_SCALE_X						(0x00)
#define LCD_SCALE_Y						(0x01)

#define LCD_PHASE_PCLK                  (0x00)
#define LCD_PHASE_HSYNC                 (0x01)
#define LCD_PHASE_VSYNC                 (0x02)
#define LCD_PHASE_DE                    (0x03)

#define LCD_ADDR_VBP					(0x00)
#define LCD_ADDR_H						(0x01)
#define LCD_ADDR_VFP					(0x02)
#define LCD_ADDR_HBP					(0x03)
#define LCD_ADDR_W						(0x04)
#define LCD_ADDR_HFP					(0x05)
#define LCD_ADDR_DIVCFG					(0x06)	//b7:0 dis,1 en; b6: 0x,1y; b[5:3]: mul part1; b[2:0]: mul part2
#define LCD_ADDR_POS					(0x07)
#define LCD_ADDR_PHASE                  (0x08)  //b0: pclk b1: hsync b2:vsync b3:de
#define LCD_ADDR_START                  (0x09)
#define LCD_ADDR_INITDONE               (0x0A)

///////////////////////////////////////////////////////////////////////////////
#if CONFIG_TYPE_480_800_4_INCH
#define LCD_VBP						(30)
#define LCD_VFP						(12)

#define LCD_INIT_LINE				(2)	/* 必须为偶数 */
#define LCD_LINE_FIX_PIXEL          (1)
#elif CONFIG_TYPE_480_854_5_INCH
#define LCD_VBP						(30)
#define LCD_VFP						(12)

#define LCD_INIT_LINE				(2)
#define LCD_LINE_FIX_PIXEL          (1)
#elif CONFIG_TYPE_360_640_4_4_INCH
#define LCD_VBP                     (30)
#define LCD_VFP                     (12)
#define LCD_INIT_LINE               (2)	
#elif CONFIG_TYPE_800_480_57_INCH
#define LCD_VBP                     (30)
#define LCD_VFP                     (12)
#define LCD_INIT_LINE               (2)	
#define LCD_LINE_FIX_PIXEL          (1)
///////////////////////////////////////////////////////////////////////////////
#endif

/* clang-format on */

extern volatile uint8_t dis_flag;

int lcd_sipeed_init(lcd_t *lcd);
uint8_t lcd_sipeed_config_disp_buf(uint8_t *lcd_disp_buf, uint8_t *lcd_disp_banner_buf);

#endif /* __LCD_SPIEED_H */
