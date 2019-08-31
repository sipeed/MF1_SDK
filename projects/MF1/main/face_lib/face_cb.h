#ifndef __FACE_CB_H
#define __FACE_CB_H

#include "stdint.h"

#include "face_lib.h"

extern uint8_t delay_flag;

void face_cb_init(void);

void detected_face_cb(face_recognition_ret_t *face);
void fake_face_cb(face_recognition_ret_t *face);
void pass_face_cb(face_recognition_ret_t *face, uint8_t ir_check);
void lcd_refresh_cb(void);
void lcd_convert_cb(void);

void lcd_draw_pass(void);

#endif
