#ifndef __FACE_CB_H
#define __FACE_CB_H

#include "stdint.h"

#include "face_lib.h"

void detected_face_cb(face_recognition_ret_t *face);
void fake_face_cb(face_recognition_ret_t *face);
void pass_face_cb(face_recognition_ret_t *face, uint8_t ir_check);
void lcd_refresh_cb(void);

void lcd_draw_pass(void);

#if CONFIG_LCD_TYPE_ST7789
void display_lcd_img_addr(uint32_t pic_addr);
void display_fit_lcd_with_alpha(uint8_t *pic_addr, uint8_t *out_img, uint8_t alpha);
#elif CONFIG_LCD_TYPE_SSD1963

#elif CONFIG_LCD_TYPE_SIPEED
void display_fit_lcd_with_alpha_v1(uint8_t *pic_flash_addr, uint8_t *in_img, uint8_t *out_img, uint8_t alpha);
#endif

#endif
