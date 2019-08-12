#ifndef _BOARD_H
#define _BOARD_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include "face_lib.h"
#include "flash.h"

#include "system_config.h"

#define RLED 1
#define GLED 2
#define BLED 4

#define PLL0_OUTPUT_FREQ 800000000UL
#define PLL1_OUTPUT_FREQ 400000000UL

extern volatile uint8_t g_key_press;
extern volatile uint8_t g_key_long_press;

extern uint8_t sKey_dir;

extern volatile board_cfg_t g_board_cfg;

extern uint8_t kpu_image[2][IMG_W * IMG_H * 3];
extern uint8_t display_image[IMG_W * IMG_H * 2];

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

int irq_gpiohs(void *ctx);

void set_IR_LED(int state);

void set_RGB_LED(int state);
void get_date_time(void);
void update_key_state(void);

void board_init(void);


#endif