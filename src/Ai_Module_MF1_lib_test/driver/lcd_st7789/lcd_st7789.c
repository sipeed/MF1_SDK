/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "lcd_st7789.h"
#include <string.h>
#include <unistd.h>
#include "../font.h"

#include "nt35310.h"
#include "stdio.h"

#if CONFIG_LCD_TYPE_ST7789
/* clang-format off */
#define NO_OPERATION            0x00
#define SOFTWARE_RESET          0x01
#define READ_ID                 0x04
#define READ_STATUS             0x09
#define READ_POWER_MODE         0x0A
#define READ_MADCTL             0x0B
#define READ_PIXEL_FORMAT       0x0C
#define READ_IMAGE_FORMAT       0x0D
#define READ_SIGNAL_MODE        0x0E
#define READ_SELT_DIAG_RESULT   0x0F
#define SLEEP_ON                0x10
#define SLEEP_OFF               0x11
#define PARTIAL_DISPALY_ON      0x12
#define NORMAL_DISPALY_ON       0x13
#define INVERSION_DISPALY_OFF   0x20
#define INVERSION_DISPALY_ON    0x21
#define GAMMA_SET               0x26
#define DISPALY_OFF             0x28
#define DISPALY_ON              0x29
#define HORIZONTAL_ADDRESS_SET  0x2A
#define VERTICAL_ADDRESS_SET    0x2B
#define MEMORY_WRITE            0x2C
#define COLOR_SET               0x2D
#define MEMORY_READ             0x2E
#define PARTIAL_AREA            0x30
#define VERTICAL_SCROL_DEFINE   0x33
#define TEAR_EFFECT_LINE_OFF    0x34
#define TEAR_EFFECT_LINE_ON     0x35
#define MEMORY_ACCESS_CTL       0x36
#define VERTICAL_SCROL_S_ADD    0x37
#define IDLE_MODE_OFF           0x38
#define IDLE_MODE_ON            0x39
#define PIXEL_FORMAT_SET        0x3A
#define WRITE_MEMORY_CONTINUE   0x3C
#define READ_MEMORY_CONTINUE    0x3E
#define SET_TEAR_SCANLINE       0x44
#define GET_SCANLINE            0x45
#define WRITE_BRIGHTNESS        0x51
#define READ_BRIGHTNESS         0x52
#define WRITE_CTRL_DISPALY      0x53
#define READ_CTRL_DISPALY       0x54
#define WRITE_BRIGHTNESS_CTL    0x55
#define READ_BRIGHTNESS_CTL     0x56
#define WRITE_MIN_BRIGHTNESS    0x5E
#define READ_MIN_BRIGHTNESS     0x5F
#define READ_ID1                0xDA
#define READ_ID2                0xDB
#define READ_ID3                0xDC
#define RGB_IF_SIGNAL_CTL       0xB0
#define NORMAL_FRAME_CTL        0xB1
#define IDLE_FRAME_CTL          0xB2
#define PARTIAL_FRAME_CTL       0xB3
#define INVERSION_CTL           0xB4
#define BLANK_PORCH_CTL         0xB5
#define DISPALY_FUNCTION_CTL    0xB6
#define ENTRY_MODE_SET          0xB7
#define BACKLIGHT_CTL1          0xB8
#define BACKLIGHT_CTL2          0xB9
#define BACKLIGHT_CTL3          0xBA
#define BACKLIGHT_CTL4          0xBB
#define BACKLIGHT_CTL5          0xBC
#define BACKLIGHT_CTL7          0xBE
#define BACKLIGHT_CTL8          0xBF
#define POWER_CTL1              0xC0
#define POWER_CTL2              0xC1
#define VCOM_CTL1               0xC5
#define VCOM_CTL2               0xC7
#define NV_MEMORY_WRITE         0xD0
#define NV_MEMORY_PROTECT_KEY   0xD1
#define NV_MEMORY_STATUS_READ   0xD2
#define READ_ID4                0xD3
#define POSITIVE_GAMMA_CORRECT  0xE0
#define NEGATIVE_GAMMA_CORRECT  0xE1
#define DIGITAL_GAMMA_CTL1      0xE2
#define DIGITAL_GAMMA_CTL2      0xE3
#define INTERFACE_CTL           0xF6
/* clang-format on */

static lcd_ctl_t lcd_ctl;

void lcd_polling_enable(void)
{
    lcd_ctl.mode = 0;
}

void lcd_interrupt_enable(void)
{
    lcd_ctl.mode = 1;
}

void lcd_init(void)
{
    uint8_t data = 0;

    tft_hard_init();
    /*soft reset*/
    tft_write_command(SOFTWARE_RESET);
    usleep(6 * 1000);
    /*exit sleep*/
    tft_write_command(SLEEP_OFF);
    usleep(6 * 1000);
    /*pixel format*/
    tft_write_command(PIXEL_FORMAT_SET);
    data = 0x05;
    tft_write_byte(&data, 1);

#if LCD_320240
    lcd_set_direction(DIR_YX_LRDU);
#endif

#if LCD_240240
#if LCD_ROTATE
    lcd_set_direction(DIR_XY_RLUD);
#else
    lcd_set_direction(DIR_YX_LRUD);
#endif

    tft_write_command(INVERSION_DISPALY_ON);
    tft_write_command(WRITE_BRIGHTNESS_CTL); //CACB
    data = 0xb3;
    tft_write_byte(&data, 1);
    tft_write_command(WRITE_MIN_BRIGHTNESS);
    data = 0x80;
    tft_write_byte(&data, 1);
#endif
/*display on*/
#if LCD_REVERSE

#endif
    tft_write_command(DISPALY_ON);
    lcd_polling_enable();
}

void lcd_set_direction(lcd_dir_t dir)
{
    uint16_t tmp = 0;

    lcd_ctl.dir = dir;
    if(dir & DIR_XY_MASK)
    {
        lcd_ctl.width = LCD_Y_MAX - 1;
        lcd_ctl.height = LCD_X_MAX - 1;
        if(lcd_ctl.width < lcd_ctl.height)
        {
            tmp = lcd_ctl.width;
            lcd_ctl.width = lcd_ctl.height;
            lcd_ctl.height = tmp;
        }
    } else
    {
        lcd_ctl.width = LCD_X_MAX - 1;
        lcd_ctl.height = LCD_Y_MAX - 1;
        if(lcd_ctl.width > lcd_ctl.height)
        {
            tmp = lcd_ctl.width;
            lcd_ctl.width = lcd_ctl.height;
            lcd_ctl.height = tmp;
        }
    }
    tft_write_command(MEMORY_ACCESS_CTL);
    tft_write_byte((uint8_t *)&dir, 1);
}

void lcd_set_area(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    uint8_t data[4] = {0};
    uint16_t ly1, ly2;

    data[0] = (uint8_t)(x1 >> 8);
    data[1] = (uint8_t)(x1);
    data[2] = (uint8_t)(x2 >> 8);
    data[3] = (uint8_t)(x2);
    tft_write_command(HORIZONTAL_ADDRESS_SET);
    tft_write_byte(data, 4);

    ly1 = y1;
    ly2 = y2;

    // #if LCD_240240
    // #if LCD_ROTATE
    //     ly1 += 80;
    //     ly2 += 80;
    // #endif
    // #endif

    data[0] = (uint8_t)(ly1 >> 8);
    data[1] = (uint8_t)(ly1);
    data[2] = (uint8_t)(ly2 >> 8);
    data[3] = (uint8_t)(ly2);
    tft_write_command(VERTICAL_ADDRESS_SET);
    tft_write_byte(data, 4);

    tft_write_command(MEMORY_WRITE);
}

void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_set_area(x, y, x, y);
    tft_write_half(&color, 1);
}

void lcd_draw_char(uint16_t x, uint16_t y, char c, uint16_t color)
{
    uint8_t i = 0;
    uint8_t j = 0;
    uint8_t data = 0;

    for(i = 0; i < 16; i++)
    {
        data = ascii0816[c * 16 + i];
        for(j = 0; j < 8; j++)
        {
            if(data & 0x80)
                lcd_draw_point(x + j, y, color);
            data <<= 1;
        }
        y++;
    }
}

void lcd_draw_string(uint16_t x, uint16_t y, char *str, uint16_t color)
{
    while(*str)
    {
        lcd_draw_char(x, y, *str, color);
        str++;
        x += 8;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void lcd_draw_char_underlap(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg_color)
{
    uint8_t i = 0;
    uint8_t j = 0;
    uint8_t data = 0;

    for(i = 0; i < 16; i++)
    {
        data = ascii0816[c * 16 + i];
        for(j = 0; j < 8; j++)
        {
            if(data & 0x80)
                lcd_draw_point(x + j, y, color);
            else
                lcd_draw_point(x + j, y, bg_color);
            data <<= 1;
        }
        y++;
    }
}

void lcd_draw_string_underlap(uint16_t x, uint16_t y, char *str, uint16_t color, uint16_t bg_color)
{
    while(*str)
    {
        lcd_draw_char_underlap(x, y, *str, color, bg_color);
        str++;
        x += 8;
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ram_draw_char(uint32_t *ptr, uint16_t x, uint16_t y, char c, uint16_t color)
{
    uint8_t i, j, data;
    uint16_t *addr;

    for(i = 0; i < 16; i++)
    {
        addr = ((uint16_t *)ptr) + y * (IMG_W + 0) + x;
        data = ascii0816[c * 16 + i];
        for(j = 0; j < 8; j++)
        {
            if(data & 0x80)
            {
                if((x + j) & 1)
                    *(addr - 1) = color;
                else
                    *(addr + 1) = color;
            }
            data <<= 1;
            addr++;
        }
        y++;
    }
}

void ram_draw_string(uint32_t *ptr, uint16_t x, uint16_t y, char *str, uint16_t color)
{
    while(*str)
    {
        ram_draw_char(ptr, x, y, *str, color);
        str++;
        x += 8;
    }
}

void lcd_ram_draw_string(char *str, uint32_t *ptr, uint16_t font_color, uint16_t bg_color)
{
    uint8_t i = 0;
    uint8_t j = 0;
    uint8_t data = 0;
    uint8_t *pdata = NULL;
    uint16_t width = 0;
    uint32_t *pixel = NULL;

    width = 4 * strlen(str);
    while(*str)
    {
        pdata = (uint8_t *)&ascii0816[(*str) * 16];
        for(i = 0; i < 16; i++)
        {
            data = *pdata++;
            pixel = ptr + i * width;
            for(j = 0; j < 4; j++)
            {
                switch(data >> 6)
                {
                    case 0:
                        *pixel = ((uint32_t)bg_color << 16) | bg_color;
                        break;
                    case 1:
                        *pixel = ((uint32_t)bg_color << 16) | font_color;
                        break;
                    case 2:
                        *pixel = ((uint32_t)font_color << 16) | bg_color;
                        break;
                    case 3:
                        *pixel = ((uint32_t)font_color << 16) | font_color;
                        break;
                    default:
                        *pixel = 0;
                        break;
                }
                data <<= 2;
                pixel++;
            }
        }
        str++;
        ptr += 4;
    }
}

void lcd_clear(uint16_t color)
{
    uint32_t data = ((uint32_t)color << 16) | (uint32_t)color;

    lcd_set_area(0, 0, lcd_ctl.width, lcd_ctl.height);
    tft_fill_data(&data, LCD_X_MAX * LCD_Y_MAX / 2);
}

void lcd_fill_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    uint32_t data = ((uint32_t)color << 16) | (uint32_t)color;

    lcd_set_area(x1, y1, x2, y2);
    tft_fill_data(&data, (x2 - x1) * (y2 - y1) / 2);
    return;
}

void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t width, uint16_t color)
{
    uint32_t data_buf[640] = {0};
    uint32_t *p = data_buf;
    uint32_t data = color;
    uint32_t index = 0;

    data = (data << 16) | data;
    for(index = 0; index < 160 * width; index++)
        *p++ = data;

    lcd_set_area(x1, y1, x2, y1 + width - 1);
    tft_write_word(data_buf, ((x2 - x1 + 1) * width + 1) / 2, 0);
    lcd_set_area(x1, y2 - width + 1, x2, y2);
    tft_write_word(data_buf, ((x2 - x1 + 1) * width + 1) / 2, 0);
    lcd_set_area(x1, y1, x1 + width - 1, y2);
    tft_write_word(data_buf, ((y2 - y1 + 1) * width + 1) / 2, 0);
    lcd_set_area(x2 - width + 1, y1, x2, y2);
    tft_write_word(data_buf, ((y2 - y1 + 1) * width + 1) / 2, 0);
}

void lcd_draw_picture(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint32_t *ptr)
{
    lcd_set_area(x1, y1, x1 + width - 1, y1 + height - 1);
    tft_write_word(ptr, width * height / 2, lcd_ctl.mode ? 2 : 0);
}

#endif