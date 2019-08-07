/* Copyright 2018 Sipeed Inc.
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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "lcd_ssd1963.h"
#include "ssd1963.h"
#include "sleep.h"
#include "font.h"
#include "zh_CN_font.h"

#if CONFIG_LCD_TYPE_SSD1963
lcd_ctl_t lcd_ctl;

//x:800,y:480,depth:18,pclk_khz:33000,le:87,ri:40,up:31,lo:13,hs:1,vs:1,sync:3,vmode:0
ss1963_lcd_panel_t lcd_800x480_5inch = {
    .panel_type = PANEL_LCD,

    .clk_Mhz = 33,

    .width = 800,
    .height = 480,

    .hsync_front_porch = 87,
    .hsync_back_porch = 40,
    .hsync_pulse = 5,

    .vsync_front_porch = 31,
    .vsync_back_porch = 13,
    .vsync_pulse = 5,
};

//x:480,y:272,depth:18,pclk_khz:10000,le:42,ri:8,up:11,lo:4,hs:1,vs:1,sync:3,vmode:0
ss1963_lcd_panel_t lcd_480x272_4_3inch = {
    .panel_type = PANEL_LCD,

    .clk_Mhz = 10,

    .width = 480,
    .height = 272,

    .hsync_front_porch = 42,
    .hsync_back_porch = 8,
    .hsync_pulse = 5,

    .vsync_front_porch = 11,
    .vsync_back_porch = 4,
    .vsync_pulse = 5,
};

//x:800,y:600,depth:24,pclk_khz:40000,le:88,ri:40,up:19,lo:5,hs:128,vs:4,sync:3,vmode:0
/* 这个分辨率超出了SSD1963的FB大小，所以不实用！！！ */
ss1963_lcd_panel_t lcd_vga_module = {
    .panel_type = PANEL_VGA_MODULE,

    .clk_Mhz = 40,

    .width = 800,
    .height = 600,

    .hsync_front_porch = 88,
    .hsync_back_porch = 40,
    .hsync_pulse = 30,

    .vsync_front_porch = 60,
    .vsync_back_porch = 20,
    .vsync_pulse = 20,
};

static inline void WriteComm(uint8_t cmd)
{
    tft_ssd1963_write_cmd(cmd);
}

static inline void WriteData(uint8_t dat)
{
    tft_ssd1963_write_data(&dat, 1);
}

void lcd_init(ss1963_lcd_panel_t *lcd)
{
    uint16_t tmp;
    uint32_t lcd_clk_div;

    tft_hard_init();

#define PLL_100M 0
#define PLL_200M 1

#if PLL_100M //pll 100M
    //10M xtal,pll 100M
    WriteComm(0xE2); //set_pll_mn
    WriteData(0x1d); //(30-1)*10=300
    WriteData(0x02); //300/3=100
    WriteData(0x54); //????
#elif PLL_200M       //pll 200M
    //10M xtal,pll 200M
    WriteComm(0xE2); //set_pll_mn
    WriteData(0x3B); //(60-1)*10=600
    WriteData(0x02); //600/3=200
    WriteData(0x54); //????
#else
#error "please select ssd1963 pll freq"
#endif

    WriteComm(0xE0); //set_pll
    WriteData(0x01);
    msleep(30);
    WriteComm(0xE0);
    WriteData(0x03);
    msleep(50);

    WriteComm(0x01); // software reset
    msleep(50);

#if PLL_100M         //pll 100M
    WriteComm(0xE6); //Set LSHIFT(pixel clock) Frequency
    //100M pll lcd_clk 33M
    //(33M*1048576/100M)-1=0x547ad
    lcd_clk_div = lcd->clk_Mhz * 1048576 / 100 - 1;
    printf("lcd_clk_div:%08X\r\n", lcd_clk_div);
    WriteData((uint8_t)(lcd_clk_div >> 16 & 0xFF));
    WriteData((uint8_t)(lcd_clk_div >> 8 & 0xFF));
    WriteData((uint8_t)(lcd_clk_div & 0xFF));
#elif PLL_200M //pll 200M
    WriteComm(0xE6); //Set LSHIFT(pixel clock) Frequency
    lcd_clk_div = lcd->clk_Mhz * 1048576 / 200 - 1;
    printf("lcd_clk_div:%08X\r\n", lcd_clk_div);
    WriteData((uint8_t)(lcd_clk_div >> 16 & 0xFF));
    WriteData((uint8_t)(lcd_clk_div >> 8 & 0xFF));
    WriteData((uint8_t)(lcd_clk_div & 0xFF));
#else
#error "please select ssd1963 pll freq"
#endif

    WriteComm(0xB0);                          //set_lcd_mode
    WriteData(0x20 | (2 << lcd->panel_type)); //TFT Pannel 24 Bit
    WriteData(0x00);
    WriteData(((lcd->width - 1) >> 8) & 0xFF); //Set HDP
    WriteData((lcd->width - 1) & 0xFF);
    WriteData(((lcd->height - 1) >> 8) & 0xFF); //Set VDP
    WriteData((lcd->height - 1) & 0xFF);
    WriteData(0x00);
    msleep(50);

    WriteComm(0xB4); //set_hori_period,HSYNC
    //Set HT
    tmp = lcd->width + lcd->hsync_pulse + lcd->hsync_back_porch + lcd->hsync_front_porch;
    WriteData((((tmp - 1) >> 8) & 0x07));
    WriteData(((tmp - 1) & 0xFF));
    //Set HPS
    tmp = lcd->hsync_front_porch + lcd->hsync_pulse;
    WriteData((tmp - 1) / 256);
    WriteData((tmp - 1) & 0xFF);
    //Set HPW
    WriteData(lcd->hsync_pulse - 1);
    //SetLPS
    WriteData(0x00);
    WriteData(0x00);
    WriteData(0x00);

    WriteComm(0xB6); //set_vert_period,VSYNC
    //Set VT
    tmp = lcd->height + lcd->vsync_pulse + lcd->vsync_back_porch + lcd->vsync_front_porch;
    WriteData(((tmp - 1) >> 8) & 0x07);
    WriteData((tmp - 1) & 0xFF);
    //Set VPS
    tmp = lcd->vsync_front_porch + lcd->vsync_pulse;
    WriteData((tmp - 1) / 256);
    WriteData((tmp - 1) & 0xFF);
    //Set VPW
    WriteData(lcd->vsync_pulse);
    //Set FPS
    WriteData(0x00);
    WriteData(0x00);

    WriteComm(0x36); //set_address_mode
    WriteData(0x00); //default Horizontal

    WriteComm(0xF0); //set_pixel_data_interface
    WriteData(0x00); //0x0:8bit
    msleep(50);

    WriteComm(0xD4); /* Import */
    //TH1 = display width * display height * 3 * 0.1 /16
    tmp = lcd->width * lcd->height * 3 / 16 / 10;
    WriteData(0x00);
    WriteData((uint8_t)(tmp >> 8 & 0xFF));
    WriteData((uint8_t)(tmp & 0xFF));
    //TH2 = display width * display height * 3 * 0.25 /16
    tmp = lcd->width * lcd->height * 3 / 16 / 4;
    WriteData(0x00);
    WriteData((uint8_t)(tmp >> 8 & 0xFF));
    WriteData((uint8_t)(tmp & 0xFF));
    //TH3 = display width * display height * 3 * 0.6 /16
    tmp = lcd->width * lcd->height * 3 / 16 * 3 / 5;
    WriteData(0x00);
    WriteData((uint8_t)(tmp >> 8 & 0xFF));
    WriteData((uint8_t)(tmp & 0xFF));

    //TE, not use
    // WriteComm(0x44);
    // WriteData(0x00);
    // WriteData(0x1e);

    WriteComm(0x29); /* display on */
    msleep(50);

    lcd_ctl.dir = DIR_HORIZONTAL;

    lcd_ctl.width = lcd->width;
    lcd_ctl.height = lcd->height;

    lcd_ctl.display_width = lcd_ctl.width;
    lcd_ctl.display_height = lcd_ctl.height;
}

void lcd_set_dir(uint8_t val)
{
    lcd_ctl.dir = val;
    WriteComm(0x36);         //set_address_mode
    if (DIR_VERTICAL == val) /* 1 */
    {
        WriteData((0 << 7) | (1 << 6) | (1 << 5));
    }
    else if (DIR_HORIZONTAL_MIRROR == val) /* 2 */
    {
        WriteData((1 << 7) | (1 << 6) | (0 << 5));
    }
    else if (DIR_VERTICAL_FLIP == val) /* 3 */
    {
        WriteData((1 << 7) | (0 << 6) | (1 << 5));
    }
    else if (DIR_HORIZONTAL == val) /* 0 */
    {
        WriteData(0x00);
    }
    else
    {
        /* default */
        WriteData(0x00);
    }

    if (val % 2)
    {
        lcd_ctl.display_width = lcd_ctl.height;
        lcd_ctl.display_height = lcd_ctl.width;
    }
    else
    {
        lcd_ctl.display_width = lcd_ctl.width;
        lcd_ctl.display_height = lcd_ctl.height;
    }
}

void lcd_set_area(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height)
{
    uint8_t data[4] = {0};
    uint16_t tmp = 0, xend = 0, yend = 0;

    /* 这里需要保证画图的大小不超过LCD的实际大小 */

    if (DIR_VERTICAL == lcd_ctl.dir)
    {
        tmp = sx;
        sx = lcd_ctl.width - height - sy;
        xend = sx + height - 1;
        yend = tmp + width - 1;
        sy = tmp;
    }
    else if (DIR_HORIZONTAL_MIRROR == lcd_ctl.dir)
    {
        xend = lcd_ctl.width - sx - 1;
        sx = lcd_ctl.width - width - sx;
        yend = lcd_ctl.height - sy - 1;
        sy = lcd_ctl.height - height - sy;
    }
    else if (DIR_VERTICAL_FLIP == lcd_ctl.dir)
    {
        tmp = sx;
        sx = sy;
        xend = sx + height - 1;
        yend = lcd_ctl.height - tmp - 1;
        sy = lcd_ctl.height - tmp - width;
    }
    else /* DIR_HORIZONTAL */
    {
        xend = sx + width - 1;
        yend = sy + height - 1;
    }

    data[0] = (uint8_t)((sx >> 8) & 0xFF);
    data[1] = (uint8_t)(sx & 0xFF);
    data[2] = (uint8_t)((xend >> 8) & 0xFF);
    data[3] = (uint8_t)(xend & 0xFF);
    WriteComm(0x2A);
    tft_ssd1963_write_data(data, 4);

    data[0] = (uint8_t)((sy >> 8) & 0xFF);
    data[1] = (uint8_t)(sy & 0xFF);
    data[2] = (uint8_t)((yend >> 8) & 0xFF);
    data[3] = (uint8_t)(yend & 0xFF);
    WriteComm(0x2B);
    tft_ssd1963_write_data(data, 4);

    WriteComm(0x2C);
}

uint32_t lcd_color(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint32_t)(r << 16 | g << 8 | b);
}

void lcd_draw_point(uint16_t x, uint16_t y, uint8_t color[3])
{
    lcd_set_area(x, y, 1, 1);
    tft_ssd1963_write_data(color, 3);
}

static void lcd_draw_char(uint16_t x, uint16_t y, char c, uint8_t color[3])
{
    uint8_t data = 0;

    for (uint8_t i = 0; i < 16; i++)
    {
        data = ascii0816[c * 16 + i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (data & 0x80)
            {
                lcd_draw_point(x + j, y, color);
            }
            data <<= 1;
        }
        y++;
    }
}

void lcd_draw_string(uint16_t x, uint16_t y, char *str, uint32_t color)
{
    uint8_t font[3];

    font[0] = (uint8_t)(color >> 16 & 0xFF);
    font[1] = (uint8_t)(color >> 8 & 0xFF);
    font[2] = (uint8_t)(color & 0xFF);

    while (*str)
    {
        lcd_draw_char(x, y, *str, font);
        str++;
        x += 8;
    }
}

static void lcd_ram_draw_char(char c, uint8_t *ptr,
                              uint16_t x, uint16_t y, uint16_t img_width,
                              uint8_t font_color[3], uint8_t bg_color[3])
{
    uint8_t data = 0, *addr = NULL;

    for (uint8_t i = 0; i < 16; i++)
    {
        addr = (uint8_t *)((uint32_t)(ptr) + ((y * img_width + x) * 3));
        data = ascii0816[(c ) * 16 + i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (data & 0x80)
            {
                *(addr + 0) = font_color[0];
                *(addr + 1) = font_color[1];
                *(addr + 2) = font_color[2];
            }
            else
            {
                if (bg_color)
                {
                    *(addr + 0) = bg_color[0];
                    *(addr + 1) = bg_color[1];
                    *(addr + 2) = bg_color[2];
                }
            }
            data <<= 1;
            addr += 3;
        }
        y++;
    }
}

void lcd_draw_string_on_ram(char *str, uint8_t *ptr,
                            uint16_t x, uint16_t y,
                            uint16_t img_width, uint32_t color)
{
    uint8_t font[3];

    font[0] = (uint8_t)(color >> 16 & 0xFF);
    font[1] = (uint8_t)(color >> 8 & 0xFF);
    font[2] = (uint8_t)(color & 0xFF);

    while (*str)
    {
        lcd_ram_draw_char(*str, ptr, x, y, img_width, font, NULL);
        str++;
        x += 8;
    }
}

void lcd_draw_string_to_ram(char *str, uint8_t *ptr, uint32_t font_color, uint32_t bg_color)
{
    uint8_t font[3], bg[3];
    uint16_t x = 0, width = 0;

    font[0] = (uint8_t)(font_color >> 16 & 0xFF);
    font[1] = (uint8_t)(font_color >> 8 & 0xFF);
    font[2] = (uint8_t)(font_color & 0xFF);

    bg[0] = (uint8_t)(bg_color >> 16 & 0xFF);
    bg[1] = (uint8_t)(bg_color >> 8 & 0xFF);
    bg[2] = (uint8_t)(bg_color & 0xFF);

    width = 8 * strlen(str);

    while (*str)
    {
        lcd_ram_draw_char(*str, ptr, x, 0, width, font, bg);
        str++;
        x += 8;
    }
}

static uint8_t get_zh_CN_dat(uint8_t *zh_CN_char, uint8_t font_dat[32])
{
    for (uint16_t index = 0; index < sizeof(zh_CN_font_table) / 3; index++)
    {
        if (*(zh_CN_char) == zh_CN_font_table[index * 3])
        {
            if (*(zh_CN_char + 1) == zh_CN_font_table[index * 3 + 1])
            {
                if (*(zh_CN_char + 2) == zh_CN_font_table[index * 3 + 2])
                {
                    memcpy(font_dat, zh_CN_font_dat[index], 32);
                    return 1;
                }
            }
        }
    }
    return 0;
}

static void lcd_draw_zh_CN_char(uint16_t x, uint16_t y, uint8_t *zh_CN_char, uint8_t color[3])
{
    uint8_t zh_CN_dat[32];
    uint16_t data;

    if (get_zh_CN_dat(zh_CN_char, zh_CN_dat))
    {
        for (uint8_t t = 0; t < 16; t++)
        {
            data = zh_CN_dat[t * 2] << 8 | zh_CN_dat[t * 2 + 1];
            for (uint8_t t1 = 0; t1 < 16; t1++)
            {
                if (data & 0x8000)
                {
                    lcd_draw_point(x + t1, y, color);
                }
                data <<= 1;
            }
            y++;
        }
    }
}

void lcd_draw_zh_CN_string(uint16_t x, uint16_t y, uint8_t *zh_CN_string, uint32_t color)
{
    uint8_t font[3];

    font[0] = (uint8_t)(color >> 16 & 0xFF);
    font[1] = (uint8_t)(color >> 8 & 0xFF);
    font[2] = (uint8_t)(color & 0xFF);

    while (*zh_CN_string && *(zh_CN_string + 1) && *(zh_CN_string + 2))
    {
        lcd_draw_zh_CN_char(x, y, zh_CN_string, font);
        zh_CN_string += 3;
        x += 16;
    }
}

static void lcd_ram_draw_zh_CN_char(uint8_t *zh_CN_char, uint8_t *ptr,
                                    uint16_t x, uint16_t y, uint16_t img_width,
                                    uint8_t font_color[3], uint8_t bg_color[3])
{
    uint8_t zh_CN_dat[32], *addr = NULL;
    uint16_t data;

    if (get_zh_CN_dat(zh_CN_char, zh_CN_dat))
    {
        for (uint8_t t = 0; t < 16; t++)
        {
            addr = (uint8_t *)((uint32_t)(ptr) + ((y * img_width + x) * 3));
            data = zh_CN_dat[t * 2] << 8 | zh_CN_dat[t * 2 + 1];
            for (uint8_t t1 = 0; t1 < 16; t1++)
            {
                if (data & 0x8000)
                {
                    *(addr + 0) = font_color[0];
                    *(addr + 1) = font_color[1];
                    *(addr + 2) = font_color[2];
                }
                else
                {
                    if (bg_color)
                    {
                        *(addr + 0) = bg_color[0];
                        *(addr + 1) = bg_color[1];
                        *(addr + 2) = bg_color[2];
                    }
                }
                data <<= 1;
                addr += 3;
            }
            y++;
        }
    }
}

void lcd_draw_zh_CN_string_on_ram(uint8_t *zh_CN_string, uint32_t zh_CN_str_len, uint8_t *ptr,
                                  uint16_t x, uint16_t y,
                                  uint16_t img_width, uint32_t color)
{
    uint8_t font[3];

    font[0] = (uint8_t)(color >> 16 & 0xFF);
    font[1] = (uint8_t)(color >> 8 & 0xFF);
    font[2] = (uint8_t)(color & 0xFF);

    while (*zh_CN_string && *(zh_CN_string + 1) && *(zh_CN_string + 2))
    {
        lcd_ram_draw_zh_CN_char(zh_CN_string, ptr, x, y, img_width, font, NULL);
        zh_CN_string += 3;
        x += 16;
    }
}

void lcd_draw_zh_CN_string_to_ram(uint8_t *zh_CN_string, uint32_t zh_CN_str_len, uint8_t *ptr,
                                  uint32_t font_color, uint32_t bg_color)
{
    uint8_t font[3], bg[3];
    uint16_t x = 0, width = 0;

    font[0] = (uint8_t)(font_color >> 16 & 0xFF);
    font[1] = (uint8_t)(font_color >> 8 & 0xFF);
    font[2] = (uint8_t)(font_color & 0xFF);

    bg[0] = (uint8_t)(bg_color >> 16 & 0xFF);
    bg[1] = (uint8_t)(bg_color >> 8 & 0xFF);
    bg[2] = (uint8_t)(bg_color & 0xFF);

    width = 16 * zh_CN_str_len;

    while (*zh_CN_string && *(zh_CN_string + 1) && *(zh_CN_string + 2))
    {
        lcd_ram_draw_zh_CN_char(zh_CN_string, ptr, x, 0, width, font, bg);
        zh_CN_string += 3;
        x += 16;
    }
}

void lcd_draw_rectangle(rectangle_t rectangle, uint16_t width, uint32_t color)
{
    /* 最大线条限制 */
    uint16_t x1, y1, x2, y2;
    uint8_t data_buf[800 * 3 * 3] = {0};
    uint8_t tmp[3];

    tmp[0] = (uint8_t)(color >> 16 & 0xFF);
    tmp[1] = (uint8_t)(color >> 8 & 0xFF);
    tmp[2] = (uint8_t)(color & 0xFF);

    for (uint32_t i = 0; i < sizeof(data_buf) / 3; i++)
    {
        data_buf[i * 3 + 0] = tmp[0];
        data_buf[i * 3 + 1] = tmp[1];
        data_buf[i * 3 + 2] = tmp[2];
    }

    x1 = rectangle.x;
    y1 = rectangle.y;
    x2 = x1 + rectangle.w;
    y2 = y1 + rectangle.h;

    lcd_set_area(x1, y1, (x2 - x1), width);
    tft_ssd1963_write_data(data_buf, width * (x2 - x1) * 3);

    lcd_set_area(x1, y1, width, (y2 - y1));
    tft_ssd1963_write_data(data_buf, width * (y2 - y1) * 3);

    lcd_set_area(x1, y2, (x2 - x1), width);
    tft_ssd1963_write_data(data_buf, width * (x2 - x1) * 3);

    lcd_set_area(x2, y1, width, (y2 - y1));
    tft_ssd1963_write_data(data_buf, width * (y2 - y1) * 3);
}

void lcd_ram_draw_rectangle(uint8_t *gram,
                            rectangle_t rectangle,
                            uint16_t width, uint16_t height,
                            uint32_t color)
{
    uint8_t *addr1 = NULL, *addr2 = NULL, *addr3 = NULL, *addr4 = NULL;
    uint16_t x1, y1, x2, y2;
    uint8_t tmp[3];

    tmp[0] = (uint8_t)(color >> 16 & 0xFF);
    tmp[1] = (uint8_t)(color >> 8 & 0xFF);
    tmp[2] = (uint8_t)(color & 0xFF);

    x1 = rectangle.x;
    y1 = rectangle.y;
    x2 = x1 + rectangle.w;
    y2 = y1 + rectangle.h;

    if (x1 <= 0)
        x1 = 1;
    if (x2 >= width - 1)
        x2 = width - 2;
    if (y1 <= 0)
        y1 = 1;
    if (y2 >= height - 1)
        y2 = height - 2;

    addr1 = gram + (width * y1 + x1) * 3;
    addr2 = gram + (width * y1 + x2 - 8) * 3;
    addr3 = gram + (width * (y2 - 1) + x1) * 3;
    addr4 = gram + (width * (y2 - 1) + x2 - 8) * 3;

    for (uint32_t i = 0; i < 4; i++)
    {
        *(addr1 + 0) = tmp[0];
        *(addr1 + 1) = tmp[1];
        *(addr1 + 2) = tmp[2];

        *(addr1 + width * 3 + 0) = tmp[0];
        *(addr1 + width * 3 + 0) = tmp[1];
        *(addr1 + width * 3 + 0) = tmp[2];

        *(addr2 + 0) = tmp[0];
        *(addr2 + 1) = tmp[1];
        *(addr2 + 2) = tmp[2];

        *(addr2 + width * 3 + 0) = tmp[0];
        *(addr2 + width * 3 + 0) = tmp[1];
        *(addr2 + width * 3 + 0) = tmp[2];

        *(addr3 + 0) = tmp[0];
        *(addr3 + 1) = tmp[1];
        *(addr3 + 2) = tmp[2];

        *(addr3 + width * 3 + 0) = tmp[0];
        *(addr3 + width * 3 + 0) = tmp[1];
        *(addr3 + width * 3 + 0) = tmp[2];

        *(addr4 + 0) = tmp[0];
        *(addr4 + 1) = tmp[1];
        *(addr4 + 2) = tmp[2];

        *(addr4 + width * 3 + 0) = tmp[0];
        *(addr4 + width * 3 + 0) = tmp[1];
        *(addr4 + width * 3 + 0) = tmp[2];

        addr1 += 3;
        addr2 += 3;
        addr3 += 3;
        addr4 += 3;
    }

    addr1 = gram + (width * y1 + x1) * 3;
    addr2 = gram + (width * y1 + x2 - 2) * 3;
    addr3 = gram + (width * (y2 - 8) + x1) * 3;
    addr4 = gram + (width * (y2 - 8) + x2 - 2) * 3;

    for (uint32_t i = 0; i < 8; i++)
    {
        *(addr1 + 0) = tmp[0];
        *(addr1 + 1) = tmp[1];
        *(addr1 + 2) = tmp[2];

        *(addr2 + 0) = tmp[0];
        *(addr2 + 1) = tmp[1];
        *(addr2 + 2) = tmp[2];

        *(addr3 + 0) = tmp[0];
        *(addr3 + 1) = tmp[1];
        *(addr3 + 2) = tmp[2];

        *(addr4 + 0) = tmp[0];
        *(addr4 + 1) = tmp[1];
        *(addr4 + 2) = tmp[2];

        addr1 += width * 3;
        addr2 += width * 3;
        addr3 += width * 3;
        addr4 += width * 3;
    }
}

void lcd_draw_picture(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t *ptr)
{
    lcd_set_area(x, y, width, height);
    tft_ssd1963_write_data(ptr, width * height * 3);
}

void lcd_clear(uint32_t color)
{
    uint8_t line_buf[LCD_X_MAX * 3];
    uint8_t tmp[3];

    tmp[0] = (uint8_t)(color >> 16 & 0xFF);
    tmp[1] = (uint8_t)(color >> 8 & 0xFF);
    tmp[2] = (uint8_t)(color & 0xFF);

    for (uint32_t i = 0; i < lcd_ctl.width; i++)
    {
        line_buf[i * 3 + 0] = tmp[0];
        line_buf[i * 3 + 1] = tmp[1];
        line_buf[i * 3 + 2] = tmp[2];
    }

    lcd_set_area(0, 0, lcd_ctl.display_width, lcd_ctl.display_height);

    for (uint32_t i = 0; i < lcd_ctl.height; i++)
    {
        tft_ssd1963_write_data(line_buf, lcd_ctl.width * 3);
    }
}

void lcd_fill_color_box(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color)
{
    uint8_t line_buf[LCD_X_MAX * 3];
    uint8_t tmp[3];

    tmp[0] = (uint8_t)(color >> 16 & 0xFF);
    tmp[1] = (uint8_t)(color >> 8 & 0xFF);
    tmp[2] = (uint8_t)(color & 0xFF);

    width = (width > LCD_X_MAX) ? LCD_X_MAX : width;

    for (uint32_t i = 0; i < width; i++)
    {
        line_buf[i * 3 + 0] = tmp[0];
        line_buf[i * 3 + 1] = tmp[1];
        line_buf[i * 3 + 2] = tmp[2];
    }

    lcd_set_area(0, 0, width, height);

    for (uint32_t i = 0; i < height; i++)
    {
        tft_ssd1963_write_data(line_buf, width * 3);
    }
}
#endif
