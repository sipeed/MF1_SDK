#ifndef __FACE_CB_H
#define __FACE_CB_H

#include "stdint.h"

#include "face_lib.h"

extern uint8_t delay_flag;

void face_cb_init(void);

void convert_320x240_to_240x240(uint8_t *img_320, uint16_t x_offset);

void lcd_display_image_alpha(uint32_t pic_addr, uint32_t alpha);

void lcd_draw_pass(void);

uint8_t judge_face_by_keypoint(key_point_t *kp);
void protocol_record_face(proto_record_face_cfg_t *cfg);

void detected_face_cb(face_recognition_ret_t *face);
void fake_face_cb(face_recognition_ret_t *face);
void pass_face_cb(face_recognition_ret_t *face, uint8_t ir_check);
void lcd_refresh_cb(void);
void lcd_convert_cb(void);

#endif
