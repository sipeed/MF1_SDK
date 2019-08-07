#ifndef __FACE_CB_H
#define __FACE_CB_H

#include "stdint.h"

#include "face_lib.h"

void draw_edge(uint32_t *gram, face_obj_t *obj, uint16_t color);
void record_face(face_recognition_ret_t *ret);
void box_false_face(face_recognition_ret_t *ret);
void recognition_draw_callback(uint8_t *cam_image);

void utils_display_pass(void);

#endif
