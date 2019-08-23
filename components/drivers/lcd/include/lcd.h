#ifndef __LCD_H
#define __LCD_H

#include "lcd_config.h"
#include "stdint.h"


#if CONFIG_LCD_TYPE_SIPEED 
extern uint8_t lcd_image[LCD_W * LCD_H * 2]; 
#endif 
 
#if CONFIG_LCD_TYPE_ST7789 
#if LCD_240240 
extern uint8_t lcd_image[LCD_W * LCD_H * 2]; 
#else 
extern const uint8_t *lcd_image; 
#endif 
#endif

#endif

