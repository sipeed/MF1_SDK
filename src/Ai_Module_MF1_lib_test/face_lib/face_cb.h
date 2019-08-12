#ifndef __FACE_CB_H
#define __FACE_CB_H

#include "stdint.h"

#include "face_lib.h"

void display_lcd_img_addr(uint32_t pic_addr);
void display_fit_lcd_with_alpha(uint8_t *pic_addr, uint8_t *out_img, uint8_t alpha);

void lcd_draw_edge(face_obj_t *obj, uint32_t color);
void lcd_draw_picture_cb(void);
void lcd_draw_false_face(face_recognition_ret_t *ret);
void record_face(face_recognition_ret_t *ret);

void lcd_draw_pass(void);

#endif
