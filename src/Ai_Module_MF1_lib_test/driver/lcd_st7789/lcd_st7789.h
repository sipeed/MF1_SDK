#ifndef _LCD_ST7789_H_
#define _LCD_ST7789_H_

#include <stdint.h>
#include "system_config.h"

#if CONFIG_LCD_TYPE_ST7789
/* clang-format off */
#define LCD_X_MAX   (LCD_W)
#define LCD_Y_MAX   (LCD_H)

#define BLACK       0x0000
#define NAVY        0x000F
#define DARKGREEN   0x03E0
#define DARKCYAN    0x03EF
#define MAROON      0x7800
#define PURPLE      0x780F
#define OLIVE       0x7BE0
#define LIGHTGREY   0xC618
#define DARKGREY    0x7BEF
#define BLUE        0x001F
#define GREEN       0x07E0
#define CYAN        0x07FF
#define RED         0xF800
#define MAGENTA     0xF81F
#define YELLOW      0xFFE0
#define WHITE       0xFFFF
#define ORANGE      0xFD20
#define GREENYELLOW 0xAFE5
#define PINK        0xF81F
#define USER_COLOR  0xAA55
/* clang-format on */

typedef enum _lcd_dir
{
    DIR_XY_RLUD = 0x00,
    DIR_YX_RLUD = 0x20,
    DIR_XY_LRUD = 0x40,
    DIR_YX_LRUD = 0x60,
    DIR_XY_RLDU = 0x80,
    DIR_YX_RLDU = 0xA0,
    DIR_XY_LRDU = 0xC0,
    DIR_YX_LRDU = 0xE0,
    DIR_XY_MASK = 0x20,
    DIR_MASK = 0xE0,
} lcd_dir_t;

typedef struct _lcd_ctl
{
    uint8_t mode;
    uint8_t dir;
    uint16_t width;
    uint16_t height;
} lcd_ctl_t;

void lcd_init(void);
void lcd_set_direction(lcd_dir_t dir);
void lcd_set_area(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void lcd_clear(uint16_t color);

void lcd_draw_picture(uint16_t x1, uint16_t y1,
                      uint16_t width, uint16_t height,
                      uint32_t *ptr);

#endif
#endif