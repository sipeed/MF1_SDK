#include "lcd.h"

#include <stddef.h>
#include <stdio.h>

#include "lcd_st7789.h"

static lcd_t lcd = {
    .dir = 0x0,
    .width = 0,
    .height = 0,

    .lcd_config = NULL,
    .lcd_clear = NULL,
    .lcd_set_direction = NULL,
    .lcd_set_area = NULL,
    .lcd_draw_picture = NULL,
};

int lcd_set_direction(lcd_dir_t dir)
{
    printf("lcd_set_direction: %02X\r\n", dir);

    if(lcd.lcd_set_direction)
    {
        lcd.lcd_set_direction(&lcd, dir);
        return 0;
    }
    return 1;
}

int lcd_set_area(uint16_t x, uint16_t y,
                 uint16_t w, uint16_t h)
{
    if(lcd.lcd_set_area)
    {
        lcd.lcd_set_area(&lcd, x, y, w, h);
        return 0;
    }
    return 1;
}

int lcd_draw_picture(uint16_t x, uint16_t y,
                     uint16_t w, uint16_t h,
                     uint32_t *imamge)
{
    if(lcd.lcd_draw_picture)
    {
        lcd.lcd_draw_picture(&lcd, x, y, w, h, imamge);
        return 0;
    }
    return 1;
}

int lcd_clear(uint16_t rgb565_color)
{
    if(lcd.lcd_clear)
    {
        lcd.lcd_clear(&lcd, rgb565_color);
        return 0;
    }
    return 1;
}

int lcd_init(lcd_type_t lcd_type)
{
    switch(lcd_type)
    {
        case LCD_ST7789:
        {
            lcd_st7789_init(&lcd);
            lcd.lcd_config(&lcd);
            return 0;
        }
        break;
        case LCD_SIPEED:
        default:
            break;
    }
    return 1;
}
