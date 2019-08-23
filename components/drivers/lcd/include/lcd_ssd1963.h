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
#ifndef _LCD_SSD1963_H_
#define _LCD_SSD1963_H_

#include <stdint.h>
#include "lcd_sipeed.h"

#if CONFIG_LCD_TYPE_SSD1963
/* clang-format off */
#define LCD_X_MAX   (800)
#define LCD_Y_MAX   (480)
/* clang-format on */

typedef enum _lcd_dir
{
    DIR_HORIZONTAL = 0,
    DIR_VERTICAL = 1,
    DIR_HORIZONTAL_MIRROR = 2,
    DIR_VERTICAL_FLIP = 3,
} lcd_dir_t;

typedef struct _lcd_ctl
{
    uint8_t mode;
    uint8_t dir;
    uint16_t width;
    uint16_t height;

    uint16_t display_width;
    uint16_t display_height;
} lcd_ctl_t;

typedef enum _panletype
{
    PANEL_LCD = 0x0,
    PANEL_VGA_MODULE = 0x1,
} panletype_t;

typedef struct _ss1963_lcd_panel
{
    panletype_t panel_type;

    uint8_t clk_Mhz; //MHz

    uint16_t width;
    uint16_t height;

    uint16_t hsync_front_porch;
    uint16_t hsync_back_porch;
    uint8_t hsync_pulse;

    uint16_t vsync_front_porch;
    uint16_t vsync_back_porch;
    uint8_t vsync_pulse;
} ss1963_lcd_panel_t;

typedef struct _rectangle
{
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
}rectangle_t;

extern lcd_ctl_t lcd_ctl;
extern ss1963_lcd_panel_t lcd_800x480_5inch;
extern ss1963_lcd_panel_t lcd_480x272_4_3inch;
extern ss1963_lcd_panel_t lcd_vga_module;

void lcd_init(ss1963_lcd_panel_t *lcd);

void lcd_set_dir(uint8_t val);

void lcd_set_area(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height);

uint32_t lcd_color(uint8_t r, uint8_t g, uint8_t b);

void lcd_draw_point(uint16_t x, uint16_t y, uint8_t color[3]);

void lcd_draw_string(uint16_t x, uint16_t y, char *str, uint32_t color);

void lcd_draw_string_on_ram(char *str, uint8_t *ptr,
                            uint16_t x, uint16_t y,
                            uint16_t img_width, uint32_t color);

void lcd_draw_string_to_ram(char *str, uint8_t *ptr, uint32_t font_color, uint32_t bg_color);

void lcd_draw_zh_CN_string(uint16_t x, uint16_t y, uint8_t *zh_CN_string, uint32_t color);

void lcd_draw_zh_CN_string_on_ram(uint8_t *zh_CN_string, uint32_t zh_CN_str_len, uint8_t *ptr,
                                  uint16_t x, uint16_t y,
                                  uint16_t img_width, uint32_t color);

void lcd_draw_zh_CN_string_to_ram(uint8_t *zh_CN_string, uint32_t zh_CN_str_len, uint8_t *ptr,
                                  uint32_t font_color, uint32_t bg_color);

void lcd_draw_rectangle(rectangle_t rectangle, uint16_t width, uint32_t color);
void lcd_ram_draw_rectangle(uint8_t *gram,
                            rectangle_t rectangle,
                            uint16_t width, uint16_t height,
                            uint32_t color);

void lcd_draw_picture(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t *ptr);

void lcd_clear(uint32_t color);

void lcd_fill_color_box(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color);

#endif
#endif